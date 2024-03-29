//
// Created by Nick Clason on 2/11/21.
//

#ifndef COMPILER_THEORY_SYMBOL_H
#define COMPILER_THEORY_SYMBOL_H

#include "definitions.h"

#include <string>
#include <vector>

#include <llvm/IR/Value.h>

class Symbol
{
public:

    Symbol();
    ~Symbol();

    const std::string &GetId() const;
    void SetId(const std::string &id);

    int GetArraySize() const;
    void SetArraySize(int arraySize);

    int GetDeclarationType() const;
    void SetDeclarationType(int declarationType);

    int GetType() const;
    void SetType(int type);

    bool IsArray() const;
    void SetIsArray(bool isArray);

    bool IsGlobal() const;
    void SetIsGlobal(bool isGlobal);

    bool IsInitialized() const;
    void SetIsInitialized(bool isInitialized);

    bool IsValid() const;
    void SetIsValid(bool isValid);

    bool IsArrayIndexed() const;
    void SetIsArrayIndexed(bool isIndexed);

    std::vector<Symbol> &GetParameters();
    void SetParameters(const std::vector<Symbol> &parameters);

    llvm::Value *GetAddress() const;
    void SetAddress(llvm::Value *address);

    llvm::Value *GetValue() const;
    void SetValue(llvm::Value *value);

    llvm::Value *GetArrayAddress() const;
    void SetArrayAddress(llvm::Value *arrayAddress);

    llvm::Value *GetLLVMArraySize() const;
    void SetLLVMArraySize(llvm::Value *llvmArraySize);

    llvm::Function *GetFunction() const;
    void SetFunction(llvm::Function *function) ;

    void CopySymbol(Symbol toCopy);

private:

    std::string id;

    int arraySize;
    int declarationType;
    int type;

    bool isArray;
    bool isGlobal;
    bool isInitialized;
    bool isValid;
    bool isIndexed;

    std::vector<Symbol> parameters;

    // LLVM
    llvm::Value *address;
    llvm::Value *value;
    llvm::Value *arrayAddress;
    llvm::Value *llvmArraySize;
    llvm::Function *function;
};

#endif //COMPILER_THEORY_SYMBOL_H
