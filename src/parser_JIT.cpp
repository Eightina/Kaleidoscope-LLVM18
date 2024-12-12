/*
 * File: parser.txt
 * Path: /parser.txt
 * Module: src
 * Created Date: Saturday, November 16th 2024, 11:27:40 am
 * Author: orion
 * Email: orion.que@outlook.com
 * ----------------------------------------------
    This file builds a REPL with AST with recursive descent parsing
    & operator-precedence parsing, and then generate code with LLVM backend.
    
    !!!==============!!!Warning:!!!==============!!!
    Some of the code here tries to make functions redefineable, like the IO below.
    We try to start new a module everytime we complete a high level handling, so that 
    we expect the symbols can be named the same in a new module without conflicts.
    However, duplication of symbols in separate modules is not allowed since LLVM-9. That
    means you can not redefine function in your Kaleidoscope as its shown below.
    Just skip this part.

    The reason is that the newer OrcV2 JIT APIs are trying to stay very close to the
    static and dynamic linker rules, including rejecting duplicate symbols.
    Requiring symbol names to be unique allows us to support concurrent compilation
    for symbols using the (unique) symbol names as keys for tracking.

    ''' this is ilegal now
    //ready> def foo(x) x + 1;
    //ready> foo(2);
    //Evaluated to 3.000000
    //ready> def foo(x) x + 2;
    //ready> foo(2);
    //Evaluated to 4.000000
    '''
 *    ____                  __  _
     / __/__ _  _____ ___  / /_(_)__  ___ _
    _\ \/ -_) |/ / -_) _ \/ __/ / _ \/ _ `/
   /___/\__/|___/\__/_//_/\__/_/_//_/\_,_/
 */
#include "parser_JIT.h"

std::unique_ptr<ExprAST> LogErr(const char *str) {
    std::cout << stderr << "Error: " << str << std::endl;
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrP(const char *str) {
    LogErr(str);
    return nullptr;
}

llvm::Value *LogErrorV(const char *str) {
    LogErr(str);
    return nullptr;
}

// =====================================================================
// codegen
// =====================================================================

llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(*(owner->_theContext), llvm::APFloat(_val));
}

llvm::Value *VariableExprAST::codegen() {
    llvm::Value *V = owner->_namedValues[_name];
    if (!V) LogErrorV("unknown variable name");
    return V;
}

llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *l = _lhs->codegen();
    llvm::Value *r = _rhs->codegen();
    if (!l || !r) return nullptr;

    switch (_op) {
    case ('+'):
        // we give each val IR a name
        return owner->_builder->CreateFAdd(l, r, "addtmp");
    case ('-'):
        return owner->_builder->CreateFSub(l, r, "subtmp");
    case ('*'):
        return owner->_builder->CreateFMul(l, r, "multmp");
    case ('<'):
        l = owner->_builder->CreateFCmpULT(l, r, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return owner->_builder->CreateUIToFP(
            l, llvm::Type::getDoubleTy(*owner->_theContext), "booltmp");
    default:
        return LogErrorV("invalid binary op");
    }
}

llvm::Value *CallExprAST::codegen() {
    // find the name int the global module table
    // llvm::Function *calleeF = owner->_theModule->getFunction(_callee);
    llvm::Function *calleeF = owner->getFunction(_callee);
    if (!calleeF) return LogErrorV("unknown function referenced");
    if (calleeF->arg_size() != _args.size())
        return LogErrorV("incorrect number of args passed");

    std::vector<llvm::Value *> argsV;
    for (unsigned i = 0, e = _args.size(); i != e; ++i) {
        argsV.push_back(_args[i]->codegen());
        if (!argsV.back()) return nullptr;
    }

    return owner->_builder->CreateCall(calleeF, argsV, "calltmp");
}

llvm::Function *PrototypeAST::codegen() {
    // create the arguments list for the function prototype
    std::vector<llvm::Type *> doubles(
        _args.size(), llvm::Type::getDoubleTy(*owner->_theContext));
    // make the function type: double(double, double), etc.
    llvm::FunctionType *FT = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*owner->_theContext), doubles, false);
    // codegen for function prototype. “external linkage” means that the
    //  function may be defined outside the current module and/or that it is
    //  callable by functions outside the module. Name passed in is the name the
    //  user specified: since "TheModule" is specified, this name is registered
    //  in "TheModule"s symbol table.
    llvm::Function *F = llvm::Function::Create(
        FT, llvm::Function::ExternalLinkage, _name, owner->_theModule.get());
    unsigned Idx = 0;
    for (auto &arg : F->args()) {
        arg.setName(_args[Idx++]);
    }
    return F;
}

llvm::Function *FunctionAST::codegen() {
    // In JIT, we dont mind redefinition or whatsoever.
    // Firstly, transfer ownership of the prototype to the FunctionProtos map,
    //  but keep a reference to it for use below.
    auto &p = *_proto;
    owner->_functionProtos[_proto->getName()] = std::move(_proto);
    llvm::Function *theFunction = owner->getFunction(p.getName());
    if (!theFunction) return nullptr;

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *bb =
        llvm::BasicBlock::Create(*owner->_theContext, "entry", theFunction);
    owner->_builder->SetInsertPoint(bb);

    // record the function arguments in the namedvalues table
    owner->_namedValues.clear();
    for (auto &arg : theFunction->args()) {
        owner->_namedValues[std::string(arg.getName())] = &arg;
    }
    if (llvm::Value *retVal = _body->codegen()) {
        owner->_builder->CreateRet(retVal);
        llvm::verifyFunction(*theFunction);
        // after verifying consistency, do optimizations
        owner->runOpt(theFunction);
        return theFunction;
    }

    // deleting the function we produced. later we may redefine a function that
    //  we incorrectly typed in before: if we didn’t delete it, it would live
    //  in the symbol table, with a body, preventing future redefinition.
    theFunction->eraseFromParent();
    return nullptr;
}

// =====================================================================
// Parser
// =====================================================================

// std::unique_ptr<llvm::LLVMContext> Parser::_theContext = nullptr;
// std::unique_ptr<llvm::Module> Parser::_theModule = nullptr;
// std::unique_ptr<llvm::IRBuilder<>> Parser::_builder = nullptr;
// std::map<std::string, llvm::Value *> Parser::_namedValues;

Parser::Parser() {
    initialize();
}

Parser::Parser(Lexer &lexer) : _lexer(std::move(lexer)) {
    initialize();
}

int Parser::getNextToken() {
    return _curTok = _lexer.gettok();
}

void Parser::test_lexer() {
    while (true) {
        getNextToken();
    }
}

/// numberexpr ::= number
std::unique_ptr<ExprAST> Parser::parseNumberExpr() {
    auto result = std::make_unique<NumberExprAST>(_lexer._numVal, this);
    getNextToken();
    return std::move(result);
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> Parser::parseParenExpr() {
    getNextToken();
    auto v = parseExpression();
    if (!v) return nullptr;
    if (_curTok != ')') return LogErr("expected ')'");
    getNextToken();
    return v;
}

// identifierexpr
// ::= identifier
// ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> Parser::parseIdentifierExpr() {
    std::string idName = _lexer._identifierStr;
    getNextToken(); // take in identifier
    if (_curTok != '(') return std::make_unique<VariableExprAST>(idName, this);

    getNextToken(); // take in '('
    std::vector<std::unique_ptr<ExprAST>> args;
    if (_curTok != ')') { // take in expression (args)
        while (true) {
            if (auto arg = parseExpression())
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (_curTok == ')') break;

            if (_curTok != ',')
                return LogErr("expected ')' or ',' in arg list");

            getNextToken(); // take in ','
        }
    }

    getNextToken(); // take in ')'
    return std::make_unique<CallExprAST>(idName, std::move(args), this);
}

/// primary
/// ::= identifierexpr
/// ::= numberexpr
/// ::= parenexpr
std::unique_ptr<ExprAST> Parser::parsePrimary() {
    switch (_curTok) {
    default:
        return LogErr("unknown token when expection an expression");
    case tok_identifier:
        return parseIdentifierExpr();
    case tok_number:
        return parseNumberExpr();
    case '(':
        return parseParenExpr();
    }
}

// In order to parse binary expression correctly:
//  "x+y*z" -> "x+(y*z)"
//  uses the precedence of binary operators to guide recursion
//  Operator-Precedence Parsing
int Parser::getTokPrecedence() {
    if (!isascii(_curTok)) return -1;

    // make sure it is a declared binop
    int tokPrec = _binoPrecedence[_curTok];
    if (tokPrec <= 0) return -1;
    return tokPrec;
}

//
/// binoprhs
/// ::= ('+' primary)*
/*
e.g. When parsing a + b + (c + d):
    lhs == a
    curTok == +
    then parse the rest to find possible rhs
    rhs == parsePrimary() == b
    now curTok == + (the second) and its prec is nextPrec
    then if (tokPrec < nextPrec):
        we move on to parse b + (c + d) as a whole rhs by recursion call
        that is a + (b + (c + d))
    else:
        we combine (a + b) as lhs
        that is (a + b) + (c + d)
*/
std::unique_ptr<ExprAST> Parser::parseBinOpRHS(int exprPrec,
                                               std::unique_ptr<ExprAST> lhs) {
    // The precedence value passed into ParseBinOpRHS indicates the minimal
    //  operator precedence that the function is allowed to eat
    while (true) {
        int tokPrec = getTokPrecedence();
        // guard: checking current precedence and detect whether
        //  the token runs out of operators
        if (tokPrec < exprPrec) return lhs;
        // else we know that this token is binary and should be included
        int binOp = _curTok;

        // Then get next token and parse the primary expression
        //  after the binary operator
        getNextToken();
        auto rhs = parsePrimary();
        if (!rhs) return nullptr;
        // If binOp binds less tightly with RHS than the operator after RHS, let
        //  the pending operator take RHS as its LHS.
        int nextPrec = getTokPrecedence();
        if (tokPrec < nextPrec) {
            rhs = parseBinOpRHS(tokPrec + 1, std::move(rhs));
            if (!rhs) return nullptr;
        }

        // Merge LHS/RHS.
        lhs = std::make_unique<BinaryExprAST>(binOp, std::move(lhs),
                                              std::move(rhs), this);
        // This will turn "a+b+" into "(a+b)" and execute the next iteration of
        //  the loop, with "+" as the current token
    }
    // loop around to the top of the while loop.
}

/// expression
/// ::= primary binoprhs
std::unique_ptr<ExprAST> Parser::parseExpression() {
    auto lhs = parsePrimary();
    if (lhs == nullptr) return nullptr;

    return parseBinOpRHS(0, std::move(lhs));
}

/// prototype
/// ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
    // func name
    if (_curTok != tok_identifier) {
        return LogErrP("expected function name in function prototype");
    }
    std::string fnName = _lexer._identifierStr;

    // '(' before arg list
    getNextToken();
    if (_curTok != '(') return LogErrP("expected '(' in function prototype");

    // arglist
    std::vector<std::string> argNames;
    while (getNextToken() == tok_identifier)
        argNames.push_back(_lexer._identifierStr);

    // ')' after arg list
    if (_curTok != ')') return LogErrP("expected ')' in function prototype");

    getNextToken();
    // finish
    return std::make_unique<PrototypeAST>(fnName, std::move(argNames), this);
    // notice:
    //  If we want to move the argNames out, we need to call std::move
    //  here. The construction func of PrototypeAST receives
    //  a 'std::vector<std::string> args', so it needs to create an instance
    //  inside the func. If we return (fnName, argNames) here, argNames is a
    //  lval ref, and the construction func of PrototypeAST will call the copy
    //  construction func for it.
    //
    // Why:?
    //  Why we dont use 'std::make_unique<PrototypeAST>(fnName, argNames);'
    //  with 'PrototypeAST(const std::string &name, std::vector<std::string>&
    //              args): _name(name), _args(std::move(args)) {}'?
    // Because: This is not safe! When we call this construction func, the
    //  argNames we pass in will be invalid after the call!
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> Parser::parseDefinition() {
    // begin of the line, take in def
    getNextToken();
    auto proto = parsePrototype();
    if (!proto) return nullptr;

    auto E = parseExpression();
    if (!E) return nullptr;
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E), this);
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> Parser::parseExtern() {
    getNextToken();
    return parsePrototype();
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> Parser::parseTopLevelExpr() {
    auto E = parseExpression();
    if (!E) return nullptr;

    // Make an anonymous proto
    auto proto =
        // std::make_unique<PrototypeAST>("", std::vector<std::string>(), this);
        std::make_unique<PrototypeAST>("__anon_expr",
                                       std::vector<std::string>(), this);
    return std::make_unique<FunctionAST>(std::move(proto), std::move(E), this);
}

// =====================================================================
// Top level modules and drivers
// =====================================================================
void Parser::handleDefinition() {
    if (auto defAST = parseDefinition()) {
        if (auto *defIR = defAST->codegen()) {
            fprintf(stderr, "Parsed a function definition.\n");
            defIR->print(llvm::errs());
            fprintf(stderr, "\n");
            // transfer the newly defined function to the JIT
            //  and open a new module
            auto tsm = llvm::orc::ThreadSafeModule(std::move(_theModule),
                                                   std::move(_theContext));
            _exitOnErr(_theJIT->addModule(std::move(tsm)));
            initializeModuleAndPassManager();
        }
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void Parser::handleExtern() {
    if (auto protoAST = parseExtern()) {
        if (auto *protoIR = protoAST->codegen()) {
            fprintf(stderr, "Read an extern: ");
            protoIR->print(llvm::errs());
            fprintf(stderr, "\n");
            // add the prototype to _functionProtos
            _functionProtos[protoAST->getName()] = std::move(protoAST);
        }
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void Parser::handleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (auto fnAST = parseTopLevelExpr()) {
        if (auto *fnIR = fnAST->codegen()) {
            // if (fnAST->codegen()) {
            fprintf(stderr, "Read a top-level expr: ");
            fnIR->print(llvm::errs());
            fprintf(stderr, "\n");
            // create a ResourceTracker to track JIT's memory allocated to
            //  our anonymous expression, which we can free after execution
            auto rt = _theJIT->getMainJITDylib().createResourceTracker();
            auto tsm = llvm::orc::ThreadSafeModule(std::move(_theModule),
                                                   std::move(_theContext));

            // addModule triggers code generation for all the functions
            //  in the module, and accepts a ResourceTracker which can be used
            //  to remove the module from the JIT later.
            _exitOnErr(_theJIT->addModule(std::move(tsm), rt));
            initializeModuleAndPassManager();

            // search the jit for the _anon_expr symbol
            auto exprSymbol = _exitOnErr(_theJIT->lookup("__anon_expr"));
            // assert(exprSymbol && "Function not found");

            // Get the symbol's address and cast it to the right type (takes no
            //  arguments, returns a double) so we can call it as a native
            //  function.
            double (*fp)() = exprSymbol.getAddress().toPtr<double (*)()>();
            fprintf(stderr, "Evaluated to %f\n", fp());

            // remove the anonymous expression (the whole module) from the JIT
            _exitOnErr(rt->remove());
        }
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void Parser::runOpt(llvm::Function *theFunction) {
    _theFPM->run(*theFunction, *_theFAM);
}

void Parser::initializeModuleAndPassManager() {
    // open a new context and module---------------------------------------
    _theContext = std::make_unique<llvm::LLVMContext>();
    _theModule =
        std::make_unique<llvm::Module>("KaleidoScopeJIT", *_theContext);
    _theModule->setDataLayout(_theJIT->getDataLayout());

    // create a new builder for the module---------------------------------
    _builder = std::make_unique<llvm::IRBuilder<>>(*_theContext);
    // create new pass and analysis managers
    _theFPM = std::make_unique<llvm::FunctionPassManager>();
    _theLAM = std::make_unique<llvm::LoopAnalysisManager>();
    _theFAM = std::make_unique<llvm::FunctionAnalysisManager>();
    _theCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
    _theMAM = std::make_unique<llvm::ModuleAnalysisManager>();
    _thePIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
    _theSI = std::make_unique<llvm::StandardInstrumentations>(
        *_theContext, /*DebugLogging*/ true);
    _theSI->registerCallbacks(*_thePIC, _theMAM.get());

    // Add transform passes.-----------------------------------------------
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    _theFPM->addPass(llvm::InstCombinePass());
    // _theFPM->addPass(llvm::AggressiveInstCombinePass());
    // Reassociate expressions.
    _theFPM->addPass(llvm::ReassociatePass());
    // Eliminate Common SubExpressions.
    _theFPM->addPass(llvm::GVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    _theFPM->addPass(llvm::SimplifyCFGPass());
    // // Run InstCombine again to catch any new opportunities.
    // _theFPM->addPass(llvm::InstCombinePass());

    // Register analysis passes used in these transform passes.------------
    _pb.registerModuleAnalyses(*_theMAM);
    _pb.registerFunctionAnalyses(*_theFAM);
    _pb.crossRegisterProxies(*_theLAM, *_theFAM, *_theCGAM, *_theMAM);
}

void Parser::initialize() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Prime the first token.
    fprintf(stderr, "ready> ");
    getNextToken();

    _theJIT = _exitOnErr(llvm::orc::KaleidoscopeJIT::Create());
    initializeModuleAndPassManager();
}

llvm::Function *Parser::getFunction(std::string name) {
    // check if the function has been added to the current module
    if (auto *f = _theModule->getFunction(name)) return f;
    // if not, check whether we can codegenthe declaration from prototype
    auto fi = _functionProtos.find(name);
    if (fi != _functionProtos.end()) return fi->second->codegen();

    // if no existing prototype exists, return null;
    return nullptr;
}

/// top ::= definition | external | expression | ';'
void Parser::mainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (_curTok) {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            getNextToken();
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

// Install standard binary operators: 1 is lowest precedence
std::map<char, int> Parser::_binoPrecedence{
    {'<', 10}, {'+', 20}, {'-', 20}, {'*', 40}};

int main() {
    // Install standard binary operators.
    // 1 is lowest precedence.
    // llvm::InitializeAllTargets();
    // llvm::InitializeAllTargetMCs();
    // llvm::InitializeAllAsmPrinters();
    // llvm::InitializeAllAsmParsers();

    Parser p;

    // Run the main "interpreter loop" now.
    p.mainLoop();

    p._theModule->print(llvm::errs(), nullptr);

    return 0;
}
