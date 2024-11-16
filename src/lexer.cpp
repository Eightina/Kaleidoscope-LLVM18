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

#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5,
};

static std::string identifierStr;
static double numVal;

// gettok: returns the token from string input
static int gettok() {
    static int lastChar = ' ';

    while (std::isspace(lastChar)) {
        lastChar = getchar();
    }

    // identifier: [a-zA-Z][a-zA-Z0-9]*
    if (std::isalpha(lastChar)) {
        identifierStr = lastChar;
        while (std::isalnum(lastChar = getchar())) {
            identifierStr.push_back(lastChar);
        }

        if (identifierStr == "def") {
            return tok_def;
        } else if (identifierStr == "extern") {
            return tok_extern;
        }
        return tok_identifier;
    }

    // number: [0-9.]*
    if (std::isdigit(lastChar) || lastChar == '.') {
        std::string tempNumStr;
        do {
            tempNumStr += lastChar;
            lastChar = getchar();
        } while (isdigit(lastChar) || lastChar == '.');

        numVal = strtod(tempNumStr.c_str(), 0);
        return tok_number;
    }

    if (lastChar == '#') {
        do {
            lastChar = getchar();
        } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
        if (lastChar != EOF) {
            // process next line
            return gettok();
        }
    }

    if (lastChar == EOF) {
        return tok_eof;
    }

    int thisChar = lastChar;
    lastChar = getchar();
    return thisChar;
}

int main() {
    // testing
    while (1) {
        int res = gettok();
        std::cout << res << std::endl;
    }
}
