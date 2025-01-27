/*
 * File: prototype_ast.h
 * Path:  /ast/prototype_ast.h
 * Module: ast
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
#include "compiler_type.h"
#include <llvm-18/llvm/IR/Function.h>
#include <memory>
#include <vector>

template <CompilerType CT> class ParserEnv;

enum class PrototypeType { NonOp, Unary, Binary };

// This class represents the prototype for a function, including
//  its name, arg names, arg number
template <CompilerType CT> class PrototypeAST {
public:
    PrototypeAST(const std::string &name, std::vector<std::string> args,
                 ParserEnv<CT> *env)
        : name_(name), args_(std::move(args)), env_(env),
          thisType_(PrototypeType::NonOp) {}

    const std::string &getName() const { return name_; }

    const std::vector<std::string> &getArgs() const { return args_; }

    bool isUnaryOp() const {
        return thisType_ != PrototypeType::NonOp && args_.size() == 1;
    }

    bool isBinaryOp() const {
        return thisType_ != PrototypeType::NonOp && args_.size() == 2;
    }

    char getOpName() const {
        assert(isUnaryOp() || isBinaryOp());
        return name_[name_.size() - 1];
    }

    llvm::Function *codegen() {
        // create the arguments list for the function prototype
        std::vector<llvm::Type *> doubles(
            args_.size(), llvm::Type::getDoubleTy(*(env_->getContext())));
        // make the function type: double(double, double), etc.
        llvm::FunctionType *FT = llvm::FunctionType::get(
            llvm::Type::getDoubleTy(*(env_->getContext())), doubles, false);
        // codegen for function prototype. “external linkage” means that the
        //  function may be defined outside the current module and/or that it is
        //  callable by functions outside the module. Name passed in is the name
        //  the user specified: since "TheModule" is specified, this name is
        //  registered in "TheModule"s symbol table.
        llvm::Function *F = llvm::Function::Create(
            FT, llvm::Function::ExternalLinkage, name_, env_->getModule());
        unsigned Idx = 0;
        for (auto &arg : F->args()) {
            arg.setName(args_[Idx++]);
        }
        return F;
    }

protected:
    ParserEnv<CT> *env_;
    std::string name_;
    std::vector<std::string> args_;
    PrototypeType thisType_;
};
