/*
 * File: prototype_op_ast.h
 * Path: /ast/prototype_op_ast.h
 * Module: ast
 * Lang: C/C++
 * Created Date: Monday, January 27th 2025, 7:51:59 pm
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
#include "expr_ast.h"
#include "prototype_ast.h"

template <CompilerType CT> class BinaryOperatorAST : public PrototypeAST<CT> {
public:
    BinaryOperatorAST(const std::string &name, std::vector<std::string> args,
                      unsigned precedence, ParserEnv<CT> *env)
        : PrototypeAST<CT>(name, args, env), precedence_(precedence) {
        this->thisType_ = PrototypeType::Binary;
    }
    unsigned getBinaryPrecedence() const { return precedence_; }

protected:
    unsigned precedence_;
};

template <CompilerType CT> class UnaryOperatorAST : public PrototypeAST<CT> {
public:
    UnaryOperatorAST(const std::string &name, std::vector<std::string> args,
                     ParserEnv<CT> *env)
        : PrototypeAST<CT>(name, args, env) {
        this->thisType_ = PrototypeType::Unary;
    }
};