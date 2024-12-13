/*
 * File: logger.cpp
 * Path: /logger.cpp
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 3:07:55 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _          
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/ 
 */
#include "logger.h"

std::unique_ptr<ExprAST> LogErr(const char *str) {
    std::cout << stderr << "Error: " << str << std::endl;
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrP(const char *str) {
    LogErr(str);
    return nullptr;
}

llvm::Value *LogErrorV(const char *str) {
    LogErr(str);
    return nullptr;
}