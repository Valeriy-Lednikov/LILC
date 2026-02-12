@echo off

REM Компиляция .cpp файлов
cl /c /EHsc main.cpp
cl /c /EHsc system.cpp
cl /c /EHsc lilc.cpp

REM Компиляция C файла
cl /c tinyexpr.c

REM Линковка
link main.obj system.obj lilc.obj tinyexpr.obj /OUT:test.exe

echo Build finished.
pause
