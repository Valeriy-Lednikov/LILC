
# EN

This is my project implementing a simple interpreted language.

Temporary Features
A "#" symbol is required before variable names in expression evaluation contexts (if, while, set)
# Language Syntax
VAR  - declares a new variable
VAR x; - creates a variable x with value 0
VAR x = 5; - creates a variable x with value 5
VAR x = #a + #b; (wip) - creates a variable with the value of the expression result

SET - assigns a value to a variable
SET x = 5; - sets the value of x to 5
SET x = #a + #b - sets the value of x as the result of the expression

HALT - forces interpreter termination

IF - conditional statement
Supports logical operators (==, !=, >, <, >=, <=)
IF ( #x + #y + sin(30) * 6 == #res ) { //code } - executes the code if the condition is true
ELSE { //code } - executes the else block if the IF condition evaluates to logical 0

WHILE ( #x < 10) { //code } - executes the code in a loop while the condition is true.

PRINT - outputs text or a variable
PRINTLN - outputs text or a variable and adds a newline
PRINT "Value x = "; - outputs the text "value x = "
PRINTLN x; - outputs the value of x and moves to the next line


# RU

Это мой проект, реализующий простой интерпретируемый язык.

Временные особенности  
Символ "#" обязателен перед названием переменных в контексте вычисления выражений (if, while, set)  
# Синтаксис языка  
VAR  - объявляет новую переменную  
VAR x; - создаёт переменную x со значением 0  
VAR x = 5; - создаёт переменную x со значением 5  
VAR x = #a + #b; (в разработке) - создаёт переменную со значением результата выражения  

SET - присваивает значение переменной  
SET x = 5; - устанавливает значение x равным 5  
SET x = #a + #b - устанавливает значение x как результат выражения  

HALT - принудительно завершает интерпретацию  

IF - условный оператор  
Поддерживает логические операторы (==, !=, >, <, >=, <=)  
IF ( #x + #y + sin(30) * 6 == #res ) { //код } - выполняет код, если условие истинно  
ELSE { //код } - выполняет блок else, если условие IF возвращает логический 0  

WHILE ( #x < 10) { //код } - выполняет код в цикле, пока условие истинно  

PRINT - выводит текст или переменную  
PRINTLN - выводит текст или переменную и добавляет перенос строки  
PRINT "Value x = "; - выводит текст "value x = "  
PRINTLN x; - выводит значение x и переходит на следующую строку  