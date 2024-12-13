/*
 * File: function_ast.h
 * Path: /function_ast.h
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 3:04:41 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
// This class represents a function definition
#pragma once
#include "compiler_type.h"
#include "llvm-18/llvm/IR/Function.h"
#include <memory>

class Parser;
class PrototypeAST;
class ExprAST;

template <CompilerType CT, typename Derived> class FunctionAST_Base {
public:
    FunctionAST_Base(std::unique_ptr<PrototypeAST> proto,
                     std::unique_ptr<ExprAST> body, Parser *parser)
        : _proto(std::move(proto)), _body(std::move(body)), owner(parser) {}
    llvm::Function *codegen() {
        return static_cast<Derived *>(this)->doCodegen();
    }
    Parser *owner;

protected:
    std::unique_ptr<PrototypeAST> _proto;
    std::unique_ptr<ExprAST> _body;
};

class FunctionAST_AOT
    : public FunctionAST_Base<CompilerType::AOT, FunctionAST_AOT> {
public:
    using FunctionAST_Base<CompilerType::AOT,
                           FunctionAST_AOT>::FunctionAST_Base;
private:
    llvm::Function *doCodegen();
};

class FunctionAST_JIT
    : public FunctionAST_Base<CompilerType::JIT, FunctionAST_JIT> {
public:
    using FunctionAST_Base<CompilerType::JIT,
                           FunctionAST_JIT>::FunctionAST_Base;
private:
    llvm::Function *doCodegen();
};