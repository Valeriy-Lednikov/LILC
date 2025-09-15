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
    Id VAR = nullptr, SET = nullptr, IF = nullptr, ELSE = nullptr, WHILE = nullptr,
       PROC = nullptr, RETURN = nullptr, PRINT = nullptr, PRINTLN = nullptr, HALT = nullptr;

    // Разделители/операторы
    Id LBRACE = nullptr, RBRACE = nullptr, LP = nullptr, RP = nullptr,
       LBRACKET = nullptr, RBRACKET = nullptr, SEMI = nullptr, EQ = nullptr,
       PLUS = nullptr, MINUS = nullptr, STAR = nullptr, SLASH = nullptr,
       EQEQ = nullptr, NEQ = nullptr, LEQ = nullptr, GEQ = nullptr,
       LT = nullptr, GT = nullptr, COMMA = nullptr, QUOTE = nullptr, NOT = nullptr, CARET = nullptr, PERCENT = nullptr;

    void init(Interner &I)
    {
        // ключевые слова
        VAR = I.intern("VAR");
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

        const char *tiny_names[] = {
            "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "exp", "fac",
            "floor", "ln", "log", "log10", "ncr", "npr", "pi", "pow", "sin", "sinh", "sqrt",
            "tan", "tanh", nullptr};
        for (const char **p = tiny_names; *p; ++p)
            TINY().insert(I.intern(*p));
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
    variable() = default;
    variable(Id id_, double v) : id(id_), value(v) {}
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

    // Фреймы, владеющие значениями
    std::vector<std::unordered_map<Id, double, PtrHash, PtrEq>> varFrames = {{}};
    std::vector<std::unordered_map<Id, std::vector<double>, PtrHash, PtrEq>> arrFrames = {{}};

    // Живой видимый слой: быстрые лукапы
    std::unordered_map<Id, double *, PtrHash, PtrEq> liveVars;
    std::unordered_map<Id, std::vector<double> *, PtrHash, PtrEq> liveArrays;

    // Журнал изменений для отката уровня
    struct VarChange
    {
        Id id;
        double *prev;
    };
    struct ArrChange
    {
        Id id;
        std::vector<double> *prev;
    };
    std::vector<VarChange> varLog;
    std::vector<ArrChange> arrLog;
    std::vector<size_t> varMarks{0}, arrMarks{0}; // индексы начала изменений уровня

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
    }

    // --- Переменные ---
    void addVar(Id name) { addVar(name, 0.0); }

    void addVar(Id name, double value)
    {
        auto &frame = varFrames[currentLevel];
        auto [it, inserted] = frame.emplace(name, value);
        if (!inserted)
            it->second = value; // переопределили на уровне

        double *prev = nullptr;
        if (auto itLive = liveVars.find(name); itLive != liveVars.end())
            prev = itLive->second;
        varLog.push_back({name, prev});
        liveVars[name] = &it->second;
    }

    bool setVar(Id name, double value)
    {
        auto it = liveVars.find(name);
        if (it == liveVars.end())
            return false;
        *it->second = value;
        return true;
    }

    bool getVar(Id name, double &outValue) const
    {
        auto it = liveVars.find(name);
        if (it == liveVars.end())
            return false;
        outValue = *it->second;
        return true;
    }

    bool findVar(Id name) const
    {
        return liveVars.find(name) != liveVars.end();
    }

    double *getVarPtr(Id name) const
    {
        auto it = liveVars.find(name);
        return (it == liveVars.end()) ? nullptr : it->second;
    }
    std::vector<double> *getArrayPtr(Id name) const
    {
        auto it = liveArrays.find(name);
        return (it == liveArrays.end()) ? nullptr : it->second;
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
    }

    bool findArray(Id name) const
    {
        return liveArrays.find(name) != liveArrays.end();
    }

    bool setArrayElem(Id name, size_t index, double value)
    {
        auto it = liveArrays.find(name);
        if (it == liveArrays.end())
            return false;
        auto &vec = *it->second;
        if (index >= vec.size())
        {
            std::cerr << "Index out of bounds for array '" << name << "': " << index
                      << " >= " << vec.size() << "\n";
            return false;
        }
        vec[index] = value;
        return true;
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
        auto it = liveArrays.find(name);
        if (it == liveArrays.end())
            return false;
        auto &vec = *it->second;
        if (index >= vec.size())
        {
            std::cerr << "Index out of bounds for array '" << name << "': " << index
                      << " >= " << vec.size() << "\n";
            return false;
        }
        outValue = vec[index];
        return true;
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
        auto it = liveArrays.find(name);
        if (it == liveArrays.end())
            return false;
        it->second->assign(newSize, init);
        return true;
    }

    // --- Отладочная печать ---
    void printCurrentLevel() const
    {
        std::cout << "Level " << currentLevel << " variables:\n";
        for (const auto &kv : varFrames[currentLevel])
        {
            std::cout << kv.first << " = " << kv.second << "\n";
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
                std::cout << "  " << kv.first << " = " << kv.second << "\n";
            }
        }
    }

    // --- Снимки уровня для UI/отладки ---
    const std::vector<variable> &getVarsAtLevel(int level) const
    {
        static std::vector<variable> tmp;
        tmp.clear();
        for (const auto &kv : varFrames[level])
            tmp.emplace_back(kv.first, kv.second);
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
