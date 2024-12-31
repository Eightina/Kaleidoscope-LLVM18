/*
 * File: parser_env.h
 * Path: /parser/parser_env.h
 * Module: parser
 * Lang: C/C++
 * Created Date: Saturday, December 28th 2024, 1:35:50 pm
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#pragma once
#include "KaleidoSopceJIT.h"
#include "compiler_type.h"
#include "prototype_ast.h"
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Module.h>
#include <llvm-18/llvm/Passes/PassBuilder.h>
#include <llvm-18/llvm/Passes/StandardInstrumentations.h>
#include <llvm-18/llvm/Transforms/AggressiveInstCombine/AggressiveInstCombine.h>
#include <llvm-18/llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm-18/llvm/Transforms/Scalar.h>
#include <llvm-18/llvm/Transforms/Scalar/GVN.h>
#include <llvm-18/llvm/Transforms/Scalar/Reassociate.h>
#include <llvm-18/llvm/Transforms/Scalar/SimplifyCFG.h>
#include <map>
#include <memory>
#include <vector>

template <CompilerType CT> class ParserEnv {
public:
    ParserEnv() : enableOpt_(false) {}
    ParserEnv(bool enableOpt) : enableOpt_(enableOpt) {}

    void initialize() {
        if constexpr (CT == CompilerType::JIT)
            theJIT_ = exitOnErr_(llvm::orc::KaleidoscopeJIT::Create());
        initializeModule();
        if (enableOpt_) initializePassManager();
    }

    void initializeModule() {

        // open a new context and module---------------------------------------
        theContext_ = std::make_unique<llvm::LLVMContext>();
        theModule_ =
            std::make_unique<llvm::Module>("KaleidoScopeJIT", *theContext_);
        if constexpr (CT == CompilerType::JIT)
            theModule_->setDataLayout(theJIT_->getDataLayout());

        // create a new builder for the module---------------------------------
        builder_ = std::make_unique<llvm::IRBuilder<>>(*theContext_);
    }

    void initializePassManager() {
        // create new pass and analysis managers
        theFPM_ = std::make_unique<llvm::FunctionPassManager>();
        theLAM_ = std::make_unique<llvm::LoopAnalysisManager>();
        theFAM_ = std::make_unique<llvm::FunctionAnalysisManager>();
        theCGAM_ = std::make_unique<llvm::CGSCCAnalysisManager>();
        theMAM_ = std::make_unique<llvm::ModuleAnalysisManager>();
        thePIC_ = std::make_unique<llvm::PassInstrumentationCallbacks>();
        theSI_ = std::make_unique<llvm::StandardInstrumentations>(
            *theContext_, /*DebugLogging*/ true);
        theSI_->registerCallbacks(*thePIC_, theMAM_.get());

        // Add transform passes.-----------------------------------------------
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        theFPM_->addPass(llvm::InstCombinePass());
        // _theFPM->addPass(llvm::AggressiveInstCombinePass());
        // Reassociate expressions.
        theFPM_->addPass(llvm::ReassociatePass());
        // Eliminate Common SubExpressions.
        theFPM_->addPass(llvm::GVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        theFPM_->addPass(llvm::SimplifyCFGPass());
        // // Run InstCombine again to catch any new opportunities.
        // _theFPM->addPass(llvm::InstCombinePass());

        // Register analysis passes used in these transform passes.------------
        pb_.registerModuleAnalyses(*theMAM_);
        pb_.registerFunctionAnalyses(*theFAM_);
        pb_.crossRegisterProxies(*theLAM_, *theFAM_, *theCGAM_, *theMAM_);
    }

    // =========================helper funcs===================================
    void runOpt(llvm::Function *theFunction) {
        theFPM_->run(*theFunction, *theFAM_);
    }

    void addProto(std::unique_ptr<PrototypeAST<CT>> &protoAST) {
        functionProtos_[protoAST->getName()] = std::move(protoAST);
    }

    llvm::Function *getFunction(std::string name) {
        // check if the function has been added to the current module
        if (auto *f = theModule_->getFunction(name)) return f;
        // if not, check whether we can codegenthe declaration from prototype
        auto fi = functionProtos_.find(name);
        if (fi != functionProtos_.end()) return fi->second->codegen();

        // if no existing prototype exists, return null;
        return nullptr;
    }

    void clearNamedValues() { namedValues_.clear(); }

    // transfer the newly defined function to the JIT
    //  and open a new module
    void transfer(llvm::orc::ResourceTrackerSP *rt) {
        auto tsm = llvm::orc::ThreadSafeModule(std::move(theModule_),
                                               std::move(theContext_));
        if (rt != nullptr)
            // addModule triggers code generation for all the functions
            //  in the module, and accepts a ResourceTracker which can
            //  be used to remove the module from the JIT later.
            this->exitOnErr_(theJIT_->addModule(std::move(tsm), *rt));
        else {
            this->exitOnErr_(theJIT_->addModule(std::move(tsm)));
        }
        initializeModule();
        if (enableOpt_) initializePassManager();
    }

    // =========================set & get===================================
    llvm::LLVMContext *getContext() const __attribute__((always_inline)) {
        return theContext_.get();
    }

    llvm::IRBuilder<> *getBuilder() const __attribute__((always_inline)) {
        return builder_.get();
    }

    llvm::Module *getModule() __attribute__((always_inline)) {
        return theModule_.get();
    }

    llvm::orc::KaleidoscopeJIT *getJIT() const __attribute__((always_inline)) {
        return theJIT_.get();
    }

    llvm::Value *getValue(std::string &name) __attribute__((always_inline)) {
        return namedValues_[name];
    }

    constexpr bool getEnableOpt() const __attribute__((always_inline)) {
        return enableOpt_;
    }

    void setValue(const std::string &k, llvm::Value *v)
        __attribute__((always_inline)) {
        namedValues_[k] = v;
    }

    void printErr() { theModule_->print(llvm::errs(), nullptr); }

private:
    std::unique_ptr<llvm::LLVMContext> theContext_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::Module> theModule_;
    std::map<std::string, llvm::Value *> namedValues_;
    std::map<std::string, std::unique_ptr<PrototypeAST<CT>>> functionProtos_;

    // for optimizations and JIT
    std::unique_ptr<llvm::FunctionPassManager> theFPM_;
    std::unique_ptr<llvm::LoopAnalysisManager> theLAM_;
    std::unique_ptr<llvm::FunctionAnalysisManager> theFAM_;
    std::unique_ptr<llvm::CGSCCAnalysisManager> theCGAM_;
    std::unique_ptr<llvm::ModuleAnalysisManager> theMAM_;
    std::unique_ptr<llvm::PassInstrumentationCallbacks> thePIC_;
    std::unique_ptr<llvm::StandardInstrumentations> theSI_;
    std::unique_ptr<llvm::orc::KaleidoscopeJIT> theJIT_;
    //
    llvm::PassBuilder pb_;
    llvm::ExitOnError exitOnErr_;

    //
    const bool enableOpt_;
};
