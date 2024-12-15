/*
 * File: parser.h
 * Path: /parser.h
 * Module: include
 * Lang: C/C++
 * Created Date: Sunday, November 17th 2024, 12:13:08 am
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
    This file builds AST with recursive descent parsing
    & operator-precedence parsing, and then generate code with LLVM backend.
    Both AOT and JIT REPL are implemented.
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#pragma once
#include "KaleidoSopceJIT.h"
#include "ast.h"
#include "compiler_type.h"
#include "lexer.h"
#include "logger.h"
#include "token.h"
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

// class Lexer;
// class ExprAST<CT>;
// class NumberExprAST;
// class PrototypeAST;
// class VariableExprAST;
// class CallExprAST_AOT;
// class CallExprAST_JIT;
// class BinaryExprAST;
// class FunctionAST_AOT;
// class FunctionAST_JIT;

template <CompilerType CT> class Parser {
public:
    Parser() { initialize(); }

    Parser(Lexer &lexer) : _lexer(std::move(lexer)) { initialize(); }

    void test_lexer() {
        while (true) {
            getNextToken();
        }
    }

    int getNextToken() { return _curTok = _lexer.gettok(); }

    // In order to parse binary expression correctly:
    // "x+y*z" -> "x+(y*z)"
    // uses the precedence of binary operators to guide recursion
    // Operator-Precedence Parsing
    int getTokPrecedence() {
        if (!isascii(_curTok)) return -1;

        // make sure it is a declared binop
        int tokPrec = _binoPrecedence[_curTok];
        if (tokPrec <= 0) return -1;
        return tokPrec;
    }

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
    std::unique_ptr<ExprAST<CT>> parseNumberExpr() {
        auto result = std::make_unique<NumberExprAST<CT>>(_lexer._numVal, this);
        getNextToken();
        return std::move(result);
    }
    /// parenexpr ::= '(' expression ')'
    std::unique_ptr<ExprAST<CT>> parseParenExpr() {
        getNextToken();
        auto v = parseExpression();
        if (!v) return nullptr;
        if (_curTok != ')') return LogErr<CT>("expected ')'");
        getNextToken();
        return v;
    }
    // identifierexpr
    // ::= identifier
    // ::= identifier '(' expression* ')'
    std::unique_ptr<ExprAST<CT>> parseIdentifierExpr() {
        std::string idName = _lexer._identifierStr;
        getNextToken(); // take in identifier
        if (_curTok != '(')
            return std::make_unique<VariableExprAST<CT>>(idName, this);

        getNextToken(); // take in '('
        std::vector<std::unique_ptr<ExprAST<CT>>> args;
        if (_curTok != ')') { // take in expression (args)
            while (true) {
                if (auto arg = parseExpression())
                    args.push_back(std::move(arg));
                else
                    return nullptr;

                if (_curTok == ')') break;

                if (_curTok != ',')
                    return LogErr<CT>("expected ')' or ',' in arg list");

                getNextToken(); // take in ','
            }
        }

        getNextToken(); // take in ')'
        return std::make_unique<CallExprAST<CT>>(idName, std::move(args), this);
    }
    /// primary
    /// ::= identifierexpr
    /// ::= numberexpr
    /// ::= parenexpr
    std::unique_ptr<ExprAST<CT>> parsePrimary() {
        switch (_curTok) {
        default:
            return LogErr<CT>("unknown token when expection an expression");
        case tok_identifier:
            return parseIdentifierExpr();
        case tok_number:
            return parseNumberExpr();
        case '(':
            return parseParenExpr();
        }
    }
    /// expression
    /// ::= primary binoprhs
    std::unique_ptr<ExprAST<CT>> parseExpression() {
        auto lhs = parsePrimary();
        if (lhs == nullptr) return nullptr;

        return parseBinOpRHS(0, std::move(lhs));
    }
    //-------------------------------------------------------------------------

    // handling binary expressions---------------------------------------------
    //
    /// binoprhs
    /// ::= ('+' primary)*
    /*
    e.g. When parsing a + b + (c + d):
        lhs == a
        curTok == +
        then parse the rest to find possible rhs
        rhs == parsePrimary() == b
        now curTok == + (the second) and its prec is nextPrec
        then if (tokPrec < nextPrec):
            we move on to parse b + (c + d) as a whole rhs by recursion call
            that is a + (b + (c + d))
        else:
            we combine (a + b) as lhs
            that is (a + b) + (c + d)
    */
    std::unique_ptr<ExprAST<CT>>
    parseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST<CT>> lhs) {
        // The precedence value passed into ParseBinOpRHS indicates the minimal
        //  operator precedence that the function is allowed to eat
        while (true) {
            int tokPrec = getTokPrecedence();
            // guard: checking current precedence and detect whether
            //  the token runs out of operators
            if (tokPrec < exprPrec) return lhs;
            // else we know that this token is binary and should be included
            int binOp = _curTok;

            // Then get next token and parse the primary expression
            //  after the binary operator
            getNextToken();
            auto rhs = parsePrimary();
            if (!rhs) return nullptr;
            // If binOp binds less tightly with RHS than the operator after RHS,
            // let
            //  the pending operator take RHS as its LHS.
            int nextPrec = getTokPrecedence();
            if (tokPrec < nextPrec) {
                rhs = parseBinOpRHS(tokPrec + 1, std::move(rhs));
                if (!rhs) return nullptr;
            }

            // Merge LHS/RHS.
            lhs = std::make_unique<BinaryExprAST<CT>>(binOp, std::move(lhs),
                                                      std::move(rhs), this);
            // This will turn "a+b+" into "(a+b)" and execute the next iteration
            // of
            //  the loop, with "+" as the current token
        }
        // loop around to the top of the while loop.
    }
    //-------------------------------------------------------------------------

    // handling fucntion prototypes--------------------------------------------
    /// prototype
    /// ::= id '(' id* ')'
    std::unique_ptr<PrototypeAST<CT>> parsePrototype() {
        // func name
        if (_curTok != tok_identifier) {
            return LogErrP<CT>("expected function name in function prototype");
        }
        std::string fnName = _lexer._identifierStr;

        // '(' before arg list
        getNextToken();
        if (_curTok != '(')
            return LogErrP<CT>("expected '(' in function prototype");

        // arglist
        std::vector<std::string> argNames;
        while (getNextToken() == tok_identifier)
            argNames.push_back(_lexer._identifierStr);

        // ')' after arg list
        if (_curTok != ')')
            return LogErrP<CT>("expected ')' in function prototype");

        getNextToken();
        // finish
        return std::make_unique<PrototypeAST<CT>>(fnName, std::move(argNames),
                                                  this);
        // notice:
        //  If we want to move the argNames out, we need to call std::move
        //  here. The construction func of PrototypeAST receives
        //  a 'std::vector<std::string> args', so it needs to create an instance
        //  inside the func. If we return (fnName, argNames) here, argNames is a
        //  lval ref, and the construction func of PrototypeAST will call the
        //  copy construction func for it.
        //
        // Why:?
        //  Why we dont use 'std::make_unique<PrototypeAST>(fnName, argNames);'
        //  with 'PrototypeAST(const std::string &name,
        //  std::vector<std::string>&
        //              args): _name(name), _args(std::move(args)) {}'?
        // Because: This is not safe! When we call this construction func, the
        //  argNames we pass in will be invalid after the call!
    }

    /// definition ::= 'def' prototype expression
    std::unique_ptr<FunctionAST<CT>> parseDefinition() {
        // begin of the line, take in def
        getNextToken();
        auto proto = parsePrototype();
        if (!proto) return nullptr;

        auto E = parseExpression();
        if (!E) return nullptr;
        return std::make_unique<FunctionAST<CT>>(std::move(proto), std::move(E),
                                                 this);
    }

    /// external ::= 'extern' prototype
    std::unique_ptr<PrototypeAST<CT>> parseExtern() {
        getNextToken();
        return parsePrototype();
    }

    /// toplevelexpr ::= expression
    std::unique_ptr<FunctionAST<CT>> parseTopLevelExpr() {
        auto E = parseExpression();
        if (!E) return nullptr;

        // Make an anonymous proto
        auto proto =
            // std::make_unique<PrototypeAST>("", std::vector<std::string>(),
            // this);
            std::make_unique<PrototypeAST<CT>>(
                "__anon_expr", std::vector<std::string>(), this);
        return std::make_unique<FunctionAST<CT>>(std::move(proto), std::move(E),
                                                 this);
    }

    // helper func----------------------------------------------------
    void runOpt(llvm::Function *theFunction) {
        _theFPM->run(*theFunction, *_theFAM);
    }

    void initialize() {
        if constexpr (CT == CompilerType::JIT) {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
        }

        fprintf(stderr, "ready> ");
        getNextToken();

        if constexpr (CT == CompilerType::JIT)
            _theJIT = _exitOnErr(llvm::orc::KaleidoscopeJIT::Create());

        initializeModuleAndPassManager();
    }

    void initializeModuleAndPassManager() {
        // open a new context and module---------------------------------------
        _theContext = std::make_unique<llvm::LLVMContext>();
        _theModule =
            std::make_unique<llvm::Module>("KaleidoScopeJIT", *_theContext);
        if constexpr (CT == CompilerType::JIT)
            _theModule->setDataLayout(_theJIT->getDataLayout());

        // create a new builder for the module---------------------------------
        _builder = std::make_unique<llvm::IRBuilder<>>(*_theContext);
        // create new pass and analysis managers
        _theFPM = std::make_unique<llvm::FunctionPassManager>();
        _theLAM = std::make_unique<llvm::LoopAnalysisManager>();
        _theFAM = std::make_unique<llvm::FunctionAnalysisManager>();
        _theCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
        _theMAM = std::make_unique<llvm::ModuleAnalysisManager>();
        _thePIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
        _theSI = std::make_unique<llvm::StandardInstrumentations>(
            *_theContext, /*DebugLogging*/ true);
        _theSI->registerCallbacks(*_thePIC, _theMAM.get());

        // Add transform passes.-----------------------------------------------
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        _theFPM->addPass(llvm::InstCombinePass());
        // _theFPM->addPass(llvm::AggressiveInstCombinePass());
        // Reassociate expressions.
        _theFPM->addPass(llvm::ReassociatePass());
        // Eliminate Common SubExpressions.
        _theFPM->addPass(llvm::GVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        _theFPM->addPass(llvm::SimplifyCFGPass());
        // // Run InstCombine again to catch any new opportunities.
        // _theFPM->addPass(llvm::InstCombinePass());

        // Register analysis passes used in these transform passes.------------
        _pb.registerModuleAnalyses(*_theMAM);
        _pb.registerFunctionAnalyses(*_theFAM);
        _pb.crossRegisterProxies(*_theLAM, *_theFAM, *_theCGAM, *_theMAM);
    }

    llvm::Function *getFunction(std::string name) {
        // check if the function has been added to the current module
        if (auto *f = _theModule->getFunction(name)) return f;
        // if not, check whether we can codegenthe declaration from prototype
        auto fi = _functionProtos.find(name);
        if (fi != _functionProtos.end()) return fi->second->codegen();

        // if no existing prototype exists, return null;
        return nullptr;
    }

    // public:
    Lexer _lexer;
    int _curTok;
    // this holds the precedence for each binary operator that we define
    static std::map<char, int> _binoPrecedence;
    std::unique_ptr<llvm::LLVMContext> _theContext;
    std::unique_ptr<llvm::IRBuilder<>> _builder;
    std::unique_ptr<llvm::Module> _theModule;
    std::map<std::string, llvm::Value *> _namedValues;
    std::map<std::string, std::unique_ptr<PrototypeAST<CT>>> _functionProtos;
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

// Install standard binary operators: 1 is lowest precedence
template <CompilerType CT>
inline std::map<char, int> Parser<CT>::_binoPrecedence{
    {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};
