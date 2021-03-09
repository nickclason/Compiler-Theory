//
// Created by Nick Clason on 2/1/21.
//

#ifndef COMPILER_THEORY_PARSER_H
#define COMPILER_THEORY_PARSER_H

#include "../include/definitions.h"
#include "../include/Scanner.h"
#include "../include/Symbol.h"
#include "../include/SymbolTable.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

class Parser
{
public:

    Parser(std::string fileName, bool debug_, Scanner scanner_, SymbolTable symbolTable_, token_t *token_);
    ~Parser();

    void Program();
    void InitLLVM();

private:
    Scanner scanner;
    SymbolTable symbolTable;
    token_t *token;

    int procedureCount;

    bool debug;
    bool stopParse;
    bool errorFlag;

    llvm::Module *llvmModule;
    llvm::IRBuilder<> *llvmBuilder;
    llvm::LLVMContext llvmContext;
    llvm::Function *llvmCurrProc;


    // General/Utility Functions
    bool ValidateToken(int tokenType);

    void PrintDebugInfo(std::string langID);

    void YieldError(token_t token);
    void YieldError(std::string msg, token_t token);
    void YieldMissingTokenError(std::string expected, token_t token);
    void YieldTypeMismatchError(std::string expected, std::string actual, token_t token);
    void YieldOpTypeCheckError(std::string op, std::string type1, std::string type2, token_t token);

    void YieldWarning(std::string msg, token_t token);

    void Resync(token_t tokens[], int length);


    // Parsing
    void ProgramHeader();
    void ProgramBody();

    void Declaration();
    void WhileDeclarations();
    void ProcedureDeclaration(Symbol &procedure);
    void ProcedureHeader(Symbol &procedure);
    void ParameterList(Symbol &procedure);
    void Parameter(Symbol &procedure);
    void ProcedureBody(Symbol &procedure);
    void VariableDeclaration(Symbol &variable);
    void TypeMark(Symbol &symbol);
    void Bound(Symbol &symbol);
    void Statement();
    void AssignmentStatement();
    void IfStatement();
    void LoopStatement();
    void ReturnStatement();

    Symbol ProcedureCall();
    Symbol Destination();
    Symbol Expression(Symbol expectedType);
    Symbol ExpressionTail(Symbol expectedType);
    Symbol ArithOp(Symbol expectedType);
    Symbol ArithOpTail(Symbol expectedType);
    Symbol Relation(Symbol expectedType);
    Symbol RelationTail(Symbol expectedType);
    Symbol Term(Symbol expectedType);
    Symbol TermTail(Symbol expectedType);
    Symbol Factor(Symbol expectedType);
    Symbol Name();
    Symbol Number();
    Symbol String();

    std::string Identifier();

    std::vector<llvm::Value *> ArgumentList(std::vector<Symbol>::iterator curr, std::vector<Symbol>::iterator end);

    // LLVM
    llvm::Type* GetLLVMType(Symbol symbol);
};

#endif //COMPILER_THEORY_PARSER_H
