/*
 * File: function_ast.cpp
 * Path: /function_ast.cpp
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 3:04:33 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#include "function_ast.h"
#include "expr_ast.h"
#include "parser.h"
#include "prototype_ast.h"

llvm::Function *FunctionAST_AOT::doCodegen() {
    // First, check for an existing function from a previous 'extern'
    //  declaration.
    llvm::Function *theFunction =
        owner->_theModule->getFunction(_proto->getName());
    bool nameSame = true, argsSame = true;
    if (theFunction) {
        const auto &fargs = theFunction->args();
        const auto &pargs = _proto->getArgs();
        int tempi = 0;
        if (theFunction->arg_size() == pargs.size()) {
            for (auto &arg : fargs) {
                if (std::string(arg.getName()) != pargs[tempi]) {
                    argsSame = false;
                    break;
                }
                ++tempi;
            }
        } else {
            argsSame = false;
        }
    } else {
        nameSame = false;
    }

    if (!nameSame || !argsSame) theFunction = _proto->codegen();
    if (!theFunction) return nullptr;

    // we want to assert that the function is empty (i.e. has no body yet)
    //  before we start
    if (!theFunction->empty())
        return (llvm::Function *)LogErrorV("function cannot be redefined");

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *bb =
        llvm::BasicBlock::Create(*owner->_theContext, "entry", theFunction);
    owner->_builder->SetInsertPoint(bb);
    // record the function arguments in the namedvalues table
    owner->_namedValues.clear();
    for (auto &arg : theFunction->args()) {
        owner->_namedValues[std::string(arg.getName())] = &arg;
    }
    if (llvm::Value *retVal = _body->codegen()) {
        owner->_builder->CreateRet(retVal);
        llvm::verifyFunction(*theFunction);
        // after verifying consistency, do optimizations
        owner->runOpt(theFunction);
        return theFunction;
    }

    // deleting the function we produced. later we may redefine a function that
    //  we incorrectly typed in before: if we didn’t delete it, it would live
    //  in the symbol table, with a body, preventing future redefinition.
    theFunction->eraseFromParent();
    return nullptr;
}

llvm::Function *FunctionAST_JIT::doCodegen() {
    // In JIT, we dont mind redefinition or whatsoever.
    // Firstly, transfer ownership of the prototype to the FunctionProtos map,
    //  but keep a reference to it for use below.
    auto &p = *_proto;
    owner->_functionProtos[_proto->getName()] = std::move(_proto);
    llvm::Function *theFunction = owner->getFunction(p.getName());
    if (!theFunction) return nullptr;

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *bb =
        llvm::BasicBlock::Create(*owner->_theContext, "entry", theFunction);
    owner->_builder->SetInsertPoint(bb);

    // record the function arguments in the namedvalues table
    owner->_namedValues.clear();
    for (auto &arg : theFunction->args()) {
        owner->_namedValues[std::string(arg.getName())] = &arg;
    }
    if (llvm::Value *retVal = _body->codegen()) {
        owner->_builder->CreateRet(retVal);
        llvm::verifyFunction(*theFunction);
        // after verifying consistency, do optimizations
        owner->runOpt(theFunction);
        return theFunction;
    }

    // deleting the function we produced. later we may redefine a function that
    //  we incorrectly typed in before: if we didn’t delete it, it would live
    //  in the symbol table, with a body, preventing future redefinition.
    theFunction->eraseFromParent();
    return nullptr;
}
