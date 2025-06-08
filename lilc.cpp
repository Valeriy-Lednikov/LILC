#include "system.cpp"
extern "C"
{
#include "tinyexpr.h"
}
#include <iostream>
#include <vector>
#include <cstring>
#include <cctype>

const static char *oneCharOperators = "><{}();,+-*/^%=\0";

const static char *tiny_key[] = {
    "abs",
    "acos",
    "asin",
    "atan",
    "atan2",
    "ceil",
    "cos",
    "cosh",
    "exp",
    "fac",
    "floor",
    "ln",
    "log",
    "log10",
    "ncr",
    "npr",
    "pi",
    "pow",
    "sin",
    "sinh",
    "sqrt",
    "tan",
    "tanh",
    "\0",
};

const static char *keywords_op[] = {
    // Операторы
    "==",
    "!=",
    ">=",
    "<=",

    // Функции
    "abs",
    "acos",
    "asin",
    "atan",
    "atan2",
    "ceil",
    "cos",
    "cosh",
    "exp",
    "fac",
    "floor",
    "ln",
    "log",
    "log10",
    "ncr",
    "npr",
    "pi",
    "pow",
    "sin",
    "sinh",
    "sqrt",
    "tan",
    "tanh",

    // Языковые операторы
    "VAR",
    "SET",
    "IF",
    "ELSE",
    "WHILE",
    "PRINTLN",
    "PRINT",
    "\0"

};

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

    bool isKeyword(const char *word)
    {
        for (int i = 0; keywords_op[i][0] != '\0'; ++i)
        {
            if (strcmp(word, keywords_op[i]) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool isOneCharOperator(char c)
    {
        for (int i = 0; oneCharOperators[i] != '\0'; ++i)
        {
            if (c == oneCharOperators[i])
            {
                return true;
            }
        }
        return false;
    }

    bool isKeywordFull(const char *word)
    {
        for (int i = 0; keywords_op[i][0] != '\0'; ++i)
        {
            if (strcmp(word, keywords_op[i]) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool isKeywordPrefix(const char *word)
    {
        size_t len = strlen(word);
        for (int i = 0; keywords_op[i][0] != '\0'; ++i)
        {
            if (strncmp(word, keywords_op[i], len) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool isTinyKey(const char *word)
    {
        // tiny_key объявлен где-то в том же файле или во внешней области видимости
        for (int i = 0; tiny_key[i][0] != '\0'; ++i)
        {
            if (std::strcmp(word, tiny_key[i]) == 0)
                return true;
        }
        return false;
    }

public:
    ~lilc()
    {
        // if (program)
        //     delete[] program;
        // for (auto word : words)
        // {
        //     delete[] word;
        // }
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

    int foundNextWord(const char *word) const
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

    int foundPrevWord(const char *word) const
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
            printError("gotoWord: invalid word index\n");
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

        if (startWord < 0 || endWord >= static_cast<int>(words.size()) || startWord > endWord)
        {
            printError("Invalid expression range\n");
            return nullptr;
        }

        // Ориентировочный расчёт необходимого размера
        size_t totalLength = 0;
        for (int i = startWord; i <= endWord; ++i)
            totalLength += std::strlen(words[i]) + 32;

        expressionBuffer = new char[totalLength + 1];
        char *ptr = expressionBuffer;

        for (int i = startWord; i <= endWord; ++i)
        {
            const char *word = words[i];
            size_t len = std::strlen(word);

            // 1) Функции tiny_key
            if (isTinyKey(word))
            {
                std::memcpy(ptr, word, len);
                ptr += len;
            }
            // 2) Языковые ключевые слова (VAR, IF, ELSE, PRINTLN и т.п.)
            else if (isKeyword(word))
            {
                std::memcpy(ptr, word, len);
                ptr += len;
            }
            // 3) Однобуквенные операторы и знаки
            else if (len == 1 && isOneCharOperator(word[0]))
            {
                *ptr++ = word[0];
            }
            // 4) Всё остальное — переменная: подставляем её значение
            else
            {
                double value;
                if (control.getVar(word, value))
                {
                    char valueStr[64];
                    std::snprintf(valueStr, sizeof(valueStr), "%g", value);
                    size_t vlen = std::strlen(valueStr);
                    std::memcpy(ptr, valueStr, vlen);
                    ptr += vlen;
                }
                else
                {
                    std::string er = "Variable '" + std::string(word) + "' not found";
                    printError(er.c_str());
                    halt();
                    return nullptr;
                }
            }
            // без пробелов между частями
        }

        *ptr = '\0';
        return expressionBuffer;
    }

    int foundCloseBrace()
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

    int foundCloseParenthes()
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
        int closeBrace = foundNextWord("}");
        currentWord = closeBrace;
    }

    void _opBREAK()
    {
        int closeBrace = foundNextWord("}");
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
                printError("VAR name not found\n", 1);
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
            printError("VAR name not found\n", 1);
            halt();
        }
        if (!varSet)
        {
            printError("VAR = no found\n");
            halt();
        }
        if (!valueStr)
        {
            printError("VAR value not found\n");
            halt();
        }
        if (!lineEnd || std::strcmp(lineEnd, ";") != 0)
        {
            printError("VAR \";\" not found\n", 4);
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
                printError("PRINT TEXT \" CLOSE not found", 3);
                halt();
            }
            if (!lineEnd || std::strcmp(lineEnd, ";") != 0)
            {
                printError("PRINT TEXT\";\" not found", 4);
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
            printError("PRINT VAR name not found", 1);
            halt();
        }
        if (!lineEnd || std::strcmp(lineEnd, ";") != 0)
        {
            printError("PRINT VAR \";\" not found", 2);
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
            printError("PRINT VAR variable name not found");
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
            printError("SET name not found\n");
            halt();
        }
        if (!varSet)
        {
            printError("SET = no found\n");
            halt();
        }

        if (std::strcmp(islineEnd, ";") == 0)
        {
            const char *valueStr = getWord(3);
            if (!valueStr)
            {
                printError("SET value not found\n");
                halt();
            }
            double value = std::atof(valueStr);
            if (!control.setVar(varName, value))
            {
                printError("SET name not found", 1);
            }
            currentWord += 4;
            return;
        }
        else
        {
            int lineEndI = foundNextWord(";");
            double result = _fnEval(currentWord + 3, lineEndI - 1);
            if (!control.setVar(varName, result))
            {
                printError("SET name not found", 1);
            }
            currentWord = lineEndI;
            return;
        }
    }

    void _opIF()
    {
        int closeParenthes = foundCloseParenthes();
        int openBrace = foundNextWord("{");
        int closeBrace = foundCloseBrace();
        const char *openParenthes = getWord(1);

        // std::cout << "close Parenthes " << closeParenthes << " openBrace " << openBrace << " closeBrace " << closeBrace << "\n";

        if (!openParenthes || std::strcmp(openParenthes, "(") != 0)
        {
            printError("IF \"(\" not found", 1);
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
        int closeBrace = foundCloseBrace();
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
        int closeParenthes = foundCloseParenthes();
        int openBrace = foundNextWord("{");
        int closeBrace = foundCloseBrace();
        const char *openParenthes = getWord(1);

        if (!openParenthes || std::strcmp(openParenthes, "(") != 0)
        {
            printError("WHILE \"(\" not found", 1);
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
                int closeELSE = foundCloseBrace();
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
        char buffer[512];
        int bufIndex = 0;

        auto flushBuffer = [&]()
        {
            if (bufIndex > 0)
            {
                buffer[bufIndex] = '\0';
                words.push_back(strdup(buffer));
                bufIndex = 0;
            }
        };

        while (*program)
        {
            if (*program == '"')
            {
                flushBuffer();
                words.push_back(strdup("\""));
                ++program;
                char strBuffer[512];
                int strIndex = 0;
                while (*program && *program != '"' && strIndex < 511)
                {
                    strBuffer[strIndex++] = *program++;
                }
                strBuffer[strIndex] = '\0';
                words.push_back(strdup(strBuffer));
                if (*program == '"')
                {
                    words.push_back(strdup("\""));
                    ++program;
                }
            }
            else if (isspace(*program))
            {
                flushBuffer();
                ++program;
            }
            else if (isOneCharOperator(*program))
            {
                flushBuffer();
                char op[2] = {*program, '\0'};
                words.push_back(strdup(op));
                ++program;
            }
            else
            {
                // Ловушка: мы должны аккуратно добавить символ, если не конец строки
                if (bufIndex < sizeof(buffer) - 1) // чтобы не вылезти за предел
                {
                    buffer[bufIndex++] = *program;
                }
                ++program;
            }
        }
        flushBuffer();
    }
};
