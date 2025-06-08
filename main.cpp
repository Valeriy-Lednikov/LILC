#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "lilc.cpp"



const char* loadFile(const char* filename) {
    FILE* file = std::fopen(filename, "rb"); // Открываем в бинарном режиме
    if (!file) {
        std::perror("Ошибка при открытии файла");
        return nullptr;
    }

    // Узнаём размер файла
    std::fseek(file, 0, SEEK_END);
    long size = std::ftell(file);
    std::rewind(file);

    // Выделяем память (+1 байт под '\0')
    char* buffer = (char*)std::malloc(size + 1);
    if (!buffer) {
        std::fclose(file);
        std::fprintf(stderr, "Не удалось выделить память\n");
        return nullptr;
    }

    // Читаем файл в буфер
    size_t readBytes = std::fread(buffer, 1, size, file);
    buffer[readBytes] = '\0'; // Добавляем null-терминатор

    std::fclose(file);
    return buffer; // Возвращаем указатель (нужно будет освободить вручную!)
}





int main(int argc, char *argv[])
{
    const char* text = loadFile("prog1.lc");
    lilc interpreter;
    
    //interpreter.loadProgram("VAR x; WHILE ( #x < 10 ) { PRINTLN x ; IF ( #x == 5 ) { PRINTLN \"X5!\";} SET x = #x + 1;} ");

    if (text) {
    interpreter.loadProgram(text);

    //interpreter.printWords();
    interpreter.interpretate();

    }

    // const char *c = "sqrt(5^2+7^2+11^2+(8-2)^2)";
    // double r = te_interp(c, 0);
    // std::cout << "The expressionres " << r << "\n";
    // return 0;
}
