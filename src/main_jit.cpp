/*
 * File: main_jit.cpp
 * Path: /main_jit.cpp
 * Module: ast
 * Lang: C/C++
 * Created Date: Saturday, December 14th 2024, 12:23:20 am
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _          
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/ 
 */
#include "parser.h"
#include "compiler_type.h"

int main() {
    Parser<CompilerType::JIT> p;

    // Run the main "interpreter loop" now.
    p.mainLoop();

    p._theModule->print(llvm::errs(), nullptr);

    return 0;
}