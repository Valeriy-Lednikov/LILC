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

class controller
{
private:
    int currentLevel = 0;
    std::vector<std::vector<variable>> vars = {{}};
    
public:
    void addVar(const char *name) {
        vars[currentLevel].emplace_back(name, 0.0);
    }

    void addVar(const char *name, double value) {
        vars[currentLevel].emplace_back(name, value);
    }

    void inLevel() {
        currentLevel++;
        if (vars.size() <= static_cast<size_t>(currentLevel)) {
            vars.push_back({});
        }
    }

    void outLevel() {
        if (currentLevel > 0) {
            vars.pop_back(); 
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

};