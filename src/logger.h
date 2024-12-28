/*
 * File: logger.h
 * Path: /logger.h
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 3:07:50 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#pragma once
#include "compiler_type.h"
#include <iostream>
#include <llvm-18/llvm/IR/Value.h>
#include <memory>

template <CompilerType CT> class ExprAST;
template <CompilerType CT> class PrototypeAST;

template <CompilerType CT>
std::unique_ptr<ExprAST<CT>> LogErr(const char *str) {
    std::cout << stderr << "Error: " << str << std::endl;
    return nullptr;
}

template <CompilerType CT>
std::unique_ptr<PrototypeAST<CT>> LogErrP(const char *str) {
    LogErr<CT>(str);
    return nullptr;
}

template <CompilerType CT> llvm::Value *LogErrorV(const char *str) {
    LogErr<CT>(str);
    return nullptr;
}
