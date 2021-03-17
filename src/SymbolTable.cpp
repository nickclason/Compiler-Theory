//
// Created by Nick Clason on 2/2/21.
//

#include "../include/definitions.h"
#include "../include/SymbolTable.h"


SymbolTable::SymbolTable()
{
    scopeCount = -1;
}

SymbolTable::~SymbolTable()
{

}

void SymbolTable::AddScope() {
    localScopes.push_back(std::map<std::string, Symbol>());
    scopeCount++;
}

void SymbolTable::RemoveScope() {
    if (localScopes.size() > 0)
    {
        localScopes.pop_back();
    }
    else
    {
        std::cout << "Error: No scope to remove" << std::endl;
    }

    scopeCount--;
}

void SymbolTable::AddSymbol(Symbol &symbol) {
    // Debug only
    std::cout << "\tInserting symbol: ";
    symbol.PrintDebugInfo();

    if (symbol.IsGlobal())
    {
        globalScope[symbol.GetId()] = symbol;
    }
    else
    {
        localScopes.back()[symbol.GetId()] = symbol;
    }
}

void SymbolTable::SetScopeProc(Symbol proc) {
    localScopes.back()["_proc"] = proc;
}

Symbol SymbolTable::GetScopeProc() {
    std::map<std::string, Symbol>::iterator it = localScopes.back().find("_proc");
    if (it != localScopes.back().end())
    {
        return it->second;
    }
    else
    {
        Symbol symbol;
        symbol.SetIsValid(false);
        return symbol;
    }
}

bool SymbolTable::DoesSymbolExist(std::string id) {
    std::map<std::string, Symbol>::iterator it = localScopes.back().find(id);
    if (it != localScopes.back().end())
    {
        return true;
    }
    else if (scopeCount != 0)
    {
        // This might solve the issue of variables that were prefixed with the global keyword not being able
        // to be redefined in a local scope
        return false;
    }

    it = globalScope.find(id);
    if (it != globalScope.end())
    {
        return true;
    }

    if (id.compare("main") == 0)
    {
        return true;
    }

    return false;
}

Symbol SymbolTable::FindSymbol(std::string id) {
    std::map<std::string, Symbol>::iterator it = localScopes.back().find(id);
    if (it != localScopes.back().end())
    {
        // Debug
        std::cout << "Found the following symbol:" << std::endl;
        it->second.PrintDebugInfo();

        return it->second;
    }

    it = globalScope.find(id);
    if (it != globalScope.end())
    {
        return it->second;
    }
    Symbol symbol;
    symbol.SetIsValid(false);;
    return symbol;
}

std::map<std::string, Symbol> SymbolTable::GetLocalScope() {
    return localScopes.back();
}

void SymbolTable::AddIOFunctions(llvm::Module *llvmModule, llvm::LLVMContext &llvmContext, llvm::IRBuilder<> *llvmBuilder)
{
    llvm::FunctionType *function_type = nullptr;
    llvm::Function *procedure = nullptr;

    
    Symbol get_bool;
    get_bool.SetId("GETBOOL");
    get_bool.SetType(T_BOOL);
    get_bool.SetIsGlobal(true);
    get_bool.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getInt1Ty(),
                {},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                get_bool.GetId(),
                llvmModule);
        get_bool.SetLLVMFunction(procedure);
    AddSymbol(get_bool);

    // getInteger() : integer value
    Symbol get_integer;
    get_integer.SetId("GETINTEGER");
    get_integer.SetType(T_INTEGER);
    get_integer.SetIsGlobal(true);
    get_integer.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getInt32Ty(),
                {},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                get_integer.GetId(),
                llvmModule);
        get_integer.SetLLVMFunction(procedure);

    AddSymbol(get_integer);

    // getFloat() : float value
    Symbol get_float;
    get_float.SetId("GETFLOAT");
    get_float.SetType(T_FLOAT);
    get_float.SetIsGlobal(true);
    get_float.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getFloatTy(),
                {},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                get_float.GetId(),
                llvmModule);
        get_float.SetLLVMFunction(procedure);

    AddSymbol(get_float);

    // getString() : string value
    Symbol get_string;
    get_string.SetId("GETSTRING");
    get_string.SetType(T_STRING);
    get_string.SetIsGlobal(true);
    get_string.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getInt8PtrTy(),
                {},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                get_string.GetId(),
                llvmModule);
        get_string.SetLLVMFunction(procedure);

    AddSymbol(get_string);

    // putBool(bool value) : bool
    Symbol put_bool;
    put_bool.SetId("PUTBOOL");
    put_bool.SetType(T_BOOL);
    Symbol put_bool_arg;
    put_bool_arg.SetId("value");
    put_bool_arg.SetType(T_BOOL);
    put_bool_arg.SetDeclarationType(T_VARIABLE);
    put_bool.GetParameters().push_back(put_bool_arg);
    put_bool.SetIsGlobal(true);
    put_bool.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getInt1Ty(),
                {llvmBuilder->getInt1Ty()},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                put_bool.GetId(),
                llvmModule);
        put_bool.SetLLVMFunction(procedure);

    AddSymbol(put_bool);

    // putInteger(integer value) : bool
    Symbol put_integer;
    put_integer.SetId("PUTINTEGER");
    put_integer.SetType(T_BOOL);
    Symbol put_integer_arg;
    put_integer_arg.SetId("value");
    put_integer_arg.SetType(T_INTEGER);
    put_integer_arg.SetDeclarationType(T_VARIABLE);
    put_integer.GetParameters().push_back(put_integer_arg);
    put_integer.SetIsGlobal(true);
    put_integer.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getInt1Ty(),
                {llvmBuilder->getInt32Ty()},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                put_integer.GetId(),
                llvmModule);
        put_integer.SetLLVMFunction(procedure);

    AddSymbol(put_integer);

    // putFloat(float value) : bool
    Symbol put_float;
    put_float.SetId("PUTFLOAT");
    put_float.SetType(T_BOOL);
    Symbol put_float_arg;
    put_float_arg.SetId("value");
    put_float_arg.SetType(T_FLOAT);
    put_float_arg.SetDeclarationType(T_VARIABLE);
    put_float.GetParameters().push_back(put_float_arg);
    put_float.SetIsGlobal(true);
    put_float.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getInt1Ty(),
                {llvmBuilder->getFloatTy()},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                put_float.GetId(),
                llvmModule);
        put_float.SetLLVMFunction(procedure);

    AddSymbol(put_float);

    // putString(string value) : bool
    Symbol put_string;
    put_string.SetId("PUTSTRING");
    put_string.SetType(T_BOOL);
    Symbol put_string_arg;
    put_string_arg.SetId("value");
    put_string_arg.SetType(T_STRING);
    put_string_arg.SetDeclarationType(T_VARIABLE);
    put_string.GetParameters().push_back(put_string_arg);
    put_string.SetIsGlobal(true);
    put_string.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getInt1Ty(),
                {llvmBuilder->getInt8PtrTy()},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                put_string.GetId(),
                llvmModule);
        put_string.SetLLVMFunction(procedure);

    AddSymbol(put_string);

    // sqrt(integer value) : float
    Symbol sqrt;
    sqrt.SetId("SQRT");
    sqrt.SetType(T_FLOAT);
    Symbol sqrt_arg;
    sqrt_arg.SetId("value");
    sqrt_arg.SetType(T_INTEGER);
    sqrt_arg.SetDeclarationType(T_VARIABLE);
    sqrt.GetParameters().push_back(sqrt_arg);
    sqrt.SetIsGlobal(true);
    sqrt.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getFloatTy(),
                {llvmBuilder->getInt32Ty()},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                "mysqrt",
                llvmModule);
        sqrt.SetLLVMFunction(procedure);

    AddSymbol(sqrt);

    // create a runtime function for out of bounds errors
    Symbol error_func;
    error_func.SetId("_error_func"); // hide it with leading '_'
    error_func.SetType(T_BOOL);
    error_func.SetIsGlobal(true);
    error_func.SetDeclarationType(T_PROCEDURE);

        function_type = llvm::FunctionType::get(
                llvmBuilder->getVoidTy(),
                {},
                false);
        procedure = llvm::Function::Create(
                function_type,
                llvm::Function::ExternalLinkage,
                "boundsError",
                llvmModule);
        error_func.SetLLVMFunction(procedure);

    AddSymbol(error_func);
}

int SymbolTable::GetScopeCount()
{
    return scopeCount;
}
