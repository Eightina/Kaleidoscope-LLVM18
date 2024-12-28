/*
 * File: parser.h
 * Path: /parser/parser.h
 * Module: parser
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
#include "parser_env.h"
#include "token.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <llvm-18/llvm/Support/TargetSelect.h>
#include <map>
#include <memory>
#include <vector>

template <CompilerType CT> class Parser {
public:
    Parser() { initialize(); }

    Parser(Lexer &lexer) : lexer_(std::move(lexer)) { initialize(); }

    void test_lexer() {
        while (true) {
            getNextToken();
        }
    }

    int getNextToken() { return curTok_ = lexer_.getTok(); }

    // In order to parse binary expression correctly:
    // "x+y*z" -> "x+(y*z)"
    // uses the precedence of binary operators to guide recursion
    // Operator-Precedence Parsing
    int getTokPrecedence() {
        if (!isascii(curTok_)) return -1;

        // make sure it is a declared binop
        int tokPrec = binoPrecedence_[curTok_];
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
        auto result =
            std::make_unique<NumberExprAST<CT>>(lexer_.getNumVal(), env_.get());
        getNextToken();
        return std::move(result);
    }
    /// parenexpr ::= '(' expression ')'
    std::unique_ptr<ExprAST<CT>> parseParenExpr() {
        getNextToken();
        auto v = parseExpression();
        if (!v) return nullptr;
        if (curTok_ != ')') return LogErr<CT>("expected ')'");
        getNextToken();
        return v;
    }
    // identifierexpr
    // ::= identifier
    // ::= identifier '(' expression* ')'
    std::unique_ptr<ExprAST<CT>> parseIdentifierExpr() {
        std::string idName(lexer_.getIdentifierStr());
        getNextToken(); // take in identifier
        if (curTok_ != '(')
            return std::make_unique<VariableExprAST<CT>>(idName, env_.get());

        getNextToken(); // take in '('
        std::vector<std::unique_ptr<ExprAST<CT>>> args;
        if (curTok_ != ')') { // take in expression (args)
            while (true) {
                if (auto arg = parseExpression())
                    args.push_back(std::move(arg));
                else
                    return nullptr;

                if (curTok_ == ')') break;

                if (curTok_ != ',')
                    return LogErr<CT>("expected ')' or ',' in arg list");

                getNextToken(); // take in ','
            }
        }

        getNextToken(); // take in ')'
        return std::make_unique<CallExprAST<CT>>(idName, std::move(args),
                                                 env_.get());
    }
    /// primary
    /// ::= identifierexpr
    /// ::= numberexpr
    /// ::= parenexpr
    std::unique_ptr<ExprAST<CT>> parsePrimary() {
        switch (curTok_) {
        default:
            return LogErr<CT>("unknown token when expection an expression");
        case tokIdentifier:
            return parseIdentifierExpr();
        case tokNumber:
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
            int binOp = curTok_;

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
            lhs = std::make_unique<BinaryExprAST<CT>>(
                binOp, std::move(lhs), std::move(rhs), env_.get());
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
        if (curTok_ != tokIdentifier) {
            return LogErrP<CT>("expected function name in function prototype");
        }
        std::string fnName(lexer_.getIdentifierStr());

        // '(' before arg list
        getNextToken();
        if (curTok_ != '(')
            return LogErrP<CT>("expected '(' in function prototype");

        // arglist
        std::vector<std::string> argNames;
        while (getNextToken() == tokIdentifier)
            argNames.push_back(lexer_.getIdentifierStr());

        // ')' after arg list
        if (curTok_ != ')')
            return LogErrP<CT>("expected ')' in function prototype");

        getNextToken();
        // finish
        return std::make_unique<PrototypeAST<CT>>(fnName, std::move(argNames),
                                                  env_.get());
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
                                                 env_.get());
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
                "__anon_expr", std::vector<std::string>(), env_.get());
        return std::make_unique<FunctionAST<CT>>(std::move(proto), std::move(E),
                                                 env_.get());
    }

    // helper func----------------------------------------------------

    void initialize() {
        if constexpr (CT == CompilerType::JIT) {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
            llvm::InitializeNativeTargetAsmParser();
        }

        fprintf(stderr, "ready> ");
        getNextToken();

        env_ = std::make_unique<ParserEnv<CT>>();
        env_->initialize();
    }

    int getCurToken() const __attribute__((always_inline)) { return curTok_; }
    ParserEnv<CT> *getEnv() const __attribute__((always_inline)) {
        return env_.get();
    }

private:
    Lexer lexer_;
    int curTok_;
    // this holds the precedence for each binary operator that we define
    static std::map<char, int> binoPrecedence_;
    std::unique_ptr<ParserEnv<CT>> env_;
};

// Install standard binary operators: 1 is lowest precedence
template <CompilerType CT>
inline std::map<char, int> Parser<CT>::binoPrecedence_{
    {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};
