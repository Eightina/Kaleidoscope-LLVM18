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
#include "compiler_type.h"
#include "driver.h"
#include "parser.h"

/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X) {
  fputc((char)X, stderr);
  return 0;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT double printd(double X) {
  fprintf(stderr, "%f\n", X);
  return 0;
}


int main() {
    Parser<CompilerType::JIT> p(false);
    Driver<CompilerType::JIT> driver(&p);
    ParserEnv<CompilerType::JIT> *pEnv = p.getEnv();

    // Run the main "interpreter loop" now.
    driver.mainLoop();

    pEnv->printErr();

    return 0;
}
 