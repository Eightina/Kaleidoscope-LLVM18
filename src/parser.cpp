/*
 * File: parser.txt
 * Path: /parser.txt
 * Module: src
 * Created Date: Saturday, November 16th 2024, 11:27:40 am
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
    This file builds AST with recursive descent parsing
    & operator-precedence parsing.
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#include "parser.h"
#include <memory>

std::unique_ptr<ExprAST> LogErr(const char *str) {
    std::cout << stderr << "Error: " << str << std::endl;
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrP(const char *str) {
    LogErr(str);
    return nullptr;
}

int Parser::getNextToken() {
    return _curTok = _lexer.gettok();
}

void Parser::test_lexer() {
    while (true) {
        getNextToken();
    }
}

/// numberexpr ::= number
std::unique_ptr<ExprAST> Parser::parseNumberExpr() {
    auto result = std::make_unique<NumberExprAST>(_lexer._numVal);
    getNextToken();
    return std::move(result);
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> Parser::parseParenExpr() {
    getNextToken();
    auto v = parseExpression();
    if (!v) return nullptr;
    if (_curTok != ')') return LogErr("expected ')'");
    getNextToken();
    return v;
}

// identifierexpr
// ::= identifier
// ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> Parser::parseIdentifierExpr() {
    std::string idName = _lexer._identifierStr;
    getNextToken(); // take in identifier
    if (_curTok != '(') return std::make_unique<VariableExprAST>(idName);

    getNextToken(); // take in '('
    std::vector<std::unique_ptr<ExprAST>> args;
    if (_curTok != ')') { // take in expression (args)
        while (true) {
            if (auto arg = parseExpression())
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (_curTok == ')') break;

            if (_curTok != ',')
                return LogErr("expected ')' or ',' in arg list");

            getNextToken(); // take in ','
        }
    }

    getNextToken(); // take in ')'
    return std::make_unique<CallExprAST>(idName, std::move(args));
}

/// primary
/// ::= identifierexpr
/// ::= numberexpr
/// ::= parenexpr
std::unique_ptr<ExprAST> Parser::parsePrimary() {
    switch (_curTok) {
    default:
        return LogErr("unknown token when expection an expression");
    case tok_identifier:
        return parseIdentifierExpr();
    case tok_number:
        return parseNumberExpr();
    case '(':
        return parseParenExpr();
    }
}

// In order to parse binary expression correctly:
//  "x+y*z" -> "x+(y*z)"
//  uses the precedence of binary operators to guide recursion
//  Operator-Precedence Parsing
int Parser::getTokPrecedence() {
    if (!isascii(_curTok)) return -1;

    // make sure it is a declared binop
    int tokPrec = _binoPrecedence[_curTok];
    if (tokPrec <= 0) return -1;
    return tokPrec;
}
// Install standard binary operators: 1 is lowest precedence
std::map<char, int> Parser::_binoPrecedence{
    {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};

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
std::unique_ptr<ExprAST> Parser::parseBinOpRHS(int exprPrec,
                                               std::unique_ptr<ExprAST> lhs) {
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
        // If binOp binds less tightly with RHS than the operator after RHS, let
        //  the pending operator take RHS as its LHS.
        int nextPrec = getTokPrecedence();
        if (tokPrec < nextPrec) {
            rhs = parseBinOpRHS(tokPrec + 1, std::move(rhs));
            if (!rhs) return nullptr;
        }

        // Merge LHS/RHS.
        lhs = std::make_unique<BinaryExprAST>(binOp, std::move(lhs),
                                              std::move(rhs));
        // This will turn "a+b+" into "(a+b)" and execute the next iteration of
        //  the loop, with "+" as the current token
    }
    // loop around to the top of the while loop.
}

/// expression
/// ::= primary binoprhs
std::unique_ptr<ExprAST> Parser::parseExpression() {
    auto lhs = parsePrimary();
    if (lhs == nullptr) return nullptr;

    return parseBinOpRHS(0, std::move(lhs));
}

/// prototype
/// ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
    // func name
    if (_curTok != tok_identifier) {
        return LogErrP("expected function name in function prototype");
    }
    std::string fnName = _lexer._identifierStr;

    // '(' before arg list
    getNextToken();
    if (_curTok != '(') return LogErrP("expected '(' in function prototype");

    // arglist
    std::vector<std::string> argNames;
    while (getNextToken() == tok_identifier)
        argNames.push_back(_lexer._identifierStr);

    // ')' after arg list
    if (_curTok != ')') return LogErrP("expected ')' in function prototype");

    getNextToken();
    // finish
    return std::make_unique<PrototypeAST>(fnName, std::move(argNames));
    // notice:
    //  If we want to move the argNames out, we need to call std::move
    //  here. The construction func of PrototypeAST receives
    //  a 'std::vector<std::string> args', so it needs to create an instance
    //  inside the func. If we return (fnName, argNames) here, argNames is a
    //  lval ref, and the construction func of PrototypeAST will call the copy
    //  construction func for it.
    //
    // Why:?
    //  Why we dont use 'std::make_unique<PrototypeAST>(fnName, argNames);'
    //  with 'PrototypeAST(const std::string &name, std::vector<std::string>&
    //              args): _name(name), _args(std::move(args)) {}'?
    // Because: This is not safe! When we call this construction func, the
    //  argNames we pass in will be invalid after the call!
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> Parser::parseDefinition() {
    // begin of the line, take in def
    getNextToken();
    auto proto = parsePrototype();
    if (!proto) return nullptr;

    auto E = parseExpression();
    if (!E) return nullptr;
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> Parser::parseExtern() {
    getNextToken();
    return parsePrototype();
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> Parser::parseTopLevelExpr() {
    auto E = parseExpression();
    if (!E) return nullptr;

    // Make an anonymous proto
    auto proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
}

void Parser::handleDefinition() {
    if (parseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void Parser::handleExtern() {
    if (parseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void Parser::handleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (parseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
void Parser::mainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (_curTok) {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            getNextToken();
            break;
        case tok_def:
            handleDefinition();
            break;
        case tok_extern:
            handleExtern();
            break;
        default:
            handleTopLevelExpression();
            break;
        }
    }
}

// int main() {
//     std::cout << "parser_test" << std::endl;
//     Parser parser;
//     parser.test_lexer();
// }
int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  Parser p;

  // Prime the first token.
  fprintf(stderr, "ready> ");
  p.getNextToken();

  // Run the main "interpreter loop" now.
  p.mainLoop();

  return 0;
}