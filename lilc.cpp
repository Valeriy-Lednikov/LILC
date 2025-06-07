#include "system.cpp"
extern "C"
{
#include "tinyexpr.h"
}
#include <iostream>
#include <vector>
#include <cstring>
#include <cctype>

class lilc
{
private:
    char *program = nullptr;
    char *expressionBuffer = nullptr; // Буфер для результата выражения

    std::vector<char *> words;
    int currentWord = 0;
    bool isHalted = false;

    controller control; // экземпляр контроллера для переменных

    enum DeepType
    {
        FREE = 0, // пустая вложенность {}
        IF = 1,
        ELSE = 2,
        WHILE = 3,
        FUNK = 4 // функция (пока отложим)
    };

    struct DeepCode
    {
        DeepType type;
        int EXPRstart = -1; // выражение условия (например, условие if/while)
        int EXPRend = -1;   // выражение условия (например, условие if/while)
        int INword = -1;    // номер слова открытия {
        int OUTword = -1;   // номер слова закрытия }
    };

    std::vector<DeepCode> deepStack; // Стек вложенности

public:
    ~lilc()
    {
        if (program)
            delete[] program;
        for (auto word : words)
        {
            delete[] word;
        }
    }

    void printDeepStack() const
    {
        std::cout << "Deep Stack (size = " << deepStack.size() << "):\n";
        for (size_t i = 0; i < deepStack.size(); ++i)
        {
            const DeepCode &dc = deepStack[i];

            const char *typeStr = nullptr;
            switch (dc.type)
            {
            case FREE:
                typeStr = "FREE";
                break;
            case IF:
                typeStr = "IF";
                break;
            case ELSE:
                typeStr = "ELSE";
                break;
            case WHILE:
                typeStr = "WHILE";
                break;
            case FUNK:
                typeStr = "FUNK";
                break;
            default:
                typeStr = "UNKNOWN";
                break;
            }

            std::cout << "  [" << i << "] Type: " << typeStr
                      << ", INword: " << dc.INword
                      << ", OUTword: " << dc.OUTword;

            if (dc.type == IF || dc.type == WHILE) // У IF и WHILE есть выражение
            {
                std::cout << ", EXPRstart: " << dc.EXPRstart
                          << ", EXPRend: " << dc.EXPRend;
            }

            std::cout << "\n";
        }
    }

    void loadProgram(const char *prog)
    {
        if (program)
            delete[] program;
        for (auto word : words)
        {
            delete[] word;
        }
        words.clear();

        program = new char[strlen(prog) + 1];
        std::strcpy(program, prog);

        parseProgram();
        currentWord = 0;
        isHalted = false;
        control = controller(); // создаём новый контроллер
    }

    const char *getWord(int i) const
    {
        int index = currentWord + i;
        if (index >= 0 && index < static_cast<int>(words.size()))
        {
            return words[index];
        }
        else
        {
            return nullptr;
        }
    }

    const char *getWordGlob(int i)
    {
        int index = i;
        if (index >= 0 && index < static_cast<int>(words.size()))
        {
            return words[index];
        }
        else
        {
            return nullptr;
        }
    }

    void nextWord()
    {
        if (currentWord < static_cast<int>(words.size()) - 1)
        {
            ++currentWord;
        }
        else
        {
            halt();
        }
    }

    int findNextWord(const char *word) const
    {
        for (int i = currentWord + 1; i < static_cast<int>(words.size()); ++i)
        {
            if (std::strcmp(words[i], word) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    int findPrevWord(const char *word) const
    {
        for (int i = currentWord - 1; i >= 0; --i)
        {
            if (std::strcmp(words[i], word) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    void gotoWord(int wordIndex)
    {
        if (wordIndex >= 0 && wordIndex < static_cast<int>(words.size()))
        {
            currentWord = wordIndex;
        }
        else
        {
            std::cerr << "gotoWord: invalid word index\n";
            halt();
        }
    }

    const char *getExpression(int startWord, int endWord)
    {
        // Очистим старый буфер, если был
        if (expressionBuffer)
        {
            delete[] expressionBuffer;
            expressionBuffer = nullptr;
        }

        if (startWord < 0 || endWord >= (int)words.size() || startWord > endWord)
        {
            std::cerr << "Invalid expression range\n";
            return nullptr;
        }

        // Оцениваем грубо размер (переменные максимум увеличатся до 32 знаков)
        size_t totalLength = 0;
        for (int i = startWord; i <= endWord; ++i)
        {
            totalLength += std::strlen(words[i]) + 32; // запас под замену переменной на число
            // Убираем подсчет пробелов! Пробелов между словами не будет
        }

        expressionBuffer = new char[totalLength + 1];
        char *ptr = expressionBuffer;

        for (int i = startWord; i <= endWord; ++i)
        {
            const char *word = words[i];
            if (word[0] == '#')
            {
                // Это переменная, убираем #
                const char *varName = word + 1;

                double value;
                if (control.getVar(varName, value))
                {
                    // Переводим double в строку
                    char valueStr[64];
                    std::snprintf(valueStr, sizeof(valueStr), "%g", value); // Компактный вывод double
                    size_t len = std::strlen(valueStr);
                    std::memcpy(ptr, valueStr, len);
                    ptr += len;
                }
                else
                {
                    std::cerr << "Variable " << varName << " not found.\n";
                    halt();
                    return nullptr;
                }
            }
            else
            {
                // Обычное слово
                size_t len = std::strlen(word);
                std::memcpy(ptr, word, len);
                ptr += len;
            }

            // Убираем добавление пробела!
        }
        *ptr = '\0'; // Завершаем строку

        // std::cout << "Expr " << expressionBuffer << "\n"; // Отладочный вывод
        return expressionBuffer;
    }

    int findCloseBrace()
    {
        int level = 0;
        {
            for (int i = currentWord + 1; i < (int)words.size(); ++i)
            {
                if (std::strcmp(words[i], "{") == 0)
                {
                    level++;
                }
                else if (std::strcmp(words[i], "}") == 0)
                {
                    level--;
                    if (level == 0)
                    {
                        return i; // нашли нужную закрывающую скобку
                    }
                }
            }
            // Если дошли сюда — не нашли
            printError("Closing } not found\n");
            halt();

            return -1;
        }
    }

    int findCloseParenthes()
    {
        int level = 0; // мы уже внутри первой {
        for (int i = currentWord + 1; i < (int)words.size(); ++i)
        {
            if (std::strcmp(words[i], "(") == 0)
            {
                level++;
            }
            else if (std::strcmp(words[i], ")") == 0)
            {
                level--;
                if (level == 0)
                {
                    return i; // нашли нужную закрывающую скобку
                }
            }
        }
        // Если дошли сюда — не нашли
        printError("Closing ) not found\n");
        halt();

        return -1;
    }

    void halt()
    {
        isHalted = true;
    }

    void _opCONTINUE()
    {
        int closeBrace = findNextWord("}");
        currentWord = closeBrace;
    }

    void _opBREAK()
    {
        int closeBrace = findNextWord("}");
        currentWord = closeBrace + 1;
        control.outLevel();
    }

    void _opCreateVar()
    {
        const char *islineEnd = getWord(2);
        if (std::strcmp(islineEnd, ";") == 0)
        {
            const char *varName = getWord(1);
            if (!varName)
            { // Добавить условие корректности названия
                  printError("VAR name not find\n", 1);
                halt();
            }
            if (!control.setVar(varName, 0))
            {
                control.addVar(varName, 0); // создаём новую переменную
            }
            currentWord += 2;
            return;
        }

        const char *varName = getWord(1);
        const char *varSet = getWord(2);
        const char *valueStr = getWord(3);
        const char *lineEnd = getWord(4);
        if (!varName)
        { // Добавить условие корректности названия
            printError("VAR name not find\n", 1);
            halt();
        }
        if (!varSet)
        {
            std::cerr << "VAR = no find\n";
            halt();
        }
        if (!valueStr)
        {
            std::cerr << "VAR value not find\n";
            halt();
        }
        if (!lineEnd || std::strcmp(lineEnd, ";") != 0)
        {
            printError("VAR \";\" not find\n",4);
            halt();
        }
        double value = std::atof(valueStr);
        if (!control.setVar(varName, value))
        {
            control.addVar(varName, value); // создаём новую переменную
        }
        currentWord += 4;
    }

    void _opPrint(bool ln = 0)
    {
        const char *isTextOpen = getWord(1);
        if (std::strcmp(isTextOpen, "\"") == 0)
        {
            const char *text = getWord(2);
            const char *isTextClose = getWord(3);
            const char *lineEnd = getWord(4);
            if (!isTextClose || std::strcmp(isTextClose, "\"") != 0)
            {
                printError("PRINT TEXT \" CLOSE not find", 3);
                halt();
            }
            if (!lineEnd || std::strcmp(lineEnd, ";") != 0)
            {
                printError("PRINT TEXT\";\" not find", 4);
                halt();
            }

            if (ln)
                std::cout << text << std::endl;
            else
                std::cout << text;
            currentWord += 4;
            return;
        }

        const char *varName = getWord(1);
        const char *lineEnd = getWord(2);
        if (!varName)
        { // Добавить условие корректности названия
            printError("PRINT VAR name not find", 1);
            halt();
        }
        if (!lineEnd || std::strcmp(lineEnd, ";") != 0)
        {
            printError("PRINT VAR \";\" not find", 2);
            halt();
        }
        double value;
        if (control.getVar(varName, value))
        {
            if (ln)
                std::cout << value << std::endl;
            else
                std::cout << value;
        }
        else
        {
            printError("PRINT VAR variable name not find");
        }
        currentWord += 3;
    }

    double _fnEval(int startWord, int endWord)
    {
        const char *expr = getExpression(startWord, endWord);
        if (!expr)
        {
            halt();
            return 0;
        }

        // Поиск оператора
        const char *ops[] = {"!=", "==", ">=", "<=", ">", "<"};
        const char *foundOp = nullptr;
        int foundOpLen = 0;
        const char *p = expr; // <<< Вынесли p сюда!

        for (; *p; ++p)
        {
            for (int i = 0; i < 6; ++i)
            {
                int len = std::strlen(ops[i]);
                if (std::strncmp(p, ops[i], len) == 0)
                {
                    foundOp = ops[i];
                    foundOpLen = len;
                    goto operator_found;
                }
            }
        }

    operator_found:

        if (foundOp)
        {
            int pos = p - expr; // ← теперь p доступна тут!

            int exprLen = std::strlen(expr);

            // Левое выражение
            char *exprLeft = new char[pos + 1];
            std::strncpy(exprLeft, expr, pos);
            exprLeft[pos] = '\0';

            // Правое выражение
            const char *rightStart = p + foundOpLen;
            int rightLen = exprLen - pos - foundOpLen;
            char *exprRight = new char[rightLen + 1];
            std::strncpy(exprRight, rightStart, rightLen);
            exprRight[rightLen] = '\0';

            // Вычисляем
            double leftVal = te_interp(exprLeft, 0);
            double rightVal = te_interp(exprRight, 0);

            delete[] exprLeft;
            delete[] exprRight;

            // Сравнение
            if (std::strcmp(foundOp, "==") == 0)
            {
                return leftVal == rightVal;
            }
            else if (std::strcmp(foundOp, "!=") == 0)
            {
                return leftVal != rightVal;
            }
            else if (std::strcmp(foundOp, ">") == 0)
            {
                return leftVal > rightVal;
            }
            else if (std::strcmp(foundOp, "<") == 0)
            {
                return leftVal < rightVal;
            }
            else if (std::strcmp(foundOp, ">=") == 0)
            {
                return leftVal >= rightVal;
            }
            else if (std::strcmp(foundOp, "<=") == 0)
            {
                return leftVal <= rightVal;
            }
        }

        // Если оператор не найден
        return te_interp(expr, 0);
    }

    void _opSet()
    {
        const char *islineEnd = getWord(4);
        const char *varName = getWord(1);
        const char *varSet = getWord(2);

        if (!varName)
        { // Добавить условие корректности названия
            std::cerr << "SET name not find\n";
            halt();
        }
        if (!varSet)
        {
            std::cerr << "SET = no find\n";
            halt();
        }

        if (std::strcmp(islineEnd, ";") == 0)
        {
            const char *valueStr = getWord(3);
            if (!valueStr)
            {
                std::cerr << "SET value not find\n";
                halt();
            }
            double value = std::atof(valueStr);
            if (!control.setVar(varName, value))
            {
                printError("SET name not find", 1);
            }
            currentWord += 4;
            return;
        }
        else
        {
            int lineEndI = findNextWord(";");
            double result = _fnEval(currentWord + 3, lineEndI - 1);
            if (!control.setVar(varName, result))
            {
                printError("SET name not find", 1);
            }
            currentWord = lineEndI;
            return;
        }
    }

    void _opIF()
    {
        int closeParenthes = findCloseParenthes();
        int openBrace = findNextWord("{");
        int closeBrace = findCloseBrace();
        const char *openParenthes = getWord(1);

        // std::cout << "close Parenthes " << closeParenthes << " openBrace " << openBrace << " closeBrace " << closeBrace << "\n";

        if (!openParenthes || std::strcmp(openParenthes, "(") != 0)
        {
            printError("IF \"(\" not find", 1);
            halt();
        }
        double result = _fnEval(currentWord + 2, closeParenthes - 1);
        // std::cout << "result = " << result << "\n";
        if (result == 1 || result > 1)
        {
            DeepCode stack;
            stack.INword = openBrace;
            stack.OUTword = closeBrace;
            stack.type = DeepType::IF;
            deepStack.push_back(stack);

            currentWord = openBrace + 1;
            control.inLevel();
            return;
        }
        else if (result == 0)
        {
            currentWord = closeBrace + 1;
            // const char *word = getWord(0);
            // if (std::strcmp(word, "ELSE") == 0){
            //     _opELSE();
            // }
            // else{
            //     deepStack.pop_back();
            // }
            return;
        }
        else
        {
            printError("IF expression not return 0/1", 1);
            halt();
        }
    }

    void _opELSE()
    {
        const char *openBrace = getWord(1);
        if (!openBrace || std::strcmp(openBrace, "{") != 0)
        {
            printError("ELSE \"{\" not found", 1);
            halt();
        }
        int closeBrace = findCloseBrace();
        DeepCode stack;
        stack.INword = currentWord + 1;
        stack.OUTword = closeBrace;
        stack.type = DeepType::ELSE;
        deepStack.push_back(stack);

        currentWord = currentWord + 2;
        control.inLevel();
    }

    void _opWHILE()
    {
        int closeParenthes = findCloseParenthes();
        int openBrace = findNextWord("{");
        int closeBrace = findCloseBrace();
        const char *openParenthes = getWord(1);

        if (!openParenthes || std::strcmp(openParenthes, "(") != 0)
        {
            printError("WHILE \"(\" not find", 1);
            halt();
        }
        double result = _fnEval(currentWord + 2, closeParenthes - 1);
        // std::cout << "result = " << result << "\n";
        if (result == 1 || result > 1)
        {
            DeepCode stack;
            stack.INword = openBrace;
            stack.OUTword = closeBrace;
            stack.type = DeepType::WHILE;
            stack.EXPRstart = currentWord + 2;
            stack.EXPRend = closeParenthes - 1;
            deepStack.push_back(stack);

            currentWord = openBrace + 1;
            control.inLevel();
            return;
        }
        else if (result == 0)
        {
            currentWord = closeBrace + 1;
            return;
        }
        else
        {
            printError("WHILE expression not return 0/1", 1);
            halt();
        }
    }

    void _opCLOSEBRACE()
    {
        // std::cout << "last type " << deepStack[deepStack.size()-1].type << " size " << deepStack.size() << "\n";

        if (deepStack[deepStack.size() - 1].type == DeepType::WHILE)
        {
            if (_fnEval(deepStack[deepStack.size() - 1].EXPRstart, deepStack[deepStack.size() - 1].EXPRend))
            {
                currentWord = deepStack[deepStack.size() - 1].INword + 1;
                return;
            }
            else
            {
                deepStack.pop_back();
                currentWord++;
                return;
            }
        }

        if (deepStack[deepStack.size() - 1].type == DeepType::IF)
        {
            const char *word = getWord(1);
            // std::cout << "is else " << word << "\n";
            if (std::strcmp(word, "ELSE") == 0)
            {
                // std::cout << "IF ok, next ELSE\n";
                int closeELSE = findCloseBrace();
                currentWord = closeELSE + 1;
                deepStack.pop_back();
                control.outLevel();
                return;
            }
        }

        control.outLevel();
        deepStack.pop_back();
        currentWord++;
    }

    void tick()
    {
        if (isHalted)
        {
            return;
        }

        const char *word = getWord(0);
        if (!word)
        {
            halt();
            return;
        }

        // обработка команд
        if (std::strcmp(word, "VAR") == 0)
        {
            _opCreateVar();
        }
        else if (std::strcmp(word, "PRINT") == 0)
        {
            _opPrint();
        }
        else if (std::strcmp(word, "PRINTLN") == 0)
        {
            _opPrint(1);
        }
        else if (std::strcmp(word, "SET") == 0)
        {
            _opSet();
        }
        else if (std::strcmp(word, "IF") == 0)
        {
            _opIF();
        }
        else if (std::strcmp(word, "WHILE") == 0)
        {
            _opWHILE();
        }
        else if (std::strcmp(word, "}") == 0)
        {
            _opCLOSEBRACE();
        }
        else if (std::strcmp(word, "ELSE") == 0)
        {
            _opELSE();
        }
        else if (std::strcmp(word, ";") == 0)
        {
            nextWord();
        }
        else if (std::strcmp(word, "HALT") == 0)
        {
            halt();
        }
        else
        {
            printError("Unknown command");
            halt();
        }
    }

    void interpretate(int limit = -1)
    {
        int steps = 0;
        while (!isHalted)
        {
            tick();
            ++steps;
            if (limit != -1 && steps >= limit)
            {
                break;
            }
        }
    }

    void printWords() const
    {
        for (size_t i = 0; i < words.size(); ++i)
        {
            std::cout << i << ": " << words[i] << std::endl;
        }
    }

    void printError(const char *text, int word = 0)
    {
        std::cout << "ERROR in word <" << currentWord + word << "><" << words[currentWord + word] << ">" << " - " << text << "\n";
    }

private:
private:
    void parseProgram()
    {
        const char *p = program;
        while (*p)
        {
            // Пропустить пробельные символы
            while (std::isspace(*p))
                ++p;

            if (*p == '\0')
                break;

            if (*p == ';')
            {
                // Обработка точки с запятой как отдельного слова
                char *word = new char[2];
                word[0] = ';';
                word[1] = '\0';
                words.push_back(word);
                ++p;
            }
            else if (*p == '"')
            {
                // Строка в кавычках
                // Сохраняем открывающую кавычку как отдельное слово
                char *openQuote = new char[2];
                openQuote[0] = '"';
                openQuote[1] = '\0';
                words.push_back(openQuote);
                ++p; // Перемещаемся за открывающую кавычку

                const char *start = p;
                std::string strContent;

                while (*p)
                {
                    if (*p == '\\' && *(p + 1) == '"')
                    {
                        // Экранированная кавычка
                        strContent += '"';
                        p += 2;
                    }
                    else if (*p == '"')
                    {
                        // Найдена закрывающая кавычка
                        break;
                    }
                    else
                    {
                        strContent += *p;
                        ++p;
                    }
                }

                // Сохраняем содержимое строки
                if (!strContent.empty())
                {
                    char *innerWord = new char[strContent.size() + 1];
                    std::strcpy(innerWord, strContent.c_str());
                    words.push_back(innerWord);
                }

                if (*p == '"')
                {
                    // Сохраняем закрывающую кавычку как отдельное слово
                    char *closeQuote = new char[2];
                    closeQuote[0] = '"';
                    closeQuote[1] = '\0';
                    words.push_back(closeQuote);
                    ++p; // Перемещаемся за закрывающую кавычку
                }
                else
                {
                    // Нет закрывающей кавычки — ошибка или остановка, в данном случае пропустим
                    std::cerr << "Warning: string without closing quote\n";
                }
            }
            else
            {
                // Обычное слово (до пробела или ;)
                const char *start = p;
                while (*p && !std::isspace(*p) && *p != ';' && *p != '"')
                {
                    ++p;
                }

                size_t len = p - start;
                if (len > 0)
                {
                    char *word = new char[len + 1];
                    std::strncpy(word, start, len);
                    word[len] = '\0';
                    words.push_back(word);
                }

                // Если после слова идет ; или ", не забудем обработать их на следующем шаге
            }
        }
    }
};
