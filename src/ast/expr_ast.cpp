/*
 * File: ast.cpp
 * Path: /ast.cpp
 * Module: src
 * Lang: C/C++
 * Created Date: Friday, December 13th 2024, 2:47:21 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#include "expr_ast.h"

// ExprAST=============================================================================
ExprAST::ExprAST(Parser *parser) : owner(parser) {}

// NumberExprAST=======================================================================
NumberExprAST::NumberExprAST(double val, Parser *parser)
    : ExprAST(parser), _val(val) {}

llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(*(owner->_theContext), llvm::APFloat(_val));
}

// VariableExprAST======================================================================
VariableExprAST::VariableExprAST(const std::string &name, Parser *parser)
    : ExprAST(parser), _name(name) {}

llvm::Value *VariableExprAST::codegen() {
    llvm::Value *V = owner->_namedValues[_name];
    if (!V) LogErrorV("unknown variable name");
    return V;
}

// BinaryExprAST=========================================================================
BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS, Parser *parser)
    : ExprAST(parser), _op(op), _lhs(std::move(LHS)), _rhs(std::move(RHS)) {}

llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *l = _lhs->codegen();
    llvm::Value *r = _rhs->codegen();
    if (!l || !r) return nullptr;

    switch (_op) {
    case ('+'):
        // we give each val IR a name
        return owner->_builder->CreateFAdd(l, r, "addtmp");
    case ('-'):
        return owner->_builder->CreateFSub(l, r, "subtmp");
    case ('*'):
        return owner->_builder->CreateFMul(l, r, "multmp");
    case ('<'):
        l = owner->_builder->CreateFCmpULT(l, r, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return owner->_builder->CreateUIToFP(
            l, llvm::Type::getDoubleTy(*owner->_theContext), "booltmp");
    default:
        return LogErrorV("invalid binary op");
    }
}

// CallExprAST_Base=========================================================================
llvm::Value *CallExprAST_AOT::CallExprAST_AOT::doCodegen() {
    llvm::Function *calleeF = owner->_theModule->getFunction(_callee);
    return doCodegenLoad(calleeF);
}

llvm::Value *CallExprAST_JIT::CallExprAST_JIT::doCodegen() {
    llvm::Function *calleeF = owner->getFunction(_callee);
    return doCodegenLoad(calleeF);
}