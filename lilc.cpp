#include "system.cpp"
extern "C"
{
#include "tinyexpr.h"
}
#include <iostream>
#include <vector>
#include <cstring>
#include <cctype>
#include <functional>
#include <string>
#include <iomanip>

const static char *oneCharOperators = "[]><{}();,+-*/^%=\0";

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
    "FN",
    "PROC",
    "RETURN"
    "WHILE",
    "PRINTLN",
    "PRINT",
    "\0"

};

class lilc
{
private:
    const Symbols *S = nullptr;
    char *program = nullptr;
    char *expressionBuffer = nullptr; // Буфер для результата выражения

    std::vector<const char *> words;
    int currentWord = 0;

    controller control; // экземпляр контроллера для переменных

    enum DeepType
    {
        FREE = 0, // пустая вложенность {}
        IF = 1,
        ELSE = 2,
        WHILE = 3,
        PROC = 4 // функция (пока отложим)
    };

    struct DeepCode
    {
        DeepType type;
        int EXPRstart = -1; // выражение условия (например, условие if/while)
        int EXPRend = -1;   // выражение условия (например, условие if/while)
        int INword = -1;    // номер слова открытия {
        int OUTword = -1;   // номер слова закрытия }
        int RETword = -1;   // слово на которое нужно вернуться после выхода из }

        // fast while condition (optional)
        bool fastCond = false;
        enum CondOp
        {
            OP_LT,
            OP_LE,
            OP_GT,
            OP_GE,
            OP_EQ,
            OP_NE
        } condOp;
        const char *condVarId = nullptr; // интернированное имя переменной из условия
        double condCst = 0.0;            // правая константа
    };

    std::vector<DeepCode> deepStack; // Стек вложенности

    inline bool isExprToken(const char *w)
    {
        return w == S->PLUS || w == S->MINUS || w == S->STAR || w == S->SLASH ||
               w == S->LP || w == S->RP || w == S->COMMA ||
               w == S->EQEQ || w == S->NEQ || w == S->LEQ || w == S->GEQ ||
               w == S->LT || w == S->GT ||
               w == S->CARET || w == S->PERCENT;
    }

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

    inline bool isTinyKey(const char *w)
    {
        return TINY().find(w) != TINY().end();
    }

    bool compareChar(const char *str1, const char *str2)
    {
        char c1 = str1[0];
        int code1 = static_cast<int>(c1);
        char c2 = str2[0];
        int code2 = static_cast<int>(c2);
        if (code1 == code2)
        {
            return true;
        }
        return false;
    }

    bool isNumber(const char *s)
    {
        if (!s || *s == '\0')
            return false; // пустая строка
        for (const char *p = s; *p; ++p)
        {
            if (!std::isdigit(*p))
                return false;
        }
        return true;
    }

public:
    bool isHalted = false;
    std::function<void(const std::string &)> printOut;

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
            case PROC:
                typeStr = "PROC";
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
        static bool syms_inited = false;
        if (!syms_inited)
        {
            SYM().init(INTERN());
            syms_inited = true;
        }
        S = &SYM();
        if (program)
            delete[] program;

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

    int foundNextWord(const char *tok) const
    {
        for (int i = currentWord + 1; i < (int)words.size(); ++i)
            if (words[i] == tok)
                return i;
        return -1;
    }

    // int foundNextWord(const char *word) const
    // {
    //     word = INTERN().intern(word);
    //     for (int i = currentWord + 1; i < static_cast<int>(words.size()); ++i)
    //     {
    //         if (words[i] == word)
    //         {
    //             return i;
    //         }
    //     }
    //     return -1;
    // }

    int foundPrevWord(const char *word) const
    {
        word = INTERN().intern(word);
        for (int i = currentWord - 1; i >= 0; --i)
        {
            if (words[i] == word)
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

    int findPROC(const char *name)
    {
        for (int i = 1; i < (int)words.size(); ++i)
        {
            if (words[i] == name && words[i - 1] == S->PROC)
                return i;
        }

        return -1;
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
            // 2) Ключевые слова языка
            else if (isExprToken(word))
            {
                std::memcpy(ptr, word, len);
                ptr += len;
            }
            // 3) Однобуквенные операторы
            else if (len == 1 && isOneCharOperator(word[0]))
            {
                *ptr++ = word[0];
            }
            // 4) Число: если начинается с цифры
            else if (std::isdigit(static_cast<unsigned char>(word[0])))
            {
                std::memcpy(ptr, word, len);
                ptr += len;
            }
            // 5) Обращение к массиву:  name [ index ]
            else if (i + 3 <= endWord && words[i + 1] == S->LBRACKET && words[i + 3] == S->RBRACKET)
            {
                const char *arrName = word;
                const char *idxTok = words[i + 2];

                // Разбираем индекс: число или переменная
                size_t idx = 0;
                bool idxOk = false;

                if (std::isdigit(static_cast<unsigned char>(idxTok[0])))
                {
                    // Убедимся, что весь токен — цифры, и аккуратно соберём индекс
                    idxOk = true;
                    for (const char *p = idxTok; *p; ++p)
                    {
                        if (!std::isdigit(static_cast<unsigned char>(*p)))
                        {
                            idxOk = false;
                            break;
                        }
                        idx = idx * 10 + static_cast<size_t>(*p - '0');
                    }
                    if (!idxOk)
                    {
                        std::string er = "Invalid array index token '" + std::string(idxTok) + "'";
                        printError(er.c_str());
                        halt();
                        return nullptr;
                    }
                }
                else
                {
                    // Индекс — имя переменной
                    double idxVal = 0.0;
                    if (!control.getVar(idxTok, idxVal))
                    {
                        std::string er = "Variable '" + std::string(idxTok) + "' not found";
                        printError(er.c_str());
                        halt();
                        return nullptr;
                    }
                    if (idxVal < 0)
                    {
                        printError("Array index must be >= 0");
                        halt();
                        return nullptr;
                    }
                    idx = static_cast<size_t>(idxVal);
                    idxOk = true;
                }

                double elemValue = 0.0;
                if (!control.getArrayElem(arrName, idx, elemValue))
                {
                    std::string er = "Array element '" + std::string(arrName) + "[" + std::to_string(idx) + "]' not found";
                    printError(er.c_str());
                    halt();
                    return nullptr;
                }

                char valueStr[64];
                std::snprintf(valueStr, sizeof(valueStr), "%g", elemValue);
                size_t vlen = std::strlen(valueStr);
                std::memcpy(ptr, valueStr, vlen);
                ptr += vlen;

                // Пропускаем [, index, ]
                i += 3;
            }
            // 6) Остальное — переменная
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
        for (int i = currentWord + 1; i < (int)words.size(); ++i)
        {
            if (words[i] == S->LBRACE)
                ++level;
            else if (words[i] == S->RBRACE)
            {
                --level;
                if (level == 0)
                    return i;
            }
        }
        printError("Closing } not found\n");
        halt();
        return -1;
    }

    int foundCloseParenthes()
    {
        int level = 0; // мы уже внутри первой {
        for (int i = currentWord + 1; i < (int)words.size(); ++i)
        {
            if (words[i] == S->LP)
            {
                level++;
            }
            else if (words[i] == S->RP)
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
        int closeBrace = foundNextWord(S->RBRACE);
        currentWord = closeBrace;
    }

    void _opBREAK()
    {
        int closeBrace = foundNextWord(S->RBRACE);
        currentWord = closeBrace + 1;
        control.outLevel();
    }

    void _opCreateVar()
    {
        const char *islineEnd = getWord(2);
        if (islineEnd == S->SEMI)
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
        const char *openSquare = getWord(2);
        const char *closeSquare = getWord(4);
        const char *lineEnd = getWord(5);
        if (!varName)
        { // Добавить условие корректности названия
            printError("VAR name not found\n", 1);
            halt();
        }
        if (openSquare == S->LBRACKET)
        {
            if (closeSquare == S->RBRACKET)
            {
                if (lineEnd == S->SEMI)
                {
                    control.addArray(varName, std::stoi(getWord(3)));
                    currentWord += 5;
                    return;
                }
            }
        }

        const char *varSet = getWord(2);
        const char *valueStr = getWord(3);
        const char *lineEnd2 = getWord(4);
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
        if (!lineEnd2 || lineEnd2 != S->SEMI)
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
        if (isTextOpen == S->QUOTE)
        {
            const char *text = getWord(2);
            const char *isTextClose = getWord(3);
            const char *lineEnd = getWord(4);
            if (!isTextClose || isTextClose != S->QUOTE)
            {
                printError("PRINT TEXT \" CLOSE not found", 3);
                halt();
            }
            if (!lineEnd || lineEnd != S->SEMI)
            {
                printError("PRINT TEXT\";\" not found", 4);
                halt();
            }

            if (ln)
            {
                if (printOut)
                {
                    std::string t = std::string(text) + "\n";
                    printOut(t);
                }
                std::cout << text << std::endl;
            }
            else
            {
                if (printOut)
                {
                    printOut(text);
                }
                std::cout << text;
            }
            currentWord += 4;
            return;
        }

        double value;

        bool isArray = (getWord(2) == S->LBRACKET) && (getWord(4) == S->RBRACKET);

        const char *varName = getWord(1);
        const char *lineEnd = getWord(2);
        if (!varName)
        { // Добавить условие корректности названия
            printError("PRINT VAR name not found", 1);
            halt();
        }
        // if ((!lineEnd || std::strcmp1(lineEnd, ";") != 0) && index == -1)
        // {
        //     printError("PRINT VAR \";\" not found", 2);
        //     halt();
        // }
        if (isArray == false)
        {
            if (!control.getVar(varName, value))
            {

                printError("PRINT VAR variable name not found");
                halt();
            }
        }
        else
        {

            double index = -1;
            if (isNumber(getWord(3)))
            {
                index = std::stoi(getWord(3));
            }
            else
            {
                control.getVar(getWord(3), index);
            }

            if (!control.getArrayElem(varName, int(index), value))
            {
                printError("PRINT VAR array name not found");
                halt();
            }
        }

        if (ln)
        {
            if (printOut)
            {
                std::string t = std::to_string(value) + "\n";
                printOut(t);
            }
            std::cout << std::fixed << std::setprecision(15) << value << std::endl;
        }
        else
        {
            if (printOut)
            {
                printOut(std::to_string(value));
            }
            std::cout << std::fixed << std::setprecision(15) << value;
        }
        if (!isArray)
        {
            currentWord += 3;
        }
        else
        {
            currentWord += 5;
        }
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
        int opIndex = -1;
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
                    opIndex = i; // <— запомнили какой именно оператор нашли
                    goto operator_found;
                }
            }
        }

    operator_found:

        if (foundOp)
        {
            int pos = int(p - expr);
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
            switch (opIndex)
            {
            case 1:
                return leftVal == rightVal; // "=="
            case 0:
                return leftVal != rightVal; // "!="
            case 4:
                return leftVal > rightVal; // ">"
            case 5:
                return leftVal < rightVal; // "<"
            case 2:
                return leftVal >= rightVal; // ">="
            case 3:
                return leftVal <= rightVal; // "<="
            }
        }

        // Если оператор не найден
        return te_interp(expr, 0);
    }

    void _opSet()
    {
        // Имя переменной обязательно
        const char *name = getWord(0);
        if (!name)
        {
            printError("SET name not found\n");
            halt();
            return;
        }

        // Второй токен: либо '[' (массив), либо '=' (скаляр)
        const char *t1 = getWord(1);
        if (!t1)
        {
            printError("SET syntax error: missing token after name\n");
            halt();
            return;
        }

        // ---------- Ветка: присваивание элементу массива: name [ index ] = expr ;
        if (t1[0] == '[' && t1[1] == '\0')
        {
            const char *idxTok = getWord(2);
            const char *t3 = getWord(3); // ожидаем ']'
            if (!idxTok || !t3 || !(t3[0] == ']' && t3[1] == '\0'))
            {
                printError("SET array syntax error: missing ']' or index\n");
                halt();
                return;
            }

            // Ищем '=' и ';' один раз
            const int eqI = foundNextWord(S->EQ);
            const int endI = foundNextWord(S->SEMI);
            if (eqI < 0 || endI < 0 || eqI >= endI)
            {
                printError("SET array syntax error: '=' or ';' not found\n");
                halt();
                return;
            }

            // Индекс: число или имя переменной
            int idx;
            if (isNumber(idxTok))
            {
                idx = (int)std::strtol(idxTok, 0, 10);
            }
            else
            {
                double tmp = 0.0;
                control.getVar(idxTok, tmp); // внутри есть обработка ошибок
                idx = (int)tmp;
            }

            // Вычисляем выражение справа от '='
            const double value = _fnEval(eqI + 1, endI - 1);
            control.setArrayElem(name, idx, value);

            currentWord = endI; // встанем на ';' — tick() сам перепрыгнет
            return;
        }

        // ---------- Ветка: скалярное присваивание: name = ... ;
        if (!(t1[0] == '=' && t1[1] == '\0'))
        {
            printError("SET '=' not found\n");
            halt();
            return;
        }

        // Быстрый путь: ровно 4 токена: name = value ;
        {
            const char *t3 = getWord(3);
            if (t3 && t3[0] == ';' && t3[1] == '\0')
            {
                const char *rhs = getWord(2);
                if (!rhs)
                {
                    printError("SET value not found\n");
                    halt();
                    return;
                }

                // x = число;
                if (isNumber(rhs))
                {
                    const double v = std::strtod(rhs, 0);
                    if (!control.setVar(name, v))
                        printError("SET name not found", 1);

                    currentWord += 4; // name '=' value ';'
                    return;
                }

                // x = y;  (переменная справа)
                // Это быстрее, чем вызывать общий вычислитель.
                double tmp = 0.0;
                control.getVar(rhs, tmp); // если переменной нет — ожидается внутренняя ошибка
                if (!control.setVar(name, tmp))
                    printError("SET name not found", 1);

                currentWord += 4;
                return;
            }
        }

        // ---------- Супер-быстрые шаблоны без вычислителя ----------
        // Паттерны: name = name + 1;  |  name = 1 + name;  |  name = name - 1;
        {
            const char *a = getWord(2);
            const char *op = getWord(3);
            const char *b = getWord(4);
            const char *t5 = getWord(5);

            auto isOne = [](const char *tok) noexcept
            {
                // дешёвая проверка числа "1" (в языке числа — только цифры)
                return tok && tok[0] == '1' && tok[1] == '\0';
            };

            if (t5 == S->SEMI && a && op && b)
            {
                // name = name + 1;  или  name = 1 + name;
                if (op == S->PLUS && ((a == name && isOne(b)) || (isOne(a) && b == name)))
                {
                    double cur = 0.0;
                    if (!control.getVar(name, cur) || !control.setVar(name, cur + 1.0))
                    {
                        printError("SET name not found", 1);
                        halt();
                        return;
                    }
                    currentWord += 6; // name '=' a '+' b ';'
                    return;
                }

                // name = name - 1;
                if (op == S->MINUS && a == name && isOne(b))
                {
                    double cur = 0.0;
                    if (!control.getVar(name, cur) || !control.setVar(name, cur - 1.0))
                    {
                        printError("SET name not found", 1);
                        halt();
                        return;
                    }
                    currentWord += 6; // name '=' name '-' '1' ';'
                    return;
                }
            }
        }

        // ---------- Общий случай: name = <выражение...> ;
        const int endI = foundNextWord(S->SEMI);
        if (endI < 0)
        {
            printError("SET ';' not found\n");
            halt();
            return;
        }

        const double result = _fnEval(currentWord + 2, endI - 1);
        if (!control.setVar(name, result))
            printError("SET name not found", 1);

        currentWord = endI; // встанем на ';' — tick() сам перепрыгнет
    }

    void _opIF()
    {
        int closeParenthes = foundCloseParenthes();
        int openBrace = foundNextWord(S->LBRACE);
        int closeBrace = foundCloseBrace();
        const char *openParenthes = getWord(1);

        // std::cout << "close Parenthes " << closeParenthes << " openBrace " << openBrace << " closeBrace " << closeBrace << "\n";

        if (openParenthes != S->LP)
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
            // if (std::strcmp1(word, "ELSE") == 0){
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
        if (openBrace != S->LBRACE)
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

    void _opPROC()
    {
        int closeBrace = foundCloseBrace();
        currentWord = closeBrace + 1;
    }

    void _opWHILE()
    {
        // Находим границы "( ... ) { ... }"
        int closeParenthes = foundCloseParenthes();
        int openBrace = foundNextWord(S->LBRACE);
        int closeBrace = foundCloseBrace();

        const char *openParenthes = getWord(1);
        if (openParenthes != S->LP)
        {
            printError("WHILE \"(\" not found", 1);
            halt();
            return;
        }

        // Готовим запись для стека вложенностей
        DeepCode dc;
        dc.type = DeepType::WHILE;
        dc.INword = openBrace;
        dc.OUTword = closeBrace;
        dc.EXPRstart = currentWord + 2;  // содержимое условия без '('
        dc.EXPRend = closeParenthes - 1; // и без ')'

        // --- Попытка быстрого условия: ( ident < number ) ---
        // Быстрая форма строго из трёх токенов между скобками:
        //   tok2 = идентификатор переменной
        //   tok3 = оператор сравнения
        //   tok4 = целочисленная константа (строка цифр)
        const char *tok2 = getWord(2);
        const char *tok3 = getWord(3);
        const char *tok4 = getWord(4);

        auto isNum = [&](const char *s) -> bool
        {
            if (!s || !*s)
                return false;
            for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
                if (!std::isdigit(*p))
                    return false;
            return true;
        };

        dc.fastCond = false;
        if (tok2 && tok3 && tok4 &&
            getWord(5) == S->RP && // ровно три токена внутри ( ... )
            isNum(tok4) &&
            (tok3 == S->LT || tok3 == S->LEQ || tok3 == S->GT || tok3 == S->GEQ || tok3 == S->EQEQ || tok3 == S->NEQ))
        {
            dc.fastCond = true;
            dc.condVarId = tok2;
            dc.condCst = std::strtod(tok4, nullptr);

            if (tok3 == S->LT)
                dc.condOp = DeepCode::OP_LT;
            else if (tok3 == S->LEQ)
                dc.condOp = DeepCode::OP_LE;
            else if (tok3 == S->GT)
                dc.condOp = DeepCode::OP_GT;
            else if (tok3 == S->GEQ)
                dc.condOp = DeepCode::OP_GE;
            else if (tok3 == S->EQEQ)
                dc.condOp = DeepCode::OP_EQ;
            else
                dc.condOp = DeepCode::OP_NE;
        }

        // Функция локальной оценки условия
        auto evalCond = [&]() -> double
        {
            if (!dc.fastCond)
            {
                return _fnEval(dc.EXPRstart, dc.EXPRend);
            }
            double cur = 0.0;
            // быстрый путь всё равно читает текущее значение переменной
            if (!control.getVar(dc.condVarId, cur))
                return 0.0;

            switch (dc.condOp)
            {
            case DeepCode::OP_LT:
                return (cur < dc.condCst) ? 1.0 : 0.0;
            case DeepCode::OP_LE:
                return (cur <= dc.condCst) ? 1.0 : 0.0;
            case DeepCode::OP_GT:
                return (cur > dc.condCst) ? 1.0 : 0.0;
            case DeepCode::OP_GE:
                return (cur >= dc.condCst) ? 1.0 : 0.0;
            case DeepCode::OP_EQ:
                return (cur == dc.condCst) ? 1.0 : 0.0;
            case DeepCode::OP_NE:
                return (cur != dc.condCst) ? 1.0 : 0.0;
            }
            return 0.0;
        };

        const double result = evalCond();
        if (result >= 1.0)
        {
            // Входим в тело цикла
            deepStack.push_back(dc);
            currentWord = openBrace + 1;
            control.inLevel();
            return;
        }
        else if (result == 0.0)
        {
            // Пропускаем тело
            currentWord = closeBrace + 1;
            return;
        }
        else
        {
            printError("WHILE expression not return 0/1", 1);
            halt();
            return;
        }
    }

    void _opRETURN()
    {
        if (deepStack[deepStack.size() - 1].type == DeepType::PROC)
        {
            currentWord = deepStack[deepStack.size() - 1].RETword;
            deepStack.pop_back();
            return;
        }
    }
    void _opCLOSEBRACE()
    {
        // std::cout << "last type " << deepStack[deepStack.size()-1].type << " size " << deepStack.size() << "\n";
        if (deepStack[deepStack.size() - 1].type == DeepType::PROC)
        {
            currentWord = deepStack[deepStack.size() - 1].RETword;
            deepStack.pop_back();
            return;
        }

        if (deepStack.back().type == DeepType::WHILE)
        {
            DeepCode &dc = deepStack.back();

            bool ok;
            if (dc.fastCond)
            {
                double cur = 0.0;
                control.getVar(dc.condVarId, cur);
                switch (dc.condOp)
                {
                case DeepCode::OP_LT:
                    ok = (cur < dc.condCst);
                    break;
                case DeepCode::OP_LE:
                    ok = (cur <= dc.condCst);
                    break;
                case DeepCode::OP_GT:
                    ok = (cur > dc.condCst);
                    break;
                case DeepCode::OP_GE:
                    ok = (cur >= dc.condCst);
                    break;
                case DeepCode::OP_EQ:
                    ok = (cur == dc.condCst);
                    break;
                case DeepCode::OP_NE:
                    ok = (cur != dc.condCst);
                    break;
                }
            }
            else
            {
                ok = _fnEval(dc.EXPRstart, dc.EXPRend);
            }

            if (ok)
            {
                currentWord = dc.INword + 1;
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
            if (word == S->ELSE)
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
        if (word == S->VAR)
        {
            _opCreateVar();
        }
        else if (word == S->PRINT)
        {
            _opPrint();
        }
        else if (word == S->PRINTLN)
        {
            _opPrint(1);
        }
        else if (getWord(1) == S->SEMI) //(findPROC(word) != -1)
        {
            if (findPROC(word) != -1)
            {
                DeepCode t;
                t.RETword = currentWord + 1;
                t.type = DeepType::PROC;
                deepStack.push_back(t);
                int gotoPROC = findPROC(word);
                currentWord = gotoPROC + 2; // name { ...
            }
        }
        else if (getWord(1) == S->EQ || getWord(1) == S->LBRACKET)
        {
            _opSet(); // внутри уже разберём, и если имя не найдено — выведем ошибку
        }
        else if (word == S->IF)
        {
            _opIF();
        }
        else if (word == S->WHILE)
        {
            _opWHILE();
        }
        else if (word == S->RBRACE)
        {
            _opCLOSEBRACE();
        }
        else if (word == S->ELSE)
        {
            _opELSE();
        }
        else if (word == S->SEMI)
        {
            nextWord();
        }
        else if (word == S->HALT)
        {
            halt();
        }
        else if (word == S->PROC)
        {
            _opPROC();
        }
        else if (word == S->RETURN)
        {
            _opRETURN();
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
            std::cout << i << ": ";
            for (int j = 0; j < std::strlen(words[i]); j++)
            {
                std::cout << words[i][j];
            }
            std::cout << std::endl;
        }
    }

    void printError(const char *text, int word = 0)
    {
        std::string t = "ERROR in word <" + std::to_string(currentWord) + std::to_string(word) + "><" + words[currentWord + word] + ">" + " - \n" + text + "\n";
        if (printOut)
        {
            printOut(t);
        }
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
                words.push_back(INTERN().intern(buffer));
                bufIndex = 0;
            }
        };

        while (*program)
        {
            if (*program == '"')
            {
                // 1) Закрываем предыдущий токен, если он есть
                flushBuffer();

                // 2) Открывающая кавычка как отдельный токен
                words.push_back(S->QUOTE);
                ++program;

                // 3) Собираем содержимое строки с поддержкой любого экранирования
                char strBuffer[512];
                int strIndex = 0;

                while (*program && strIndex < (int)sizeof(strBuffer) - 1)
                {

                    if (*program == '\\' && program[1] != '\0')
                    {

                        // встретили '\' — значит экранирование: копируем '\' и следующий символ
                        *program++;
                        strBuffer[strIndex++] = *program++;
                    }
                    else if (*program == '"')
                    {
                        // настоящая кавычка — конец литерала
                        break;
                    }
                    else
                    {
                        // обычный символ
                        strBuffer[strIndex++] = *program++;
                    }
                }

                // 4) Завершаем строковый буфер и сохраняем как отдельный токен
                strBuffer[strIndex] = '\0';
                words.push_back(INTERN().intern(strBuffer));

                // 5) Закрывающая кавычка как отдельный токен
                if (*program == '"')
                {
                    words.push_back(S->QUOTE);
                    ++program;
                }
            }
            else if (isspace(*program))
            {
                // пробел/перевод строки — разделитель
                flushBuffer();
                ++program;
            }
            else if (isOneCharOperator(*program))
            {
                // операторы одиночного символа тоже отдельные токены
                flushBuffer();
                char op[2] = {*program, '\0'};
                words.push_back(INTERN().intern(op));
                ++program;
            }
            else
            {
                // часть обычного идентификатора/числа/слова
                if (bufIndex < (int)sizeof(buffer) - 1)
                    buffer[bufIndex++] = *program;
                ++program;
            }
        }

        // последний буфер
        flushBuffer();

        // пост проход. Убираем !, = в !=
        for (size_t i = 0; i + 1 < words.size();)
        {
            if (words[i] == S->NOT && words[i + 1] == S->EQ)
            {
                words[i] = S->NEQ;
                words.erase(words.begin() + (i + 1));
            }
            else
            {
                ++i;
            }
        }
    }
};
