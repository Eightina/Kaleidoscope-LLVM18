/*
 * File: main_aot.cpp
 * Path: /main_aot.cpp
 * Module: ast
 * Lang: C/C++
 * Created Date: Saturday, December 14th 2024, 12:23:12 am
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#include "compiler_type.h"
#include "driver.h"
#include "lexer.h"
#include "parser.h"

int main() {
    // Lexer l;
    Parser<CompilerType::AOT> p(true);
    Driver<CompilerType::AOT> driver(&p);
    ParserEnv<CompilerType::AOT> *pEnv = p.getEnv();

    // Run the main "interpreter loop" now.
    driver.mainLoop();

    pEnv->printErr();

    return 0;
}
