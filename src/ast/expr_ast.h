/*
 * File: expr_ast.h
 * Path: /ast/expr_ast.h
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
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/Value.h>

template <CompilerType CT> class Parser;

// Base class for AST node
template <CompilerType CT> class ExprAST {
public:
    ExprAST(Parser<CT> *parser) : owner(parser) {}
    virtual ~ExprAST() = default;
    virtual llvm::Value *codegen() = 0;
    Parser<CT> *owner;
};

// Expression class for numeric literals
template <CompilerType CT> class NumberExprAST : public ExprAST<CT> {
public:
    NumberExprAST(double val, Parser<CT> *parser)
        : ExprAST<CT>(parser), _val(val) {}
    llvm::Value *codegen() override {
        return llvm::ConstantFP::get(*(this->owner->_theContext),
                                     llvm::APFloat(this->_val));
    }

private:
    double _val;
};

// Expression class for variable
template <CompilerType CT> class VariableExprAST : public ExprAST<CT> {
public:
    VariableExprAST(const std::string &name, Parser<CT> *parser)
        : ExprAST<CT>(parser), _name(name) {}
    llvm::Value *codegen() override {
        llvm::Value *V = this->owner->_namedValues[this->_name];
        if (!V) LogErrorV<CT>("unknown variable name");
        return V;
    }

private:
    std::string _name;
};

// CallExprAST=========================================================================
// Expression class for binary operator
template <CompilerType CT> class BinaryExprAST : public ExprAST<CT> {
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST<CT>> LHS,
                  std::unique_ptr<ExprAST<CT>> RHS, Parser<CT> *parser)
        : ExprAST<CT>(parser), _op(op), _lhs(std::move(LHS)),
          _rhs(std::move(RHS)) {}
    llvm::Value *codegen() override {
        llvm::Value *l = this->_lhs->codegen();
        llvm::Value *r = this->_rhs->codegen();
        if (!l || !r) return nullptr;

        switch (this->_op) {
            case ('+'):
                // we give each val IR a name
                return this->owner->_builder->CreateFAdd(l, r, "addtmp");
            case ('-'):
                return this->owner->_builder->CreateFSub(l, r, "subtmp");
            case ('*'):
                return this->owner->_builder->CreateFMul(l, r, "multmp");
            case ('<'):
                l = this->owner->_builder->CreateFCmpULT(l, r, "cmptmp");
                // Convert bool 0/1 to double 0.0 or 1.0
                return this->owner->_builder->CreateUIToFP(
                    l, llvm::Type::getDoubleTy(*(this->owner)->_theContext),
                    "booltmp");
            default:
                return LogErrorV<CT>("invalid binary op");
        }
    }

private:
    char _op;
    std::unique_ptr<ExprAST<CT>> _lhs, _rhs;
};

// Expression class for function calls
template <CompilerType CT> class CallExprAST : public ExprAST<CT> {
public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST<CT>>> args,
                Parser<CT> *parser)
        : ExprAST<CT>(parser), _callee(callee), _args(std::move(args)) {}

    llvm::Value *codegen() override {
        llvm::Function *calleeF;
        if constexpr (CT == CompilerType::AOT) {
            calleeF = this->owner->_theModule->getFunction(this->_callee);
        } else if constexpr (CT == CompilerType::JIT) {
            calleeF = this->owner->getFunction(this->_callee);
        }

        if (!calleeF) return LogErrorV<CT>("unknown function referenced");
        if (calleeF->arg_size() != this->_args.size())
            return LogErrorV<CT>("incorrect number of args passed");

        std::vector<llvm::Value *> argsV;
        for (unsigned i = 0, e = this->_args.size(); i != e; ++i) {
            argsV.push_back(this->_args[i]->codegen());
            if (!argsV.back()) return nullptr;
        }

        return this->owner->_builder->CreateCall(calleeF, argsV, "calltmp");
    }

private:
    std::string _callee;
    std::vector<std::unique_ptr<ExprAST<CT>>> _args;
};
