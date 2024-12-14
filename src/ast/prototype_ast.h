/*
 * File: prototype_ast.h
 * Path:  /ast/prototype_ast.h
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
#include "compiler_type.h"
#include <llvm-18/llvm/IR/Function.h>
#include <memory>
#include <vector>

template <CompilerType CT> class Parser;

// This class represents the prototype for a function, including
//  its name, arg names, arg number
template <CompilerType CT> class PrototypeAST {
public:
    PrototypeAST(const std::string &name, std::vector<std::string> args,
                 Parser<CT> *parser)
        : _name(name), _args(std::move(args)), owner(parser) {}

    const std::string &getName() const { return _name; }

    const std::vector<std::string> &getArgs() const { return _args; }

    llvm::Function *codegen() {
        // create the arguments list for the function prototype
        std::vector<llvm::Type *> doubles(
            _args.size(), llvm::Type::getDoubleTy(*owner->_theContext));
        // make the function type: double(double, double), etc.
        llvm::FunctionType *FT = llvm::FunctionType::get(
            llvm::Type::getDoubleTy(*owner->_theContext), doubles, false);
        // codegen for function prototype. “external linkage” means that the
        //  function may be defined outside the current module and/or that it is
        //  callable by functions outside the module. Name passed in is the name
        //  the user specified: since "TheModule" is specified, this name is
        //  registered in "TheModule"s symbol table.
        llvm::Function *F =
            llvm::Function::Create(FT, llvm::Function::ExternalLinkage, _name,
                                   owner->_theModule.get());
        unsigned Idx = 0;
        for (auto &arg : F->args()) {
            arg.setName(_args[Idx++]);
        }
        return F;
    }

    Parser<CT> *owner;

private:
    std::string _name;
    std::vector<std::string> _args;
};
