/*
 * File: prototype_ast.h
 * Path: /prototype_ast.h
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 3:01:46 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#pragma once
#include <llvm-18/llvm/IR/Function.h>
#include <memory>
#include <vector>

class Parser;

// This class represents the prototype for a function, including
//  its name, arg names, arg number
class PrototypeAST {
public:
    PrototypeAST(const std::string &name, std::vector<std::string> args,
                 Parser *parser);
    const std::string &getName() const { return _name; }
    const std::vector<std::string> &getArgs() const { return _args; }
    llvm::Function *codegen();
    Parser *owner;

private:
    std::string _name;
    std::vector<std::string> _args;
};