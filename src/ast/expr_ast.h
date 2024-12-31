/*
 * File: expr_ast.h
 * Path: /ast/expr_ast.h
 * Module: ast
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
#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm-18/llvm/IR/BasicBlock.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Value.h>
#include <memory>

// template <CompilerType CT> class Parser;
template <CompilerType CT> class ParserEnv;

// Base class for AST node
template <CompilerType CT> class ExprAST {
public:
    ExprAST(ParserEnv<CT> *env) : env_(env) {}
    virtual ~ExprAST() = default;
    virtual llvm::Value *codegen() = 0;

protected:
    ParserEnv<CT> *env_;
};

// Expression class for numeric literals
template <CompilerType CT> class NumberExprAST : public ExprAST<CT> {
public:
    NumberExprAST(double val, ParserEnv<CT> *env)
        : ExprAST<CT>(env), val_(val) {}
    llvm::Value *codegen() override {
        return llvm::ConstantFP::get(*(this->env_->getContext()),
                                     llvm::APFloat(this->val_));
    }

private:
    double val_;
};

// Expression class for variable
template <CompilerType CT> class VariableExprAST : public ExprAST<CT> {
public:
    VariableExprAST(const std::string &name, ParserEnv<CT> *env)
        : ExprAST<CT>(env), name_(name) {}
    llvm::Value *codegen() override {
        llvm::Value *V = this->env_->getValue(this->name_);
        if (!V) LogErrorV<CT>("unknown variable name");
        return V;
    }

private:
    std::string name_;
};

// CallExprAST=========================================================================
// Expression class for binary operator
template <CompilerType CT> class BinaryExprAST : public ExprAST<CT> {
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST<CT>> LHS,
                  std::unique_ptr<ExprAST<CT>> RHS, ParserEnv<CT> *env)
        : ExprAST<CT>(env), op_(op), lhs_(std::move(LHS)),
          rhs_(std::move(RHS)) {}
    llvm::Value *codegen() override {
        llvm::Value *l = this->lhs_->codegen();
        llvm::Value *r = this->rhs_->codegen();
        if (!l || !r) return nullptr;

        llvm::IRBuilder<> *curBuilder = this->env_->getBuilder();
        switch (this->op_) {
        case ('+'):
            // we give each val IR a name
            return curBuilder->CreateFAdd(l, r, "addtmp");
        case ('-'):
            return curBuilder->CreateFSub(l, r, "subtmp");
        case ('*'):
            return curBuilder->CreateFMul(l, r, "multmp");
        case ('<'):
            l = curBuilder->CreateFCmpULT(l, r, "cmptmp");
            // Convert bool 0/1 to double 0.0 or 1.0
            return curBuilder->CreateUIToFP(
                l, llvm::Type::getDoubleTy(*(this->env_->getContext())),
                "booltmp");
        default:
            return LogErrorV<CT>("invalid binary op");
        }
    }

private:
    char op_;
    std::unique_ptr<ExprAST<CT>> lhs_, rhs_;
};

// Expression class for function calls
template <CompilerType CT> class CallExprAST : public ExprAST<CT> {
public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST<CT>>> args,
                ParserEnv<CT> *env)
        : ExprAST<CT>(env), callee_(callee), args_(std::move(args)) {}

    llvm::Value *codegen() override {
        llvm::Function *calleeF;
        if constexpr (CT == CompilerType::AOT) {
            calleeF = this->env_->getModule()->getFunction(this->callee_);
        } else if constexpr (CT == CompilerType::JIT) {
            calleeF = this->env_->getFunction(this->callee_);
        }

        if (!calleeF) return LogErrorV<CT>("unknown function referenced");
        if (calleeF->arg_size() != this->args_.size())
            return LogErrorV<CT>("incorrect number of args passed");

        std::vector<llvm::Value *> argsV;
        for (unsigned i = 0, e = this->args_.size(); i != e; ++i) {
            argsV.push_back(this->args_[i]->codegen());
            if (!argsV.back()) return nullptr;
        }

        return this->env_->getBuilder()->CreateCall(calleeF, argsV, "calltmp");
    }

private:
    std::string callee_;
    std::vector<std::unique_ptr<ExprAST<CT>>> args_;
};

template <CompilerType CT> class IfExprAST : public ExprAST<CT> {
public:
    IfExprAST(std::unique_ptr<ExprAST<CT>> condp,
              std::unique_ptr<ExprAST<CT>> thenp, ParserEnv<CT> *env)
        : ExprAST<CT>(env), cond_(std::move(condp)), then_(std::move(thenp)) {
        else_ = nullptr;
    }

    IfExprAST(std::unique_ptr<ExprAST<CT>> condp,
              std::unique_ptr<ExprAST<CT>> thenp,
              std::unique_ptr<ExprAST<CT>> elsep, ParserEnv<CT> *env)
        : ExprAST<CT>(env), cond_(std::move(condp)), then_(std::move(thenp)),
          else_(std::move(elsep)) {}

    llvm::Value *codegen() override {
        // We are creating a struction like: Funciton-BBlock-code

        llvm::Value *condV = cond_->codegen();
        if (!condV) return nullptr;

        // convert condition to a bool by comparing non-equal to 0.0
        llvm::IRBuilder<> *curBuilder = this->env_->getBuilder();
        llvm::LLVMContext *curContext = this->env_->getContext();
        // evaluate "if"
        condV = curBuilder->CreateFCmpONE(
            condV, llvm::ConstantFP::get(*curContext, llvm::APFloat(0.0)));

        // Create blocks for the then and else cases.
        // Insert the 'then' block at the end of the function.
        // Firstly get the current Function object that is being built
        llvm::Function *theFunction = curBuilder->GetInsertBlock()->getParent();
        // parent = theFunction ----> inserted into the end of "theFunction"
        llvm::BasicBlock *thenBB =
            llvm::BasicBlock::Create(*curContext, "then", theFunction);
        // parent = nullptr ----> not yet inserted into the function
        llvm::BasicBlock *elseBB =
            llvm::BasicBlock::Create(*curContext, "else");
        llvm::BasicBlock *mergeBB =
            llvm::BasicBlock::Create(*curContext, "ifcont");
        // Emit the conditional branch. Creating new blocks does not implicitly
        //  affect the IRBuilder, so it is still inserting into the block that
        //  the condition went into
        curBuilder->CreateCondBr(condV, thenBB, elseBB);

        // Emit "then" block. -------------------------------------------------
        // Move the builder to start inserting into the end
        //  of the "then" block. Due to thenBB being empty, it is also the
        //  beginning
        curBuilder->SetInsertPoint(thenBB);
        llvm::Value *thenV = then_->codegen();
        if (!thenV) return nullptr;
        // Create an unconditional branch to finish off the "then" block:
        //  LLVM IR requires all basic blocks to be “terminated” with
        //  a control flow instruction such as return or branch
        curBuilder->CreateBr(mergeBB);
        // Codegen of 'Then' can change current block, update ThenBB for PHI.
        thenBB = curBuilder->GetInsertBlock();

        // Emit "else" block. -------------------------------------------------
        theFunction->insert(theFunction->end(), elseBB);
        curBuilder->SetInsertPoint(elseBB);
        llvm::Value* elseV;
        if (else_ != nullptr) elseV = else_->codegen();
        else elseV = llvm::ConstantFP::get(*curContext, llvm::APFloat(0.0));
        curBuilder->CreateBr(mergeBB);
        elseBB = curBuilder->GetInsertBlock();

        // Emit "merge" block. -------------------------------------------------
        theFunction->insert(theFunction->end(), mergeBB);
        curBuilder->SetInsertPoint(mergeBB);
        llvm::PHINode *pn = curBuilder->CreatePHI(llvm::Type::getDoubleTy(*curContext), 2, "iftmp");
        pn->addIncoming(thenV, thenBB);
        pn->addIncoming(elseV, elseBB);

        return pn;
    }

private:
    std::unique_ptr<ExprAST<CT>> cond_, then_, else_;
};
