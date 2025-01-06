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
#include "parser.h"
#include "utils.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    bool enableInteractive = true;
    bool enableOptimization = true;

    if (argc > 2) {
        printf("error: input more than one file");
        return -1;
    }
    if (argc == 2) {
        auto filename = argv[1];
        if (freopen(filename, "r", stdin) == nullptr) {
            std::cerr << "Error: Failed to open file." << std::endl;
            return 1;
        }
        enableInteractive = false;
    }

    Driver<CompilerType::AOT> driver(enableOptimization, enableInteractive);
    ParserEnv<CompilerType::AOT> *pEnv = driver.getParserEnv();

    // Run the main "interpreter loop" now.
    driver.mainLoop();

    pEnv->printErr();

    return 0;
}
