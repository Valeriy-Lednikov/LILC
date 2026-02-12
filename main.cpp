

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "lilc.cpp"
#include <chrono>

const char *loadFile(const char *filename)
{
    FILE *file = std::fopen(filename, "rb"); // Открываем в бинарном режиме
    if (!file)
    {
        std::perror("Ошибка при открытии файла");
        return nullptr;
    }

    // Узнаём размер файла
    std::fseek(file, 0, SEEK_END);
    long size = std::ftell(file);
    std::rewind(file);

    // Выделяем память (+1 байт под '\0')
    char *buffer = (char *)std::malloc(size + 1);
    if (!buffer)
    {
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
    const char *text = loadFile("prog2.lc");
    lilc interpreter;


    if (text)
    {
        interpreter.loadProgram(text);
        //interpreter.printWords();

       
        {
            auto start = std::chrono::high_resolution_clock::now();
            interpreter.interpretate();
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> duration = end - start;
            std::cout << "LILC: " << duration.count() << " ms" << std::endl;
        }
    }
}
