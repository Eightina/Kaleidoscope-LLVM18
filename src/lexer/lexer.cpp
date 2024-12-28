/*
 * File: lexer.cpp
 * Path: /lexer/lexer.cpp
 * Module: lexer
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
#include "token.h"
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

// gettok: returns the token from string input
int Lexer::getTok() {

    while (std::isspace(lastChar_)) {
        lastChar_ = getchar();
    }

    // identifier: [a-zA-Z][a-zA-Z0-9]*
    if (std::isalpha(lastChar_)) {
        identifierStr_ = lastChar_;
        while (std::isalnum(lastChar_ = getchar())) {
            identifierStr_.push_back(lastChar_);
        }

        if (identifierStr_ == "def") {
            return tokDef;
        } else if (identifierStr_ == "extern") {
            return tokExtern;
        }
        return tokIdentifier;

        // if (_identifierStr == )
    }

    // number: [0-9.]*
    if (std::isdigit(lastChar_) || lastChar_ == '.') {
        std::string tempNumStr;
        do {
            tempNumStr += lastChar_;
            lastChar_ = getchar();
        } while (isdigit(lastChar_) || lastChar_ == '.');

        numVal_ = strtod(tempNumStr.c_str(), 0);
        return tokNumber;
    }

    // annotation
    if (lastChar_ == '#') {
        do {
            lastChar_ = getchar();
        } while (lastChar_ != EOF && lastChar_ != '\n' && lastChar_ != '\r');
        if (lastChar_ != EOF) {
            // process next line
            return getTok();
        }
    }

    if (lastChar_ == EOF) {
        return tokEof;
    }

    int thisChar = lastChar_;
    lastChar_ = getchar();
    return thisChar;
}

double Lexer::getNumVal() const {
    return numVal_;
}

const std::string &Lexer::getIdentifierStr() const {
    return identifierStr_;
}

// int main() {
//     // testing
//     Lexer lexer;
//     while (1) {
//         int res = lexer.gettok();
//         std::cout << res << std::endl;
//     }
// }
