/*
 * File: driver.h
 * Path: /driver.h
 * Module: src
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
    Driver(Parser<CT> *parser) : _parser(parser) {}
    // high level handling----------------------------------------------------
    void handleDefinition() {
        if (auto defAST = _parser->parseDefinition()) {
            if (auto *defIR = defAST->codegen()) {
                fprintf(stderr, "Parsed a function definition.\n");
                defIR->print(llvm::errs());
                fprintf(stderr, "\n");
                if constexpr (CT == CompilerType::JIT) {
                    // transfer the newly defined function to the JIT
                    //  and open a new module
                    auto tsm = llvm::orc::ThreadSafeModule(
                        std::move(_parser->_theModule),
                        std::move(_parser->_theContext));
                    this->_exitOnErr(
                        _parser->_theJIT->addModule(std::move(tsm)));
                    _parser->initializeModuleAndPassManager();
                }
            }
        } else {
            // Skip token for error recovery.
            _parser->getNextToken();
        }
    }

    void handleExtern() {
        if (auto protoAST = _parser->parseExtern()) {
            if (auto *protoIR = protoAST->codegen()) {
                fprintf(stderr, "Read an extern: ");
                protoIR->print(llvm::errs());
                fprintf(stderr, "\n");
                if constexpr (CT == CompilerType::JIT) {
                    // add the prototype to _functionProtos
                    _parser->_functionProtos[protoAST->getName()] =
                        std::move(protoAST);
                }
            }
        } else {
            // Skip token for error recovery.
            _parser->getNextToken();
        }
    }

    void handleTopLevelExpression() {
        // Evaluate a top-level expression into an anonymous function.
        if (auto fnAST = _parser->parseTopLevelExpr()) {
            if (auto *fnIR = fnAST->codegen()) {
                // if (fnAST->codegen()) {
                fprintf(stderr, "Read a top-level expr: ");
                fnIR->print(llvm::errs());
                fprintf(stderr, "\n");
                if constexpr (CT == CompilerType::JIT) {
                    // create a ResourceTracker to track JIT's memory allocated
                    // to
                    //  our anonymous expression, which we can free after
                    //  execution
                    auto rt = _parser->_theJIT->getMainJITDylib()
                                  .createResourceTracker();
                    auto tsm = llvm::orc::ThreadSafeModule(
                        std::move(_parser->_theModule),
                        std::move(_parser->_theContext));

                    // addModule triggers code generation for all the functions
                    //  in the module, and accepts a ResourceTracker which can
                    //  be used to remove the module from the JIT later.
                    _exitOnErr(_parser->_theJIT->addModule(std::move(tsm), rt));
                    _parser->initializeModuleAndPassManager();

                    // search the jit for the _anon_expr symbol
                    llvm::orc::ExecutorSymbolDef exprSymbol =
                        _exitOnErr(_parser->_theJIT->lookup("__anon_expr"));
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
                    _exitOnErr(rt->remove());
                } else {
                    // remove the anonymous expression
                    fnIR->eraseFromParent();
                }
            }
        } else {
            // Skip token for error recovery.
            _parser->getNextToken();
        }
    }

    //-------------------------------------------------------------------------
    /// top ::= definition | external | expression | ';'
    void mainLoop() {
        while (true) {
            fprintf(stderr, "ready> ");
            switch (_parser->_curTok) {
            case tok_eof:
                return;
            case ';': // ignore top-level semicolons.
                _parser->getNextToken();
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

private:
    Parser<CT> *_parser;
    llvm::ExitOnError _exitOnErr;
};