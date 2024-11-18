/*
 * File: lexer.h
 * Path: /lexer.h
 * Module: include
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

enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5,
};

class Lexer {
public:
    // gettok: returns the token from string input
    int gettok();
    double _numVal = .0;
    std::string _identifierStr = "";

private:
    int _lastChar = ' ';
};
