#!/bin/bash

g++ -c main.cpp -o main.o
g++ -c system.cpp -o system.o
g++ -c lilc.cpp -o lilc.o
gcc -c tinyexpr.c -o tinyexpr.o   

g++ main.o system.o lilc.o tinyexpr.o -o test