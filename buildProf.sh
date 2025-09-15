#!/bin/bash

# Компилируем каждый .cpp/.c файл с флагом -pg
g++ -pg -c main.cpp -o main.o
g++ -pg -c system.cpp -o system.o
g++ -pg -c lilc.cpp -o lilc.o
gcc  -pg -c tinyexpr.c -o tinyexpr.o   # gcc тоже поддерживает -pg

# Линкуем объектные файлы с флагом -pg
g++ -pg main.o system.o lilc.o tinyexpr.o -o test

