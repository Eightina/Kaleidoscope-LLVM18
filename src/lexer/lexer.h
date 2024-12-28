/*
 * File: lexer.h
 * Path: /lexer/lexer.h
 * Module: lexer
 * Lang: C/C++
 * Created Date: Saturday, November 16th 2024, 10:47:20 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
    This file is the header of lexer.cpp.
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#pragma once
#include <string>
class Lexer {
public:
    // gettok: returns the token from string input
    int getTok();
    double getNumVal() const __attribute__((always_inline));
    const std::string &getIdentifierStr() const __attribute__((always_inline));

private:
    int lastChar_ = ' ';
    double numVal_ = .0;
    std::string identifierStr_ = "";
};
