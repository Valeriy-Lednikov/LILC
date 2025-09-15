
#include <iostream>
#include <vector>
#include <unordered_set>
#include <string>
#include <string_view>
#include <cstddef>
#include <cstring> // Для strcpy, strlen

struct variable
{
    char *name;
    double value;

    // Конструктор
    variable(const char *n, double v = 0.0) : value(v)
    {
        name = new char[strlen(n) + 1];
        std::strcpy(name, n);
    }

    // Деструктор
    ~variable()
    {
        delete[] name;
    }

    // Конструктор копирования
    variable(const variable &other) : value(other.value)
    {
        name = new char[strlen(other.name) + 1];
        std::strcpy(name, other.name);
    }

    // Оператор присваивания
    variable &operator=(const variable &other)
    {
        if (this != &other)
        {
            delete[] name;
            name = new char[strlen(other.name) + 1];
            std::strcpy(name, other.name);
            value = other.value;
        }
        return *this;
    }
};

struct array_var
{
    char *name;
    std::vector<double> data;

    // Конструктор: массив фиксированного размера, заполненный init
    array_var(const char *n, size_t size = 0, double init = 0.0) : data(size, init)
    {
        name = new char[std::strlen(n) + 1];
        std::strcpy(name, n);
    }

    // Конструктор от готовых значений
    array_var(const char *n, const std::vector<double> &values) : data(values)
    {
        name = new char[std::strlen(n) + 1];
        std::strcpy(name, n);
    }

    // Деструктор
    ~array_var()
    {
        delete[] name;
    }

    // Конструктор копирования
    array_var(const array_var &other) : data(other.data)
    {
        name = new char[std::strlen(other.name) + 1];
        std::strcpy(name, other.name);
    }

    // Оператор присваивания
    array_var &operator=(const array_var &other)
    {
        if (this != &other)
        {
            delete[] name;
            name = new char[std::strlen(other.name) + 1];
            std::strcpy(name, other.name);
            data = other.data;
        }
        return *this;
    }

    size_t size() const { return data.size(); }
    void resize(size_t newSize, double init = 0.0) { data.assign(newSize, init); } // заполнение init
};

class controller
{
private:
    int currentLevel = 0;
    std::vector<std::vector<variable>> vars = {{}};
    std::vector<std::vector<array_var>> arrays = {{}};

public:
    void addVar(const char *name)
    {
        vars[currentLevel].emplace_back(name, 0.0);
    }

    void addVar(const char *name, double value)
    {
        vars[currentLevel].emplace_back(name, value);
    }

    void inLevel()
    {
        currentLevel++;
        if (vars.size() <= static_cast<size_t>(currentLevel))
            vars.push_back({});
        if (arrays.size() <= static_cast<size_t>(currentLevel))
            arrays.push_back({});
    }

    void outLevel()
    {
        if (currentLevel > 0)
        {
            vars.pop_back();
            arrays.pop_back();
            currentLevel--;
        }
        else
        {
            std::cerr << "Already at base level, can't go lower!" << std::endl;
        }
    }

    void printCurrentLevel()
    {
        std::cout << "Level " << currentLevel << " variables:\n";
        for (const auto &var : vars[currentLevel])
        {
            std::cout << var.name << " = " << var.value << std::endl;
        }
    }

    void printAllLevels()
    {
        std::cout << "All levels:\n";
        for (size_t level = 0; level < vars.size(); ++level)
        {
            std::cout << "Level " << level << ":\n";
            for (const auto &var : vars[level])
            {
                std::cout << "  " << var.name << " = " << var.value << "\n";
            }
        }
    }

    bool setVar(const char *name, double value)
    {
        for (int level = currentLevel; level >= 0; --level)
        {
            for (auto &var : vars[level])
            {
                if (std::strcmp(var.name, name) == 0)
                {
                    var.value = value;
                    return true;
                }
            }
        }
        return false; // Переменная не найдена
    }

    bool getVar(const char *name, double &outValue)
    {
        for (int level = currentLevel; level >= 0; --level)
        {
            for (const auto &var : vars[level])
            {
                if (std::strcmp(var.name, name) == 0)
                {
                    outValue = var.value;
                    return true;
                }
            }
        }
        return false; // Переменная не найдена
    }

    bool findVar(const char *name)
    {
        for (int level = currentLevel; level >= 0; --level)
        {
            for (const auto &var : vars[level])
            {
                if (std::strcmp(var.name, name) == 0)
                {
                    return true;
                }
            }
        }
        return false; // Переменная не найдена
    }

    const std::vector<variable> &getVarsAtLevel(int level)
    {
        return vars[level];
    }

    int getCurrentLevel()
    {
        return currentLevel;
    }

    // Создать массив заданного размера, заполненный нулями (или init)
    void addArray(const char *name, size_t size, double init = 0.0)
    {
        arrays[currentLevel].emplace_back(name, size, init);
    }

    // Создать массив из набора значений
    void addArray(const char *name, const std::vector<double> &values)
    {
        arrays[currentLevel].emplace_back(name, values);
    }

    // Найти массив по имени (с учетом вложенных уровней)
    bool findArray(const char *name)
    {
        for (int level = currentLevel; level >= 0; --level)
        {
            for (const auto &arr : arrays[level])
            {
                if (std::strcmp(arr.name, name) == 0)
                    return true;
            }
        }
        return false;
    }

    // Установить элемент массива по индексу
    bool setArrayElem(const char *name, size_t index, double value)
    {
        for (int level = currentLevel; level >= 0; --level)
        {
            for (auto &arr : arrays[level])
            {
                if (std::strcmp(arr.name, name) == 0)
                {
                    if (index < arr.data.size())
                    {
                        arr.data[index] = value;
                        return true;
                    }
                    else
                    {
                        std::cerr << "Index out of bounds for array '" << name
                                  << "': " << index << " >= " << arr.data.size() << "\n";
                        return false;
                    }
                }
            }
        }
        return false; // Массив не найден
    }

    // Перегрузка на случай int-индекса (если интерпретатор оперирует int)
    bool setArrayElem(const char *name, int index, double value)
    {
        if (index < 0)
        {
            std::cerr << "Negative index for array '" << name << "': " << index << "\n";
            return false;
        }
        return setArrayElem(name, static_cast<size_t>(index), value);
    }

    // Прочитать элемент массива по индексу
    bool getArrayElem(const char *name, size_t index, double &outValue)
    {
        for (int level = currentLevel; level >= 0; --level)
        {
            for (const auto &arr : arrays[level])
            {
                if (std::strcmp(arr.name, name) == 0)
                {
                    if (index < arr.data.size())
                    {
                        outValue = arr.data[index];
                        return true;
                    }
                    else
                    {
                        std::cerr << "Index out of bounds for array '" << name
                                  << "': " << index << " >= " << arr.data.size() << "\n";
                        return false;
                    }
                }
            }
        }
        return false; // Массив не найден
    }

    bool getArrayElem(const char *name, int index, double &outValue)
    {
        if (index < 0)
        {
            std::cerr << "Negative index for array '" << name << "': " << index << "\n";
            return false;
        }
        return getArrayElem(name, static_cast<size_t>(index), outValue);
    }

    // Изменить размер массива (заполнить init)
    bool resizeArray(const char *name, size_t newSize, double init = 0.0)
    {
        for (int level = currentLevel; level >= 0; --level)
        {
            for (auto &arr : arrays[level])
            {
                if (std::strcmp(arr.name, name) == 0)
                {
                    arr.resize(newSize, init);
                    return true;
                }
            }
        }
        return false; // Массив не найден
    }

    // Получить массивы на уровне (например, для отладочной печати UI)
    const std::vector<array_var> &getArraysAtLevel(int level)
    {
        return arrays[level];
    }
};

// --- Прозрачные хеш/eq для string/string_view (без аллокаций на find) ---
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

// --- Хеш/eq по указателю (для unordered_map<const char*, ...>) ---
struct PtrHash
{
    size_t operator()(const char *p) const noexcept { return reinterpret_cast<size_t>(p); }
};
struct PtrEq
{
    bool operator()(const char *a, const char *b) const noexcept { return a == b; }
};

// --- Интернер: одна копия каждой строки, стабильный const char* ---
class Interner
{
public:
    // Зарезервировать оценочное число уникальных строк (ускоряет массовую вставку)
    void reserve(size_t n)
    {
        pool_.reserve(n);
        ptrs_.reserve(n);
    }

    // Заинтернить строку и вернуть стабильный указатель (один и тот же для одинаковых текстов)
    const char *intern(std::string_view s)
    {
        // Одним движением: если строки нет — добавит; если есть — вернёт существующую.
        auto [it, inserted] = pool_.emplace(s); // s сконструирует временный std::string
        const char *p = it->c_str();
        ptrs_.insert(p); // (необязательный дебаг-реестр указателей)
        return p;
    }

    // Сколько уникальных строк хранится
    size_t size() const noexcept { return pool_.size(); }

    // Debug-страж: проверить, что указатель действительно из интернера
    bool is_interned(const char *p) const noexcept
    {
        return ptrs_.find(p) != ptrs_.end();
    }

private:
    // Важное свойство: адреса строк стабильны на весь срок жизни Interner (мы их не мутируем)
    std::unordered_set<std::string, StringHash, StringEq> pool_;
    std::unordered_set<const char *, PtrHash, PtrEq> ptrs_;
    using VarMap   = std::unordered_map<const char*, double, PtrHash, PtrEq>;
using ArrMap   = std::unordered_map<const char*, std::vector<double>, PtrHash, PtrEq>;
};

// Глобальный доступ без ODR-проблем: единый инстанс на процесс
inline Interner &INTERN()
{
    static Interner I;
    return I;
}

inline std::unordered_set<const char *, PtrHash, PtrEq> &TINY()
{
    static std::unordered_set<const char *, PtrHash, PtrEq> S;
    return S;
}

// --- Набор "символов" (ключевые слова, операторы) ---
// После init() все поля указывают на интернированные строки
struct Symbols
{
    // Ключевые слова (дополни списком под свой язык)
    const char *VAR = nullptr, *SET = nullptr, *IF = nullptr, *ELSE = nullptr, *WHILE = nullptr,
               *PROC = nullptr, *RETURN = nullptr, *PRINT = nullptr, *PRINTLN = nullptr, *HALT = nullptr;

    // Односимвольные/многосимвольные операторы и служебные токены
    const char *LBRACE = nullptr, *RBRACE = nullptr, *LP = nullptr, *RP = nullptr,
               *LBRACKET = nullptr, *RBRACKET = nullptr, *SEMI = nullptr, *EQ = nullptr,
               *PLUS = nullptr, *MINUS = nullptr, *STAR = nullptr, *SLASH = nullptr,
               *EQEQ = nullptr, *NEQ = nullptr, *LEQ = nullptr, *GEQ = nullptr,
               *LT = nullptr, *GT = nullptr, *COMMA = nullptr, *QUOTE = nullptr, *NOT = nullptr, *CARET = nullptr, *PERCENT = nullptr;
    ;

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

// Глобальный доступ к символам (инициализировать один раз при старте)
inline Symbols &SYM()
{
    static Symbols S;
    return S;
}