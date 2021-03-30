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

    Parser(bool debug_, Scanner scanner_, SymbolTable symbolTable_, token_t *token_);
    ~Parser();

    void Program();
    void InitLLVM();

private:
    Scanner scanner;
    SymbolTable symbolTable;
    token_t *token;

    int procedureCount;
    int errorCount;
    int warningCount;
    int unwrapSize;


    bool debug;
    bool doCompile;
    bool stopParse;
    bool errorFlag;
    bool unwrap;

    llvm::Module *llvmModule;
    llvm::IRBuilder<> *llvmBuilder;
    llvm::LLVMContext llvmContext;
    llvm::Function *llvmCurrProc;
    llvm::Value *llvmUnwrapIdx;
    llvm::Value *llvmUnwrapIdxAddr;
    llvm::BasicBlock *llvmUnwrapHead;
    llvm::BasicBlock *llvmUnwrapEnd;


    // General/Utility Functions
    bool ValidateToken(int tokenType);

    std::string TypeToString(int tokenType);

    void PrintDebugInfo(std::string langID);

    void ReportError(std::string msg);
    void ReportMissingTokenError(std::string expected);
    void ReportTypeMismatchError(std::string expected, std::string actual);
    void ReportOpTypeCheckError(std::string op, std::string type1, std::string type2);

    void ReportWarning(std::string msg);

    void Resync(int tokens[], int length);


    // Parsing
    void ProgramHeader();
    void ProgramBody();

    void Declaration();
    void WhileDeclarations();
    void WhileStatements(int terminators[], int terminatorsSize);
    void ProcedureDeclaration(Symbol &procedure);
    void ProcedureHeader(Symbol &procedure);
    void ParameterList(Symbol &procedure);
    void Parameter(Symbol &procedure);
    void ProcedureBody();
    void VariableDeclaration(Symbol &variable);
    void TypeMark(Symbol &symbol);
    void Bound(Symbol &symbol);

    void Statement();
    void AssignmentStatement();
    void IfStatement();
    void LoopStatement();
    void ReturnStatement();


    Symbol IndexArray(Symbol symbol);
    Symbol AssignmentTypeCheck(Symbol dest, Symbol expr, token_t *token);

    Symbol ProcedureCallOrName();
    Symbol Destination();

    Symbol Expression(Symbol expectedType);
    Symbol Expression_(Symbol expectedType);
    Symbol ExpressionTypeCheck(Symbol expectedType, Symbol arithOp, Symbol expr_, token_t *op, bool isNotOp);

    Symbol ArithOp(Symbol expectedType);
    Symbol ArithOp_(Symbol expectedType);
    Symbol ArithOpTypeCheck(Symbol expectedType, Symbol rel, Symbol arithOp_, token_t *op);

    Symbol Relation(Symbol expectedType);
    Symbol Relation_(Symbol expectedType);
    Symbol RelationTypeCheck(Symbol expectedType, Symbol term, Symbol relation_, token_t *op);

    Symbol Term(Symbol expectedType);
    Symbol Term_(Symbol expectedType);

    Symbol Factor(Symbol expectedType);
    Symbol Number();
    Symbol String();
    // Symbol Name();

    std::string Identifier();

    std::vector<llvm::Value *> ArgumentList(std::vector<Symbol>::iterator curr, std::vector<Symbol>::iterator end);

    // LLVM
    llvm::Type* GetLLVMType(Symbol symbol);
    llvm::Value* ConvertIntToBool(llvm::Value *intVal);
};

#endif //COMPILER_THEORY_PARSER_H
