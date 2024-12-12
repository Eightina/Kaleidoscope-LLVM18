/*
 * File: parser.h
 * Path: /parser.h
 * Module: include
 * Lang: C/C++
 * Created Date: Sunday, November 17th 2024, 12:13:08 am
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#pragma once
#include "KaleidoSopceJIT.h"
#include "lexer.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <llvm-18/llvm/ADT/APFloat.h>
#include <llvm-18/llvm/ADT/STLExtras.h>
#include <llvm-18/llvm/IR/BasicBlock.h>
#include <llvm-18/llvm/IR/Constants.h>
#include <llvm-18/llvm/IR/DerivedTypes.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/IRBuilder.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Module.h>
#include <llvm-18/llvm/IR/PassInstrumentation.h>
#include <llvm-18/llvm/IR/PassManager.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/IR/Verifier.h>
#include <llvm-18/llvm/Pass.h>
#include <llvm-18/llvm/Passes/PassBuilder.h>
#include <llvm-18/llvm/Passes/StandardInstrumentations.h>
#include <llvm-18/llvm/Support/Error.h>
#include <llvm-18/llvm/Support/TargetSelect.h>
#include <llvm-18/llvm/Support/raw_ostream.h>
#include <llvm-18/llvm/Target/TargetMachine.h>
#include <llvm-18/llvm/Transforms/AggressiveInstCombine/AggressiveInstCombine.h>
#include <llvm-18/llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm-18/llvm/Transforms/Scalar.h>
#include <llvm-18/llvm/Transforms/Scalar/GVN.h>
#include <llvm-18/llvm/Transforms/Scalar/Reassociate.h>
#include <llvm-18/llvm/Transforms/Scalar/SimplifyCFG.h>
#include <map>
#include <memory>
#include <vector>

class Parser;
// Base class for AST node
class ExprAST {
public:
    ExprAST(Parser *parser) : owner(parser) {}
    virtual ~ExprAST() = default;
    virtual llvm::Value *codegen() = 0;
    Parser *owner;
};

// Expression class for numeric literals
class NumberExprAST : public ExprAST {
public:
    NumberExprAST(double val, Parser *parser) : ExprAST(parser), _val(val) {}
    llvm::Value *codegen() override;

private:
    double _val;
};

// Expression class for variable
class VariableExprAST : public ExprAST {
public:
    VariableExprAST(const std::string &name, Parser *parser)
        : ExprAST(parser), _name(name) {}
    llvm::Value *codegen() override;

private:
    std::string _name;
};

// Expression class for binary operator
class BinaryExprAST : public ExprAST {
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS, Parser *parser)
        : ExprAST(parser), _op(op), _lhs(std::move(LHS)), _rhs(std::move(RHS)) {
    }
    llvm::Value *codegen() override;

private:
    char _op;
    std::unique_ptr<ExprAST> _lhs, _rhs;
};

// Expression class for function calls
class CallExprAST : public ExprAST {
public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args, Parser *parser)
        : ExprAST(parser), _callee(callee), _args(std::move(args)) {}
    llvm::Value *codegen() override;

private:
    std::string _callee;
    std::vector<std::unique_ptr<ExprAST>> _args;
};

// This class represents the prototype for a function, including
// its name, arg names, arg number
class PrototypeAST {
public:
    PrototypeAST(const std::string &name, std::vector<std::string> args,
                 Parser *parser)
        : _name(name), _args(std::move(args)), owner(parser) {}
    const std::string &getName() const { return _name; }
    const std::vector<std::string> &getArgs() const { return _args; }
    llvm::Function *codegen();
    Parser *owner;

private:
    std::string _name;
    std::vector<std::string> _args;
};

// This class represents a function definition
class FunctionAST {
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body, Parser *parser)
        : _proto(std::move(proto)), _body(std::move(body)), owner(parser) {}
    llvm::Function *codegen();
    Parser *owner;

private:
    std::unique_ptr<PrototypeAST> _proto;
    std::unique_ptr<ExprAST> _body;
};

std::unique_ptr<ExprAST> LogErr(const char *str);
std::unique_ptr<PrototypeAST> LogErrP(const char *str);
llvm::Value *LogErrorV(const char *str);

class Parser {
public:
    Parser();
    Parser(Lexer &lexer);

    void test_lexer();

    int getNextToken();

    int getTokPrecedence();

    // For the parser funcs that are not the beginning of a line(like
    //  parseBinOpRHS):
    // e.g. need to parse:    A       B       C
    //  func should consider: ^
    //                        curTok
    // That means we have called getNextToken() for A already,
    //  so the first time we call getNextToken() in this func is for B,
    //  and we need to call it one more time after C to maintain this order.
    //

    // handling basic expression units-----------------------------------------
    /// numberexpr ::= number
    std::unique_ptr<ExprAST> parseNumberExpr();
    /// parenexpr ::= '(' expression ')'
    std::unique_ptr<ExprAST> parseParenExpr();
    /// identifierexpr ::= identifier ::= identifier '(' expression* ')'
    std::unique_ptr<ExprAST> parseIdentifierExpr();
    /// primary ::= identifierexpr ::= numberexpr ::= parenexpr
    std::unique_ptr<ExprAST> parsePrimary();
    //-------------------------------------------------------------------------

    // handling binary expressions---------------------------------------------
    /// binoprhs ::= ('+' primary)*
    std::unique_ptr<ExprAST> parseBinOpRHS(int exprPrec,
                                           std::unique_ptr<ExprAST> lhs);
    /// expression ::= primary binoprhs
    std::unique_ptr<ExprAST> parseExpression();
    //-------------------------------------------------------------------------

    // handling fucntion prototypes--------------------------------------------
    /// prototype ::= id '(' id* ')'
    std::unique_ptr<PrototypeAST> parsePrototype();
    /// definition ::= 'def' prototype expression
    std::unique_ptr<FunctionAST> parseDefinition();
    /// external ::= 'extern' prototype
    std::unique_ptr<PrototypeAST> parseExtern();

    //-------------------------------------------------------------------------
    /// toplevelexpr ::= expression
    std::unique_ptr<FunctionAST> parseTopLevelExpr();

    //-------------------------------------------------------------------------
    void runOpt(llvm::Function *theFunction);

    void initialize();

    void initializeModuleAndPassManager();

    llvm::Function *getFunction(std::string name);

    // error handling-------------------------------------------------------
    void handleDefinition();

    void handleExtern();

    void handleTopLevelExpression();

    //-------------------------------------------------------------------------
    void mainLoop();

    // public:
    Lexer _lexer;
    int _curTok;
    // this holds the precedence for each binary operator that we define
    static std::map<char, int> _binoPrecedence;
    std::unique_ptr<llvm::LLVMContext> _theContext;
    std::unique_ptr<llvm::IRBuilder<>> _builder;
    std::unique_ptr<llvm::Module> _theModule;
    std::map<std::string, llvm::Value *> _namedValues;
    std::map<std::string, std::unique_ptr<PrototypeAST>> _functionProtos;
    // for optimizations and JIT
    std::unique_ptr<llvm::FunctionPassManager> _theFPM;
    std::unique_ptr<llvm::LoopAnalysisManager> _theLAM;
    std::unique_ptr<llvm::FunctionAnalysisManager> _theFAM;
    std::unique_ptr<llvm::CGSCCAnalysisManager> _theCGAM;
    std::unique_ptr<llvm::ModuleAnalysisManager> _theMAM;
    std::unique_ptr<llvm::PassInstrumentationCallbacks> _thePIC;
    std::unique_ptr<llvm::StandardInstrumentations> _theSI;
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> _theJIT;
    //
    llvm::PassBuilder _pb;
    llvm::ExitOnError _exitOnErr;
};
