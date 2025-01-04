/*
 * File: utils.h
 * Path: /utils/utils.h
 * Module: utils
 * Lang: C/C++
 * Created Date: Saturday, January 4th 2025, 5:46:24 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _          
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/ 
 */

#include <cstdio>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X);

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT double printd(double X);
