/*
 * File: token.h
 * Path: /lexer/token.h
 * Module: lexer
 * Lang: C/C++
 * Created Date: Saturday, December 14th 2024, 12:54:34 am
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#pragma once

enum Token {
    tokEof = -1,
    tokDef = -2,
    tokExtern = -3,
    tokIdentifier = -4,
    tokNumber = -5,
    tokIf = -6,
    tokThen = -7,
    tokElse = -8,
    tokFor = -9,
    tokDo = -10,
    tokBinary = -11,
    tokUnary = -12
};
