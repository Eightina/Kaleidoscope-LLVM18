/*
 * File: driver.h
 * Path: /parser/driver.h
 * Module: parser
 * Lang: C/C++
 * Created Date: Saturday, December 14th 2024, 6:08:04 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#include "compiler_type.h"
#include "parser.h"
#include <llvm-18/llvm/Support/Error.h>

template <CompilerType CT> class Driver {
public:
    Driver(Parser<CT> *parser) : parser_(parser), pEnv_(parser->getEnv()) {}
    // high level handling----------------------------------------------------
    void handleDefinition() {
        if (auto defAST = parser_->parseDefinition()) {
            if (auto *defIR = defAST->codegen()) {
                fprintf(stderr, "Parsed a function definition.\n");
                defIR->print(llvm::errs());
                fprintf(stderr, "\n");
                if constexpr (CT == CompilerType::JIT) {
                    // transfer the newly defined function to the JIT
                    //  and open a new module
                    pEnv_->transfer(nullptr);
                }
            }
        } else {
            // Skip token for error recovery.
            parser_->getNextToken();
        }
    }

    void handleExtern() {
        if (auto protoAST = parser_->parseExtern()) {
            if (auto *protoIR = protoAST->codegen()) {
                fprintf(stderr, "Read an extern: ");
                protoIR->print(llvm::errs());
                fprintf(stderr, "\n");
                if constexpr (CT == CompilerType::JIT) {
                    // add the prototype to _functionProtos
                    pEnv_->addProto(protoAST);
                }
            }
        } else {
            // Skip token for error recovery.
            parser_->getNextToken();
        }
    }

    void handleTopLevelExpression() {
        // Evaluate a top-level expression into an anonymous function.]
        llvm::orc::KaleidoscopeJIT *pJIT = pEnv_->getJIT();
        if (auto fnAST = parser_->parseTopLevelExpr()) {
            if (auto *fnIR = fnAST->codegen()) {
                // if (fnAST->codegen()) {
                fprintf(stderr, "Read a top-level expr: ");
                fnIR->print(llvm::errs());
                fprintf(stderr, "\n");
                if constexpr (CT == CompilerType::JIT) {
                    // create a ResourceTracker to track JIT's memory allocated
                    //  to our anonymous expression, which we can free after
                    //  execution
                    auto rt = pJIT->getMainJITDylib().createResourceTracker();

                    pEnv_->transfer(&rt);

                    // search the jit for the _anon_expr symbol
                    llvm::orc::ExecutorSymbolDef exprSymbol =
                        exitOnErr_(pJIT->lookup("__anon_expr"));
                    // assert(exprSymbol && "Function not found");

                    // Get the symbol's address and cast it to the right type
                    // (takes no
                    //  arguments, returns a double) so we can call it as a
                    //  native function.
                    double (*fp)() =
                        exprSymbol.getAddress().toPtr<double (*)()>();
                    fprintf(stderr, "Evaluated to %f\n", fp());

                    // remove the anonymous expression (the whole module) from
                    // the JIT
                    exitOnErr_(rt->remove());
                } else {
                    // remove the anonymous expression
                    fnIR->eraseFromParent();
                }
            }
        } else {
            // Skip token for error recovery.
            parser_->getNextToken();
        }
    }

    //-------------------------------------------------------------------------
    /// top ::= definition | external | expression | ';'
    void mainLoop() {
        while (true) {
            fprintf(stderr, "ready> ");
            switch (parser_->getCurToken()) {
            case tokEof:
                return;
            case ';': // ignore top-level semicolons.
                parser_->getNextToken();
                break;
            case tokDef:
                handleDefinition();
                break;
            case tokExtern:
                handleExtern();
                break;
            default:
                handleTopLevelExpression();
                break;
            }
        }
    }

private:
    Parser<CT> *parser_;
    ParserEnv<CT> *pEnv_;
    llvm::ExitOnError exitOnErr_;
};
