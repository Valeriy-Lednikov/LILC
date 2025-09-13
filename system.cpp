#include <iostream>
#include <vector>

#include <cstring> // Для strcpy, strlen

struct variable {
    char* name;
    double value;

    // Конструктор
    variable(const char* n, double v = 0.0) : value(v) {
        name = new char[strlen(n) + 1];
        std::strcpy(name, n);
    }

    // Деструктор
    ~variable() {
        delete[] name;
    }

    // Конструктор копирования
    variable(const variable& other) : value(other.value) {
        name = new char[strlen(other.name) + 1];
        std::strcpy(name, other.name);
    }

    // Оператор присваивания
    variable& operator=(const variable& other) {
        if (this != &other) {
            delete[] name;
            name = new char[strlen(other.name) + 1];
            std::strcpy(name, other.name);
            value = other.value;
        }
        return *this;
    }
};


struct array_var {
    char* name;
    std::vector<double> data;

    // Конструктор: массив фиксированного размера, заполненный init
    array_var(const char* n, size_t size = 0, double init = 0.0) : data(size, init) {
        name = new char[std::strlen(n) + 1];
        std::strcpy(name, n);
    }

    // Конструктор от готовых значений
    array_var(const char* n, const std::vector<double>& values) : data(values) {
        name = new char[std::strlen(n) + 1];
        std::strcpy(name, n);
    }

    // Деструктор
    ~array_var() {
        delete[] name;
    }

    // Конструктор копирования
    array_var(const array_var& other) : data(other.data) {
        name = new char[std::strlen(other.name) + 1];
        std::strcpy(name, other.name);
    }

    // Оператор присваивания
    array_var& operator=(const array_var& other) {
        if (this != &other) {
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
    void addVar(const char *name) {
        vars[currentLevel].emplace_back(name, 0.0);
    }

    void addVar(const char *name, double value) {
        vars[currentLevel].emplace_back(name, value);
    }

    void inLevel() {
        currentLevel++;
        if (vars.size()   <= static_cast<size_t>(currentLevel))   vars.push_back({});
        if (arrays.size() <= static_cast<size_t>(currentLevel)) arrays.push_back({});
    }

    void outLevel() {
        if (currentLevel > 0) {
            vars.pop_back();
            arrays.pop_back();   
            currentLevel--;
        } else {
            std::cerr << "Already at base level, can't go lower!" << std::endl;
        }
    }

    void printCurrentLevel()  {
        std::cout << "Level " << currentLevel << " variables:\n";
        for (const auto& var : vars[currentLevel]) {
            std::cout << var.name << " = " << var.value << std::endl;
        }
    }

    void printAllLevels()  {
    std::cout << "All levels:\n";
    for (size_t level = 0; level < vars.size(); ++level) {
        std::cout << "Level " << level << ":\n";
        for (const auto& var : vars[level]) {
            std::cout << "  " << var.name << " = " << var.value << "\n";
        }
    }
}

    bool setVar(const char* name, double value) {
        for (int level = currentLevel; level >= 0; --level) {
            for (auto& var : vars[level]) {
                if (std::strcmp(var.name, name) == 0) {
                    var.value = value;
                    return true;
                }
            }
        }
        return false; // Переменная не найдена
    }

    bool getVar(const char* name, double& outValue)  {
        for (int level = currentLevel; level >= 0; --level) {
            for (const auto& var : vars[level]) {
                if (std::strcmp(var.name, name) == 0) {
                    outValue = var.value;
                    return true;
                }
            }
        }
        return false; // Переменная не найдена
    }

    bool findVar(const char* name)  {
        for (int level = currentLevel; level >= 0; --level) {
            for (const auto& var : vars[level]) {
                if (std::strcmp(var.name, name) == 0) {
                    return true;
                }
            }
        }
        return false; // Переменная не найдена
    }

    const std::vector<variable>& getVarsAtLevel(int level) {
        return vars[level];
    }

    int getCurrentLevel() {
        return currentLevel;
    }



     // Создать массив заданного размера, заполненный нулями (или init)
    void addArray(const char* name, size_t size, double init = 0.0) {
        arrays[currentLevel].emplace_back(name, size, init);
    }

    // Создать массив из набора значений
    void addArray(const char* name, const std::vector<double>& values) {
        arrays[currentLevel].emplace_back(name, values);
    }

    // Найти массив по имени (с учетом вложенных уровней)
    bool findArray(const char* name) {
        for (int level = currentLevel; level >= 0; --level) {
            for (const auto& arr : arrays[level]) {
                if (std::strcmp(arr.name, name) == 0) return true;
            }
        }
        return false;
    }

    // Установить элемент массива по индексу
    bool setArrayElem(const char* name, size_t index, double value) {
        for (int level = currentLevel; level >= 0; --level) {
            for (auto& arr : arrays[level]) {
                if (std::strcmp(arr.name, name) == 0) {
                    if (index < arr.data.size()) {
                        arr.data[index] = value;
                        return true;
                    } else {
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
    bool setArrayElem(const char* name, int index, double value) {
        if (index < 0) {
            std::cerr << "Negative index for array '" << name << "': " << index << "\n";
            return false;
        }
        return setArrayElem(name, static_cast<size_t>(index), value);
    }

    // Прочитать элемент массива по индексу
    bool getArrayElem(const char* name, size_t index, double& outValue) {
        for (int level = currentLevel; level >= 0; --level) {
            for (const auto& arr : arrays[level]) {
                if (std::strcmp(arr.name, name) == 0) {
                    if (index < arr.data.size()) {
                        outValue = arr.data[index];
                        return true;
                    } else {
                        std::cerr << "Index out of bounds for array '" << name
                                  << "': " << index << " >= " << arr.data.size() << "\n";
                        return false;
                    }
                }
            }
        }
        return false; // Массив не найден
    }

    bool getArrayElem(const char* name, int index, double& outValue) {
        if (index < 0) {
            std::cerr << "Negative index for array '" << name << "': " << index << "\n";
            return false;
        }
        return getArrayElem(name, static_cast<size_t>(index), outValue);
    }

    // Изменить размер массива (заполнить init)
    bool resizeArray(const char* name, size_t newSize, double init = 0.0) {
        for (int level = currentLevel; level >= 0; --level) {
            for (auto& arr : arrays[level]) {
                if (std::strcmp(arr.name, name) == 0) {
                    arr.resize(newSize, init);
                    return true;
                }
            }
        }
        return false; // Массив не найден
    }

    // Получить массивы на уровне (например, для отладочной печати UI)
    const std::vector<array_var>& getArraysAtLevel(int level) {
        return arrays[level];
    }

};