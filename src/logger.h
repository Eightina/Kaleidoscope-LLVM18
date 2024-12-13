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
#include <llvm-18/llvm/IR/Value.h>
#include <memory>
#include <iostream>

class ExprAST;
class PrototypeAST;

std::unique_ptr<ExprAST> LogErr(const char *str);

std::unique_ptr<PrototypeAST> LogErrP(const char *str);

llvm::Value *LogErrorV(const char *str);
