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
#include "lexer.h"
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

// Base class for AST node
class ExprAST {
public:
    virtual ~ExprAST() = default;
};

// Expression class for numeric literals
class NumberExprAST : public ExprAST {
public:
    NumberExprAST(double val) : _val(val) {}

private:
    double _val;
};

// Expression class for variable
class VariableExprAST : public ExprAST {
public:
    VariableExprAST(const std::string &name) : _name(name) {}

private:
    std::string _name;
};

// Expression class for binary operator
class BinaryExprAST : public ExprAST {
public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : _op(op), _LHS(std::move(LHS)), _RHS(std::move(RHS)) {}

private:
    char _op;
    std::unique_ptr<ExprAST> _LHS, _RHS;
};

// Expression class for function calls
class CallExprAST : public ExprAST {
public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args)
        : _callee(callee), _args(std::move(args)) {}

private:
    std::string _callee;
    std::vector<std::unique_ptr<ExprAST>> _args;
};

// This class represents the prototype for a function, including
// its name, arg names, arg number
class PrototypeAST {
public:
    PrototypeAST(const std::string &name, std::vector<std::string> args)
        : _name(name), _args(std::move(args)) {}

    const std::string &getName() const { return _name; }

private:
    std::string _name;
    std::vector<std::string> _args;
};

// This class represents a function definition
class FunctionAST {
public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body)
        : _proto(std::move(proto)), _body(std::move(body)) {}

private:
    std::unique_ptr<PrototypeAST> _proto;
    std::unique_ptr<ExprAST> _body;
};

std::unique_ptr<ExprAST> LogErr(const char *str);
std::unique_ptr<PrototypeAST> LogErrP(const char *str);

class Parser {
public:
    Parser() = default;
    Parser(Lexer &lexer) : _lexer(std::move(lexer)) {}

    void test_lexer();

    int getNextToken();

    int getTokPrecedence();

    // For the parser funcs that are not the beginning of a line(like
    // parseBinOpRHS): e.g. need to parse:    A       B       C func should
    // consider:  ^
    //                        curTok
    // That means we have called getNextToken() for A already,
    //  so the first time we call getNextToken() in this func is for B,
    //  and we need to call it one more time after C to maintain this order.
    //

    // handling basic expression units-----------------------------------------
    std::unique_ptr<ExprAST> parseNumberExpr();
    std::unique_ptr<ExprAST> parseParenExpr();
    std::unique_ptr<ExprAST> parseIdentifierExpr();
    std::unique_ptr<ExprAST> parsePrimary();
    //-------------------------------------------------------------------------

    // handling binary expressions---------------------------------------------
    std::unique_ptr<ExprAST> parseBinOpRHS(int exprPrec,
                                           std::unique_ptr<ExprAST> lhs);
    std::unique_ptr<ExprAST> parseExpression();
    //-------------------------------------------------------------------------

    // handling fucntion prototypes--------------------------------------------
    // prototype ::= id '(' id* ')'
    std::unique_ptr<PrototypeAST> parsePrototype();
    /// definition ::= 'def' prototype expression
    std::unique_ptr<FunctionAST> parseDefinition();
    /// external ::= 'extern' prototype
    std::unique_ptr<PrototypeAST> parseExtern();
    //-------------------------------------------------------------------------

    /// toplevelexpr ::= expression
    std::unique_ptr<FunctionAST> parseTopLevelExpr();
    //-------------------------------------------------------------------------

    // error handling-------------------------------------------------------
    void handleDefinition();
    void handleExtern();
    void handleTopLevelExpression();
    //-------------------------------------------------------------------------
    void mainLoop();

private:
    Lexer _lexer;
    int _curTok;
    // this holds the precedence for each binary operator that we define
    static std::map<char, int> _binoPrecedence;
};
