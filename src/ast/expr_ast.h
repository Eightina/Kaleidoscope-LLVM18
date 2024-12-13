/*
 * File: ast.h
 * Path: /ast.h
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 2:47:29 pm
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
#include "logger.h"
#include "parser.h"
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/Value.h>

class Parser;

// Base class for AST node
class ExprAST {
public:
    ExprAST(Parser *parser);
    virtual ~ExprAST() = default;
    virtual llvm::Value *codegen() = 0;
    Parser *owner;
};

// Expression class for numeric literals
class NumberExprAST : public ExprAST {
public:
    NumberExprAST(double val, Parser *parser);
    llvm::Value *codegen() override;

private:
    double _val;
};

// Expression class for variable
class VariableExprAST : public ExprAST {
public:
    VariableExprAST(const std::string &name, Parser *parser);
    llvm::Value *codegen() override;

private:
    std::string _name;
};

// CallExprAST=========================================================================
// Expression class for binary operator
class BinaryExprAST : public ExprAST {
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS, Parser *parser);
    llvm::Value *codegen() override;

private:
    char _op;
    std::unique_ptr<ExprAST> _lhs, _rhs;
};

// Expression class for function calls
template <CompilerType CT, typename Derived>
class CallExprAST_Base : public ExprAST {
public:
    CallExprAST_Base(const std::string &callee,
                     std::vector<std::unique_ptr<ExprAST>> args, Parser *parser)
        : ExprAST(parser), _callee(callee), _args(std::move(args)) {}
    llvm::Value *codegen() override {
        return static_cast<Derived *>(this)->doCodegen();
    }

protected:
    llvm::Value *doCodegenLoad(llvm::Function *calleeF) {
        if (!calleeF) return LogErrorV("unknown function referenced");
        if (calleeF->arg_size() != _args.size())
            return LogErrorV("incorrect number of args passed");
        std::vector<llvm::Value *> argsV;
        for (unsigned i = 0, e = _args.size(); i != e; ++i) {
            argsV.push_back(_args[i]->codegen());
            if (!argsV.back()) return nullptr;
        }
        return owner->_builder->CreateCall(calleeF, argsV, "calltmp");
    }

    std::string _callee;
    std::vector<std::unique_ptr<ExprAST>> _args;
};

class CallExprAST_AOT
    : public CallExprAST_Base<CompilerType::AOT, CallExprAST_AOT> {
public:
    using CallExprAST_Base<CompilerType::AOT,
                           CallExprAST_AOT>::CallExprAST_Base;

private:
    llvm::Value *doCodegen();
};

class CallExprAST_JIT
    : public CallExprAST_Base<CompilerType::JIT, CallExprAST_JIT> {
public:
    using CallExprAST_Base<CompilerType::JIT,
                           CallExprAST_JIT>::CallExprAST_Base;

private:
    llvm::Value *doCodegen();
};