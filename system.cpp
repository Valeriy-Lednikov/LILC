#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <string_view>
#include <cstddef>
#include <cstring>

// ===== Прозрачные хеш/eq для string/string_view (для интернера) =====
struct StringHash
{
    using is_transparent = void;
    size_t operator()(std::string_view s) const noexcept
    {
        return std::hash<std::string_view>{}(s);
    }
    size_t operator()(const std::string &s) const noexcept
    {
        return std::hash<std::string_view>{}(s);
    }
};
struct StringEq
{
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
    bool operator()(const std::string &a, const std::string &b) const noexcept { return a == b; }
    bool operator()(const std::string &a, std::string_view b) const noexcept { return a == b; }
    bool operator()(std::string_view a, const std::string &b) const noexcept { return a == b; }
};

// ===== Хеш/eq по указателю (для ключа Id = const char*) =====
using Id = const char *;

struct PtrHash
{
    size_t operator()(Id p) const noexcept { return reinterpret_cast<size_t>(p); }
};
struct PtrEq
{
    bool operator()(Id a, Id b) const noexcept { return a == b; }
};

// ===== Интернер: одна копия каждой строки, стабильный const char* =====
class Interner
{
public:
    void reserve(size_t n)
    {
        pool_.reserve(n);
        ptrs_.reserve(n);
    }

    // Заинтернить строку и вернуть стабильный указатель
    Id intern(std::string_view s)
    {
        auto [it, inserted] = pool_.emplace(s); // std::string создаётся один раз
        Id p = it->c_str();
        ptrs_.insert(p); // для отладочных проверок
        return p;
    }

    // Найти без вставки (если нет — nullptr). Не используем в горячем пути.
    Id try_get(std::string_view s) const
    {
        auto it = pool_.find(std::string(s));
        return (it == pool_.end()) ? nullptr : it->c_str();
    }

    size_t size() const noexcept { return pool_.size(); }

    bool is_interned(Id p) const noexcept { return ptrs_.find(p) != ptrs_.end(); }

private:
    std::unordered_set<std::string, StringHash, StringEq> pool_;
    std::unordered_set<Id, PtrHash, PtrEq> ptrs_;
};

// Глобальный доступ без ODR-проблем
inline Interner &INTERN()
{
    static Interner I;
    return I;
}

inline std::unordered_set<Id, PtrHash, PtrEq> &TINY()
{
    static std::unordered_set<Id, PtrHash, PtrEq> S;
    return S;
}

// ===== Набор "символов" (ключевые слова, операторы) =====
struct Symbols
{
    // Ключевые слова
    Id VAR = nullptr, CONST = nullptr, SET = nullptr, IF = nullptr, ELSE = nullptr, WHILE = nullptr,
       PROC = nullptr, RETURN = nullptr, PRINT = nullptr, PRINTLN = nullptr, HALT = nullptr;

    // Разделители/операторы
    Id LBRACE = nullptr, RBRACE = nullptr, LP = nullptr, RP = nullptr,
       LBRACKET = nullptr, RBRACKET = nullptr, SEMI = nullptr, EQ = nullptr,
       PLUS = nullptr, MINUS = nullptr, STAR = nullptr, SLASH = nullptr,
       EQEQ = nullptr, NEQ = nullptr, LEQ = nullptr, GEQ = nullptr,
       LT = nullptr, GT = nullptr, COMMA = nullptr, QUOTE = nullptr, NOT = nullptr, CARET = nullptr, PERCENT = nullptr;

    Id ABS = nullptr, ACOS = nullptr, ASIN = nullptr, ATAN = nullptr, ATAN2 = nullptr,
       CEIL = nullptr, COS = nullptr, COSH = nullptr, EXP = nullptr, FAC = nullptr,
       FLOOR = nullptr, LN = nullptr, LOG = nullptr, LOG10 = nullptr, NCR = nullptr, NPR = nullptr,
       PIK = nullptr, /*  'pi' назови PIK чтобы не путать с полем PERCENT  */
        POW = nullptr, SIN = nullptr, SINH = nullptr, SQRT = nullptr, TAN = nullptr, TANH = nullptr;

    void init(Interner &I)
    {
        // ключевые слова
        VAR = I.intern("VAR");
        CONST = I.intern("CONST");
        SET = I.intern("SET");
        IF = I.intern("IF");
        ELSE = I.intern("ELSE");
        WHILE = I.intern("WHILE");
        PROC = I.intern("PROC");
        RETURN = I.intern("RETURN");
        PRINT = I.intern("PRINT");
        PRINTLN = I.intern("PRINTLN");
        HALT = I.intern("HALT");

        // скобки/разделители
        LBRACE = I.intern("{");
        RBRACE = I.intern("}");
        LP = I.intern("(");
        RP = I.intern(")");
        LBRACKET = I.intern("[");
        RBRACKET = I.intern("]");
        SEMI = I.intern(";");
        COMMA = I.intern(",");
        QUOTE = I.intern("\"");

        // операторы
        EQ = I.intern("=");
        PLUS = I.intern("+");
        MINUS = I.intern("-");
        STAR = I.intern("*");
        SLASH = I.intern("/");
        EQEQ = I.intern("==");
        NEQ = I.intern("!=");
        LEQ = I.intern("<=");
        GEQ = I.intern(">=");
        LT = I.intern("<");
        GT = I.intern(">");
        NOT = I.intern("!");
        CARET = I.intern("^");
        PERCENT = I.intern("%");

        ABS = I.intern("abs");
        ACOS = I.intern("acos");
        ASIN = I.intern("asin");
        ATAN = I.intern("atan");
        ATAN2 = I.intern("atan2");
        CEIL = I.intern("ceil");
        COS = I.intern("cos");
        COSH = I.intern("cosh");
        EXP = I.intern("exp");
        FAC = I.intern("fac");
        FLOOR = I.intern("floor");
        LN = I.intern("ln");
        LOG = I.intern("log");
        LOG10 = I.intern("log10");
        NCR = I.intern("ncr");
        NPR = I.intern("npr");
        PIK = I.intern("pi");
        POW = I.intern("pow");
        SIN = I.intern("sin");
        SINH = I.intern("sinh");
        SQRT = I.intern("sqrt");
        TAN = I.intern("tan");
        TANH = I.intern("tanh");

        // const char *tiny_names[] = {
        //     "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "exp", "fac",
        //     "floor", "ln", "log", "log10", "ncr", "npr", "pi", "pow", "sin", "sinh", "sqrt",
        //     "tan", "tanh", nullptr};
        // for (const char **p = tiny_names; *p; ++p)
        //     TINY().insert(I.intern(*p));
    }
};

inline Symbols &SYM()
{
    static Symbols S;
    return S;
}

// ===== Вспомогательные структуры только для отладки/печати =====
struct variable
{
    Id id = nullptr;
    double value = 0.0;
    bool isConst = false;

    variable() = default;
    variable(Id id_, double v, bool c = false) : id(id_), value(v), isConst(c) {}
};

struct array_var
{
    Id id = nullptr;
    std::vector<double> data;
    array_var() = default;
    array_var(Id id_, const std::vector<double> &d) : id(id_), data(d) {}
};

// ===== Контроллер с O(1) доступом через "живой" слой и стек затенений =====
class controller
{
private:
    int currentLevel = 0;


    struct VarEntry
    {
        double value = 0.0;
        bool isConst = false;
    };

    // Фреймы, владеющие значениями
    std::vector<std::unordered_map<Id, VarEntry, PtrHash, PtrEq>> varFrames = {{}};
    std::vector<std::unordered_map<Id, std::vector<double>, PtrHash, PtrEq>> arrFrames = {{}};

    // Живой видимый слой: быстрые лукапы
    std::unordered_map<Id, VarEntry *, PtrHash, PtrEq> liveVars;
    std::unordered_map<Id, std::vector<double> *, PtrHash, PtrEq> liveArrays;

    // Журнал изменений для отката уровня
    struct VarChange
    {
        Id id;
        VarEntry *prev;
    };
    struct ArrChange
    {
        Id id;
        std::vector<double> *prev;
    };

    std::vector<VarChange> varLog;
    std::vector<ArrChange> arrLog;
    std::vector<size_t> varMarks{0}, arrMarks{0}; // индексы начала изменений уровня


    static constexpr int kVSlots = 5;
    static constexpr int kASlots = 5;

    mutable Id v_id_[kVSlots] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    mutable VarEntry *v_ptr_[kVSlots] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    mutable int v_hand_ = 0;

    mutable Id a_id_[kASlots] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    mutable std::vector<double> *a_ptr_[kASlots] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    mutable int a_hand_ = 0;

    inline void clearHotCaches() noexcept
    {
        for (int i = 0; i < kVSlots; ++i)
        {
            v_id_[i] = nullptr;
            v_ptr_[i] = nullptr;
        }
        for (int i = 0; i < kASlots; ++i)
        {
            a_id_[i] = nullptr;
            a_ptr_[i] = nullptr;
        }
        v_hand_ = 0;
        a_hand_ = 0;
    }

    inline VarEntry  *cacheLookupVar(Id name) const noexcept
    {
        // быстрый проход по 5 слотам
        for (int i = 0; i < kVSlots; ++i)
            if (v_id_[i] == name)
                return v_ptr_[i];
        // промах → один lookup в liveVars
        auto it = liveVars.find(name);
        if (it == liveVars.end())
            return nullptr;
        // заносим в кольцевой слот
        v_id_[v_hand_] = name;
        v_ptr_[v_hand_] = it->second;
        v_hand_ = (v_hand_ + 1) % kVSlots;
        return it->second;
    }

    inline std::vector<double> *cacheLookupArr(Id name) const noexcept
    {
        for (int i = 0; i < kASlots; ++i)
            if (a_id_[i] == name)
                return a_ptr_[i];
        auto it = liveArrays.find(name);
        if (it == liveArrays.end())
            return nullptr;
        a_id_[a_hand_] = name;
        a_ptr_[a_hand_] = it->second;
        a_hand_ = (a_hand_ + 1) % kASlots;
        return it->second;
    }

public:
    // --- Управление уровнями ---
    void inLevel()
    {
        ++currentLevel;
        if ((int)varFrames.size() <= currentLevel)
            varFrames.emplace_back();
        if ((int)arrFrames.size() <= currentLevel)
            arrFrames.emplace_back();
        varMarks.push_back(varLog.size());
        arrMarks.push_back(arrLog.size());
        clearHotCaches();
    }

    void outLevel()
    {
        if (currentLevel <= 0)
        {
            std::cerr << "Already at base level, can't go lower!\n";
            return;
        }

        // Откат массивов
        size_t aMark = arrMarks.back();
        while (arrLog.size() > aMark)
        {
            auto ch = arrLog.back();
            arrLog.pop_back();
            if (ch.prev == nullptr)
                liveArrays.erase(ch.id);
            else
                liveArrays[ch.id] = ch.prev;
        }
        arrMarks.pop_back();

        // Откат переменных
        size_t vMark = varMarks.back();
        while (varLog.size() > vMark)
        {
            auto ch = varLog.back();
            varLog.pop_back();
            if (ch.prev == nullptr)
                liveVars.erase(ch.id);
            else
                liveVars[ch.id] = ch.prev;
        }
        varMarks.pop_back();

        // Удаляем фреймы уровня (владение значениями)
        varFrames.pop_back();
        arrFrames.pop_back();
        --currentLevel;
        clearHotCaches();
    }

    // --- Переменные ---
    void addVar(Id name) { addVar(name, 0.0, false); }
    void addVar(Id name, double value) { addVar(name, value, false); }

    void addVar(Id name, double value, bool isConst)
    {
        auto &frame = varFrames[currentLevel];

        auto [it, inserted] = frame.emplace(name, VarEntry{value, isConst});
        if (!inserted)
        {
            it->second.value = value;
            it->second.isConst = isConst;
        }

        VarEntry *prev = nullptr;
        if (auto itLive = liveVars.find(name); itLive != liveVars.end())
            prev = itLive->second;

        varLog.push_back({name, prev});
        liveVars[name] = &it->second;
        clearHotCaches();
    }

    bool setVar(Id name, double value)
    {
        if (VarEntry *p = cacheLookupVar(name))
        {
            if (p->isConst)
                return false; // константу нельзя менять
            p->value = value;
            return true;
        }
        return false;
    }

    bool isVarConstant(Id name){
        if (VarEntry *p = cacheLookupVar(name))
        {
            if (p->isConst){
                return true;
            }
        }
        return false;
    }


    bool getVar(Id name, double &outValue) const
    {
        if (VarEntry  *p = cacheLookupVar(name))
        {
            outValue = p->value;
            return true;
        }
        return false;
    }

    bool findVar(Id name) const
    {
        return cacheLookupVar(name) != nullptr;
    }

    double *getVarPtr(Id name) const
    { // тоже через кэш
        if (VarEntry *p = cacheLookupVar(name))
            return &p->value;
        return nullptr;
    }
    std::vector<double> *getArrayPtr(Id name) const
    {
        return cacheLookupArr(name);
    }

    // --- Массивы ---
    void addArray(Id name, size_t size, double init = 0.0)
    {
        auto &frame = arrFrames[currentLevel];
        auto [it, inserted] = frame.emplace(name, std::vector<double>(size, init));
        if (!inserted)
            it->second.assign(size, init);

        std::vector<double> *prev = nullptr;
        if (auto itLive = liveArrays.find(name); itLive != liveArrays.end())
            prev = itLive->second;
        arrLog.push_back({name, prev});
        liveArrays[name] = &it->second;
        clearHotCaches();
    }

    void addArray(Id name, const std::vector<double> &values)
    {
        auto &frame = arrFrames[currentLevel];
        auto [it, inserted] = frame.emplace(name, values);
        if (!inserted)
            it->second = values;

        std::vector<double> *prev = nullptr;
        if (auto itLive = liveArrays.find(name); itLive != liveArrays.end())
            prev = itLive->second;
        arrLog.push_back({name, prev});
        liveArrays[name] = &it->second;
        clearHotCaches();
    }

    bool findArray(Id name) const
    {
        return cacheLookupArr(name) != nullptr;
    }

    bool setArrayElem(Id name, size_t index, double value)
    {
        if (auto *vec = cacheLookupArr(name))
        {
            if (index >= vec->size())
            {
                std::cerr << "Index out of bounds for array '" << name << "': "
                          << index << " >= " << vec->size() << "\n";
                return false;
            }
            (*vec)[index] = value;
            return true;
        }
        return false;
    }

    bool setArrayElem(Id name, int index, double value)
    {
        if (index < 0)
        {
            std::cerr << "Negative index for array '" << name << "': " << index << "\n";
            return false;
        }
        return setArrayElem(name, static_cast<size_t>(index), value);
    }

    bool getArrayElem(Id name, size_t index, double &outValue) const
    {
        if (auto *vec = cacheLookupArr(name))
        {
            if (index >= vec->size())
            {
                std::cerr << "Index out of bounds for array '" << name << "': "
                          << index << " >= " << vec->size() << "\n";
                return false;
            }
            outValue = (*vec)[index];
            return true;
        }
        return false;
    }

    bool getArrayElem(Id name, int index, double &outValue) const
    {
        if (index < 0)
        {
            std::cerr << "Negative index for array '" << name << "': " << index << "\n";
            return false;
        }
        return getArrayElem(name, static_cast<size_t>(index), outValue);
    }

    bool resizeArray(Id name, size_t newSize, double init = 0.0)
    {
        if (auto *vec = cacheLookupArr(name))
        {
            vec->assign(newSize, init);
            return true;
        }
        return false;
    }

    // --- Отладочная печать ---
    void printCurrentLevel() const
    {
        std::cout << "Level " << currentLevel << " variables:\n";
        for (const auto &kv : varFrames[currentLevel])
        {
            std::cout << kv.first << " = " << kv.second.value;
            if (kv.second.isConst) std::cout << " [CONST]";
            std::cout << "\n";
        }
    }


    void printAllLevels() const
    {
        std::cout << "All levels:\n";
        for (size_t lvl = 0; lvl < varFrames.size(); ++lvl)
        {
            std::cout << "Level " << lvl << ":\n";
            for (const auto &kv : varFrames[lvl])
            {
                std::cout << "  " << kv.first << " = " << kv.second.value;
                if (kv.second.isConst) std::cout << " [CONST]";
                std::cout << "\n";
            }
        }
    }


    // --- Снимки уровня для UI/отладки ---
    const std::vector<variable> &getVarsAtLevel(int level) const
    {
        static std::vector<variable> tmp;
        tmp.clear();
        for (const auto &kv : varFrames[level])
            tmp.emplace_back(kv.first, kv.second.value, kv.second.isConst);
        return tmp;
    }

    const std::vector<array_var> &getArraysAtLevel(int level) const
    {
        static std::vector<array_var> tmp;
        tmp.clear();
        for (const auto &kv : arrFrames[level])
            tmp.emplace_back(kv.first, kv.second);
        return tmp;
    }

    int getCurrentLevel() const { return currentLevel; }

    // --- Подготовка ёмкостей (необязательно, но уменьшает rehash) ---
    void reserve(size_t varsPerLevel, size_t arraysPerLevel, size_t levels = 8)
    {
        varFrames.reserve(levels);
        arrFrames.reserve(levels);
        for (auto &f : varFrames)
            f.reserve(varsPerLevel);
        for (auto &f : arrFrames)
            f.reserve(arraysPerLevel);
        liveVars.reserve(varsPerLevel * levels);
        liveArrays.reserve(arraysPerLevel * levels);
        varLog.reserve(varsPerLevel * levels);
        arrLog.reserve(arraysPerLevel * levels);
        varMarks.reserve(levels + 1);
        arrMarks.reserve(levels + 1);
    }
};
