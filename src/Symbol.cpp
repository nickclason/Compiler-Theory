//
// Created by Nick Clason on 3/8/21.
//

#include "../include/Symbol.h"

Symbol::Symbol() {
    id = "";

    arraySize = 0;
    declarationType = T_UNKNOWN;
    type = T_UNKNOWN;

    isArray = false;
    isGlobal = false;
    isInitialized = false;
    isValid = true;
    isIndexed = false;
    isUsed = false;

    llvmAddress = nullptr;
    llvmValue = nullptr;
    llvmArrayAddress = nullptr;
    llvmArraySize = nullptr;
    llvmFunction = nullptr;
}

Symbol::~Symbol() {
}

void Symbol::PrintDebugInfo() {
     std::cout << id << std::endl;
}

const std::string &Symbol::GetId() const {
    return id;
}

void Symbol::SetId(const std::string &id) {
    Symbol::id = id;
}

int Symbol::GetArraySize() const {
    return arraySize;
}

void Symbol::SetArraySize(int arraySize) {
    Symbol::arraySize = arraySize;
}

int Symbol::GetDeclarationType() const {
    return declarationType;
}

void Symbol::SetDeclarationType(int declarationType) {
    Symbol::declarationType = declarationType;
}

int Symbol::GetType() const {
    return type;
}

void Symbol::SetType(int type) {
    Symbol::type = type;
}

bool Symbol::IsArray() const {
    return isArray;
}

void Symbol::SetIsArray(bool isArray) {
    Symbol::isArray = isArray;
}

bool Symbol::IsGlobal() const {
    return isGlobal;
}

void Symbol::SetIsGlobal(bool isGlobal) {
    Symbol::isGlobal = isGlobal;
}

bool Symbol::IsInitialized() const {
    return isInitialized;
}

void Symbol::SetIsInitialized(bool isInitialized) {
    Symbol::isInitialized = isInitialized;
}

bool Symbol::IsValid() const {
    return isValid;
}

void Symbol::SetIsValid(bool isValid) {
    Symbol::isValid = isValid;
}

bool Symbol::IsArrayIndexed() const {
    return isIndexed;
}

void Symbol::SetIsArrayIndexed(bool isIndexed) {
    Symbol::isIndexed = isIndexed;
}

std::vector<Symbol> &Symbol::GetParameters() {
    return parameters;
}

void Symbol::SetParameters(const std::vector<Symbol> &parameters) {
    Symbol::parameters = parameters;
}

llvm::Value *Symbol::GetLLVMAddress() const {
    return llvmAddress;
}

void Symbol::SetLLVMAddress(llvm::Value *llvmAddress) {
    Symbol::llvmAddress = llvmAddress;
}

llvm::Value *Symbol::GetLLVMValue() const {
    return llvmValue;
}

void Symbol::SetLLVMValue(llvm::Value *llvmValue) {
    Symbol::llvmValue = llvmValue;
}

llvm::Value *Symbol::GetLLVMArrayAddress() const {
    return llvmArrayAddress;
}

void Symbol::SetLLVMArrayAddress(llvm::Value *llvmArrayAddress) {
    Symbol::llvmArrayAddress = llvmArrayAddress;
}

llvm::Value *Symbol::GetLLVMArraySize() const {
    return llvmArraySize;
}

void Symbol::SetLLVMArraySize(llvm::Value *llvmArraySize) {
    Symbol::llvmArraySize = llvmArraySize;
}

llvm::Function *Symbol::GetLLVMFunction() const {
    return llvmFunction;
}

void Symbol::SetLLVMFunction(llvm::Function *llvmFunction) {
    Symbol::llvmFunction = llvmFunction;
}

bool Symbol::IsUsed() const
{
    return isUsed;
}

void Symbol::SetIsUsed(bool used)
{
    isUsed = used;
}