@echo off

REM Очищаем файл ошибок
if exist errorsBuild.txt del errorsBuild.txt

REM Компиляция (stdout в nul, stderr в файл)
g++ -w -c main.cpp   -o main.o   >nul 2>>errorsBuild.txt
g++ -w -c system.cpp -o system.o >nul 2>>errorsBuild.txt
g++ -w -c lilc.cpp   -o lilc.o   >nul 2>>errorsBuild.txt
gcc -w -c tinyexpr.c -o tinyexpr.o >nul 2>>errorsBuild.txt

REM Линковка
g++ -w main.o system.o lilc.o tinyexpr.o -o test.exe >nul 2>>errorsBuild.txt

echo Build finished.
