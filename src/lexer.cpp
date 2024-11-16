/*
 * File: lexer.cpp
 * Path: /lexer.cpp
 * Module: src
 * Lang: C/C++
 * Created Date: Tuesday, November 12th 2024, 5:03:16 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
    This file implements a simple lexer.
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */

#include "lexer.h"
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

// gettok: returns the token from string input
int Lexer::gettok() {

    while (std::isspace(_lastChar)) {
        _lastChar = getchar();
    }

    // identifier: [a-zA-Z][a-zA-Z0-9]*
    if (std::isalpha(_lastChar)) {
        _identifierStr = _lastChar;
        while (std::isalnum(_lastChar = getchar())) {
            _identifierStr.push_back(_lastChar);
        }

        if (_identifierStr == "def") {
            return tok_def;
        } else if (_identifierStr == "extern") {
            return tok_extern;
        }
        return tok_identifier;
    }

    // number: [0-9.]*
    if (std::isdigit(_lastChar) || _lastChar == '.') {
        std::string tempNumStr;
        do {
            tempNumStr += _lastChar;
            _lastChar = getchar();
        } while (isdigit(_lastChar) || _lastChar == '.');

        _numVal = strtod(tempNumStr.c_str(), 0);
        return tok_number;
    }

    // annotation
    if (_lastChar == '#') {
        do {
            _lastChar = getchar();
        } while (_lastChar != EOF && _lastChar != '\n' && _lastChar != '\r');
        if (_lastChar != EOF) {
            // process next line
            return gettok();
        }
    }

    if (_lastChar == EOF) {
        return tok_eof;
    }

    int thisChar = _lastChar;
    _lastChar = getchar();
    return thisChar;
}

int main() {
    // testing
    Lexer lexer;
    while (1) {
        int res = lexer.gettok();
        std::cout << res << std::endl;
    }
}
