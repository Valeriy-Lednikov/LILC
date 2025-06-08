#include <iostream>
#include "lilc.cpp"





int main(int argc, char *argv[])
{
    lilc interpreter;
    
    //interpreter.loadProgram("VAR x; WHILE ( #x < 10 ) { PRINTLN x ; IF ( #x == 5 ) { PRINTLN \"X5!\";} SET x = #x + 1;} ");
    interpreter.loadProgram("VAR x=2;VAR y=3;VAR res;IF(#y>#x){PRINTLN\"1\";}ELSE{PRINTLN\"0\";}");

    interpreter.printWords();
    interpreter.interpretate();



    // const char *c = "sqrt(5^2+7^2+11^2+(8-2)^2)";
    // double r = te_interp(c, 0);
    // std::cout << "The expressionres " << r << "\n";
    // return 0;
}
