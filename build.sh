#!/bin/bash

# Компилируем каждый .cpp файл в .o
g++ -c main.cpp -o main.o
g++ -c system.cpp -o system.o
g++ -c lilc.cpp -o lilc.o
gcc -c tinyexpr.c -o tinyexpr.o   # для C файла gcc (но можно и g++)

# Линкуем объектные файлы в итоговый исполняемый файл
g++ main.o system.o lilc.o tinyexpr.o -o test
