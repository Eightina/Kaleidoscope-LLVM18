/*
 * File: prototype_ast.cpp
 * Path: /prototype_ast.cpp
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 3:01:33 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#include "prototype_ast.h"
#include "parser.h"

PrototypeAST::PrototypeAST(const std::string &name,
                           std::vector<std::string> args, Parser *parser)
    : _name(name), _args(std::move(args)), owner(parser) {}

llvm::Function *PrototypeAST::codegen() {
    // create the arguments list for the function prototype
    std::vector<llvm::Type *> doubles(
        _args.size(), llvm::Type::getDoubleTy(*owner->_theContext));
    // make the function type: double(double, double), etc.
    llvm::FunctionType *FT = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*owner->_theContext), doubles, false);
    // codegen for function prototype. “external linkage” means that the
    //  function may be defined outside the current module and/or that it is
    //  callable by functions outside the module. Name passed in is the name the
    //  user specified: since "TheModule" is specified, this name is registered
    //  in "TheModule"s symbol table.
    llvm::Function *F = llvm::Function::Create(
        FT, llvm::Function::ExternalLinkage, _name, owner->_theModule.get());
    unsigned Idx = 0;
    for (auto &arg : F->args()) {
        arg.setName(_args[Idx++]);
    }
    return F;
}