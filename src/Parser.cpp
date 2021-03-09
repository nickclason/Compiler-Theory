//
// Created by Nick Clason on 2/1/21.
//

#include "../include/Parser.h"
#include "../include/Scanner.h"


Parser::Parser(std::string fileName, bool debug_, Scanner scanner_, SymbolTable symbolTable_, token_t *token_)
{
    debug = debug_;
    scanner = scanner_;
    symbolTable = symbolTable_;
    token = token_;

    stopParse = false;
    errorFlag = false;

    procedureCount = 0;

    llvmModule = nullptr;
    llvmBuilder = nullptr;
    llvmCurrProc = nullptr;

    // TODO: start parsing
    Program();
    if (debug)
    {
        llvmModule->print(llvm::errs(), nullptr);
    }
}

Parser::~Parser()
{
    if (llvmBuilder != nullptr)
    {
        delete llvmBuilder;
    }

    if (llvmCurrProc != nullptr)
    {
        delete llvmCurrProc;
    }

    if (llvmModule != nullptr)
    {
        delete llvmModule;
    }
}

void Parser::Program()
{
    symbolTable.AddScope();
    ProgramHeader();
    ProgramBody();
}

void Parser::ProgramHeader()
{
    PrintDebugInfo("<program_header>");

    if (!ValidateToken(T_PROGRAM))
    {
        YieldMissingTokenError("PROGRAM", *token);
        return;
    }

    std::string id = Identifier();

    llvmModule = new llvm::Module(id, llvmContext);
    llvmBuilder = new llvm::IRBuilder<>(llvmContext);

    // TODO: insert runtime functions

    if (!ValidateToken(T_IS))
    {
        YieldMissingTokenError("IS", *token);
        return;
    }

    return;
}

void Parser::ProgramBody()
{
    PrintDebugInfo("<program_body>");

    // Get all declarations
    WhileDeclarations();

    if (!ValidateToken(T_BEGIN))
    {
        YieldMissingTokenError("BEGIN", *token);
        return;
    }

    std::vector<llvm::Type *> parameters;
    llvm::FunctionType *functionType = llvm::FunctionType::get(llvmBuilder->getInt32Ty(), parameters, false);

    llvm::FunctionCallee mainCallee = llvmModule->getOrInsertFunction("main", functionType);
    llvm::Constant *main = llvm::dyn_cast<llvm::Constant>(mainCallee.getCallee());

    llvmCurrProc = llvm::cast<llvm::Function>(main);
    llvmCurrProc->setCallingConv(llvm::CallingConv::C);

    llvm::BasicBlock *entrypoint = llvm::BasicBlock::Create(llvmContext, "entrypoint", llvmCurrProc);
    llvmBuilder->SetInsertPoint(entrypoint);

    for (auto &it : symbolTable.GetLocalScope())
    {
        // removed per revision 5 of projectLanguage.pdf
        // if (it.second.GetDeclarationType() != T_VARIABLE || it.second.IsGlobal())
        if (it.second.GetDeclarationType() != T_VARIABLE)
        {
            continue;
        }

        llvm::Value *address = nullptr;
        // array
        address = llvmBuilder->CreateAlloca(GetLLVMType(it.second));
        it.second.SetLLVMAddress(address);

        symbolTable.AddSymbol(it.second); // Update symbol table
    }

    int terminators[] = { T_END };
    int terminatorsSize = 1;
    WhileStatements(terminators, terminatorsSize);


}

bool Parser::ValidateToken(int tokenType)
{

    // Ignore comments
    token_t* tempToken = scanner.PeekToken();
    while (tempToken->type == T_COMMENT)
    {
        token = scanner.GetToken();
        tempToken = scanner.PeekToken();
    }

    if (tempToken->type == tokenType)
    {
        token = scanner.GetToken();
        return true;
    }
    else if (tempToken->type == T_UNKNOWN)
    {
        std::string errMsg = "Unknown token was found: " + token->str;
        YieldError(errMsg, *token);
        return false;
    }
    else if (tempToken->type == T_EOF)
    {
        return false;
    }

    return false;
}

void Parser::PrintDebugInfo(std::string langID)
{
    if (debug)
    {
        std::cout << langID << std::endl;
    }
}

void Parser::YieldError(token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << std::endl << std::endl;
    errorFlag = true;
}

void Parser::YieldError(std::string msg, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << " : " << msg << std::endl;
    errorFlag = true;
}

void Parser::YieldMissingTokenError(std::string expected, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << " : " << "Expected token " << expected << std::endl;
    errorFlag = true;
}

void Parser::YieldTypeMismatchError(std::string expected, std::string actual, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line: " << token.line << " Col: " << token.col << " : " << std::endl;
    std::cout << "Expected Type: " << expected << std::endl;
    std::cout << "Found Type: " << actual << std::endl;
    errorFlag = true;
}

void Parser::YieldOpTypeCheckError(std::string op, std::string type1, std::string type2, token_t token)
{
    if (errorFlag) return;

    std::cout << "Line:" << token.line << " Col:" << token.col << " - ";
    std::cout << "Incompatible types for " << op << ":" << std::endl;
    std::cout << "Type 1: " << type1 << std::endl;
    std::cout << "Type 2: " << type2 << std::endl << std::endl;
    errorFlag = true;
}

void Parser::YieldWarning(std::string msg, token_t token)
{
    if (errorFlag) return;
    std::cout << "Line:" << token.line << " Col:" << token.col << " : ";
    std::cout << "Warning:" << std::endl << msg << std::endl << std::endl;
}

std::string Parser::Identifier()
{
    PrintDebugInfo("<identifier>");

    if (!ValidateToken(T_IDENTIFIER))
    {
        YieldError("Identifier expected", *token);
        return std::string();
    }

    return token->val.stringValue;
}

void Parser::WhileDeclarations()
{
    bool continue_ = true;
    while (continue_)
    {
        Declaration();

        if (errorFlag)
        {
            // TODO: resync
            return;
        }
        else
        {
            if (!ValidateToken(T_SEMICOLON))
            {
                YieldMissingTokenError(";", *token);
                return;
            }
        }

        token = scanner.PeekToken();
        if (token->type == T_BEGIN)
        {
            continue_ = false;
        }
    }
}

void Parser::Declaration()
{
    PrintDebugInfo("<declaration>");

    Symbol symbol;

    token = scanner.PeekToken();

    // TODO: might need to rework this to make all variables defined in the outer scope global
    //       *** could also just treat localScopes[0] as the global scope ***
    if (token->type == T_GLOBAL)
    {
        ValidateToken(T_GLOBAL);
        symbol.SetIsGlobal(true);
        token = scanner.PeekToken();
    }

    if (token->type == T_PROCEDURE)
    {
        //ProcedureDeclaration(symbol);
    }
    else if (token->type == T_VARIABLE)
    {
        VariableDeclaration(symbol);
    }
    else
    {
        YieldError("Error parsing declaration, expected a procedure or variable", *token);
        return;
    }
}

void Parser::VariableDeclaration(Symbol &variable)
{
    PrintDebugInfo("<variable_declaration>");

    variable.SetDeclarationType(T_VARIABLE);

    if (!ValidateToken(T_VARIABLE))
    {
        YieldMissingTokenError("VARIABLE", *token);
        return;
    }

    std::string id = Identifier();
    variable.SetId(id);

    if (!ValidateToken(T_COLON))
    {
        YieldMissingTokenError(":", *token);
    }

    TypeMark(variable);

    // TODO: array

    if (variable.IsGlobal())
    {
        llvm::Type *globalTy = nullptr;

        // array

        globalTy = GetLLVMType(variable);
        llvm::Constant *constInit = llvm::Constant::getNullValue(globalTy);
        llvm::Value *val = new llvm::GlobalVariable(*llvmModule, globalTy, false, llvm::GlobalValue::CommonLinkage, constInit, variable.GetId());
        variable.SetIsInitialized(true);

        // array
        variable.SetLLVMAddress(val);
    }

    if (symbolTable.DoesSymbolExist(variable.GetId()))
    {
        YieldError("Identifier already exists", *token);
        return;
    }

    symbolTable.AddSymbol(variable);
}

void Parser::TypeMark(Symbol &symbol)
{
    PrintDebugInfo("<type_mark>");

    if (ValidateToken(T_INTEGER))
    {
        symbol.SetType(T_INTEGER);
        return;
    }
    else if (ValidateToken(T_FLOAT))
    {
        symbol.SetType(T_FLOAT);
        return;
    }
    else if (ValidateToken(T_BOOL))
    {
        symbol.SetType(T_BOOL);
        return;
    }
    else if (ValidateToken(T_STRING))
    {
        symbol.SetType(T_STRING);
        return;
    }
    else
    {
        YieldError("Expected int, float, bool, or string type", *token);
        return;
    }

}

void Parser::WhileStatements(int terminators[], int terminatorsSize)
{
    bool continue_ = true;

    token = scanner.PeekToken();
    for (int i = 0; i < terminatorsSize; i++)
    {
        if (token->type == terminators[i])
        {
            continue_ = false;
            break;
        }
    }

    // Parse statements
    while (continue_)
    {
        Statement();

        if (errorFlag)
        {
            // TODO: resync
            return;
        }
        else
        {
            if (!ValidateToken(T_SEMICOLON))
            {
                YieldMissingTokenError(";", *token);
                return;
            }
        }

        token = scanner.PeekToken();
        for (int i = 0; i < terminatorsSize; i++)
        {
            if (token->type == terminators[i])
            {
                continue_ = false;
                break;
            }
        }
    }



}

void Parser::Statement() {
    PrintDebugInfo("<statement>");

    token = scanner.PeekToken();
    if (token->type == T_IDENTIFIER)
    {
        AssignmentStatement();
    }
    else if (token->type == T_IF)
    {
        //IfStatement();
    }
    else if (token->type == T_FOR)
    {
        //LoopStatement();
    }
    else if (token->type == T_RETURN)
    {
        //ReturnStatement();
    }
    else
    {
        YieldError("Expected if, for, or return statement", *token);
        return;
    }

}

void Parser::AssignmentStatement()
{
    PrintDebugInfo("<assignment_statement>");

    Symbol dest = Destination();
    if (!ValidateToken(T_ASSIGNMENT))
    {
        YieldMissingTokenError(":=", *token);
    }

    Symbol expr = Expression(dest);

    // TODO: type checking/type conversions

    if (!expr.IsValid())
    {
        return;
    }

    llvmBuilder->CreateStore(expr.GetLLVMValue(), dest.GetLLVMAddress());
    dest.SetLLVMValue(expr.GetLLVMValue());
    symbolTable.AddSymbol(dest); // Update symbol

    dest.SetIsInitialized(true);
    symbolTable.AddSymbol(dest); // Update TODO: remove this or the one above
}

Symbol Parser::Destination()
{
    PrintDebugInfo("<destination>");

    std::string id = Identifier();
    Symbol dest = symbolTable.FindSymbol(id);

    if (!dest.IsValid())
    {
        YieldError("Symbol: " + id + " not found", *token);
        return dest;
    }

    if (dest.GetDeclarationType() != T_VARIABLE)
    {
        YieldError("Variable required for valid destination", *token);
        dest = Symbol(); // TODO: might need to generate a unique random identifier for this symbol
        dest.SetIsValid(false);
        return dest;
    }

    if (ValidateToken(T_LBRACKET))
    {
        // TODO: array
        // dest = Index();
    }
    else
    {
        // TODO: array unwrapping
    }

    // If we get here, the destination is valid and not an array
    return dest;
}

Symbol Parser::Expression(Symbol expectedType)
{
    PrintDebugInfo("<expression>");

    bool isNotOp = ValidateToken(T_NOT) ? true : false;

    Symbol arithOp = ArithOp(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol exprTail = ExpressionTail(expectedType);

    return ExpressionTypeCheck(expectedType, arithOp, exprTail, op, isNotOp);
}

Symbol Parser::ExpressionTail(Symbol expectedType)
{
    PrintDebugInfo("<expression_tail>");

    if (ValidateToken(T_AND) || ValidateToken(T_AND))
    {
        Symbol arithOp = ArithOp(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol exprTail = ExpressionTail(expectedType);

        return ExpressionTypeCheck(expectedType, arithOp, exprTail, op, false);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::ExpressionTypeCheck(Symbol expectedType, Symbol arithOp, Symbol exprTail, token_t *op, bool isNotOp)
{
    if (exprTail.IsValid())
    {
        Symbol sym = Symbol();
        bool isInterop = false;
        std::string opStr;

        if (expectedType.GetType() == T_BOOL)
        {
            opStr = "logical";
            isInterop = (arithOp.GetType() == T_BOOL && exprTail.GetType() == T_BOOL);
            sym.SetType(T_BOOL);
        }
        else if (expectedType.GetType() == T_INTEGER || expectedType.GetType() == T_FLOAT)
        {
            isInterop = (arithOp.GetType() == T_INTEGER && exprTail.GetType() == T_INTEGER);
            opStr = "binary";
            sym.SetType(T_INTEGER);
        }
        else
        {
            YieldError("Invalid type", *op);
            sym.SetIsValid(false);

            return sym;
        }

        if (!isInterop)
        {
            // TODO: convert types to human readable strings
            YieldOpTypeCheckError(opStr, std::to_string(arithOp.GetType()), std::to_string(exprTail.GetType()), *op);
            sym.SetIsValid(false);

            return sym;
        }

        llvm::Value *val;
        switch (op->type)
        {
            case T_AND:
                val = llvmBuilder->CreateAnd(arithOp.GetLLVMValue(), exprTail.GetLLVMValue());
                break;
            case T_OR:
                val = llvmBuilder->CreateOr(arithOp.GetLLVMValue(), exprTail.GetLLVMValue());
                break;
            default:
                std::cout << "error in codegen for operation" << std::endl;
        }

        sym.SetLLVMValue(val);

        if (isNotOp)
        {
            val = llvmBuilder->CreateNot(sym.GetLLVMValue());
            sym.SetLLVMValue(val);
        }

        return sym;
    }
    else
    {
        if (isNotOp)
        {
            bool isInterop = false;
            if (expectedType.GetType() == T_BOOL)
            {
                isInterop = (arithOp.GetType() == T_BOOL);
            }
            else if (expectedType.GetType() == T_INTEGER || expectedType.GetType() == T_FLOAT)
            {
                isInterop = (arithOp.GetType() == T_INTEGER);
            }

            if (!isInterop)
            {
                // TODO: convert type to human readable strings
                YieldOpTypeCheckError("binary", std::to_string(arithOp.GetType()), "null", *op);

                Symbol sym = Symbol();
                sym.SetIsValid(false);

                return sym;
            }

            llvm::Value *val = llvmBuilder->CreateNot(arithOp.GetLLVMValue());
            arithOp.SetLLVMValue(val);
        }

        return arithOp;
    }
}

Symbol Parser::ArithOp(Symbol expectedType)
{
    PrintDebugInfo("<arithOp>");

    Symbol rel = Relation(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol arithOpTail = ArithOpTail(expectedType);

    return ArithOpTypeCheck(expectedType, rel, arithOpTail, op);
}

Symbol Parser::ArithOpTail(Symbol expectedType)
{
    PrintDebugInfo("<arithOp_tail>");

    if (ValidateToken(T_ADD) || ValidateToken(T_SUBTRACT))
    {
        Symbol rel = Relation(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol arithOpTail = ArithOpTail(expectedType);

        return ArithOpTypeCheck(expectedType, rel, arithOpTail, op);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::ArithOpTypeCheck(Symbol expectedType, Symbol rel, Symbol tail, token_t *op)
{
    if (tail.IsValid())
    {
        Symbol sym = Symbol();
        bool isInterop = false;
        if (rel.GetType() == T_INTEGER)
        {
            isInterop = (tail.GetType() == T_INTEGER || tail.GetType() == T_FLOAT);
            if (isInterop && tail.GetType() == T_FLOAT)
            {
                // do type conversion
                llvm::Value *val = llvmBuilder->CreateSIToFP(rel.GetLLVMValue(), llvmBuilder->getFloatTy());
                rel.SetLLVMValue(val);

            }
        }
        else if (rel.GetType() == T_FLOAT)
        {
            isInterop = (tail.GetType() == T_INTEGER || tail.GetType() == T_FLOAT);
            if (isInterop && tail.GetType() == T_INTEGER)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(tail.GetLLVMValue(), llvmBuilder->getFloatTy());
                tail.SetLLVMValue(val);
            }
        }

        if (!isInterop)
        {
            // TODO: convert types to readable format
            YieldOpTypeCheckError("arith", std::to_string(rel.GetType()), std::to_string(tail.GetType()), *op);
            sym.SetIsValid(false);

            return sym;
        }

        bool isFloatOp = (rel.GetType() == T_FLOAT || tail.GetType() == T_FLOAT);

        llvm::Value *val;
        switch (op->type)
        {
            case T_ADD:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FAdd, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Add, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                break;
            case T_SUBTRACT:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FSub, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Sub, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                break;
            case T_MULTIPLY:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FMul, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::Mul, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                break;
            case T_DIVIDE:
                if (isFloatOp)
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::FDiv, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                else
                {
                    val = llvmBuilder->CreateBinOp(llvm::Instruction::SDiv, rel.GetLLVMValue(), tail.GetLLVMValue());
                }
                break;
            default:
                std::cout << "Error finding operation" << std::endl;
        }

        sym.SetLLVMValue(val);

        if (expectedType.GetType() == T_FLOAT)
        {
            sym.SetType(T_FLOAT);
            if (!isFloatOp)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(sym.GetLLVMValue(), llvmBuilder->getFloatTy());
                sym.SetLLVMValue(val);
            }
        }
        else if (expectedType.GetType() == T_INTEGER)
        {
            sym.SetType(T_INTEGER);
            if (isFloatOp)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(sym.GetLLVMValue(), llvmBuilder->getInt32Ty());
                sym.SetLLVMValue(val);
            }
        }
        else
        {
            if (rel.GetType() == T_FLOAT || tail.GetType() == T_FLOAT)
            {
                sym.SetType(T_FLOAT);
            }
            else
            {
                sym.SetType(T_INTEGER);
            }
        }

        return sym;
    }
    else
    {
        return rel;
    }
}

Symbol Parser::Relation(Symbol expectedType)
{
    PrintDebugInfo("<relation>");

    Symbol term = Term(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol relTail = RelationTail(expectedType);

    return RelationTypeCheck(expectedType, term, relTail, op);
}

Symbol Parser::RelationTail(Symbol expectedType)
{
    PrintDebugInfo("<relation_tail>");

    if (ValidateToken(T_LESSTHAN) ||
        ValidateToken(T_GREATERTHAN) ||
        ValidateToken(T_GTEQ) ||
        ValidateToken(T_EQEQ) ||
        ValidateToken(T_NOTEQ))
    {
        Symbol term = Term(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol relTail = RelationTail(expectedType);

        return RelationTypeCheck(expectedType, term, relTail, op);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::RelationTypeCheck(Symbol expectedType, Symbol term, Symbol relTail, token_t *op)
{
    if (relTail.IsValid())
    {
        Symbol sym = Symbol();
        bool isInterop = false;
        bool isFloatOp = false;

        if (term.GetType() == T_BOOL)
        {
            isInterop = (relTail.GetType() == T_BOOL || relTail.GetType() == T_INTEGER);
            if (isInterop && relTail.GetType() == T_INTEGER)
            {
                llvm::Value *val = llvmBuilder->CreateZExtOrTrunc(term.GetLLVMValue(), llvmBuilder->getInt32Ty());
                term.SetLLVMValue(val);
            }
        }
        else if (term.GetType() == T_FLOAT)
        {
            isFloatOp = true;
            isInterop = (relTail.GetType() == T_FLOAT || relTail.GetType() == T_INTEGER);
            if (isInterop && relTail.GetType() == T_INTEGER)
            {
                llvm::Value *val = llvmBuilder->CreateSIToFP(relTail.GetLLVMValue(), llvmBuilder->getFloatTy());
                relTail.SetLLVMValue(val);
            }
        }
        else if (term.GetType() == T_INTEGER)
        {
            isInterop = (relTail.GetType() == T_INTEGER || relTail.GetType() == T_FLOAT || relTail.GetType() == T_BOOL);
            if (isInterop)
            {
                if (relTail.GetType() == T_BOOL)
                {
                    llvm::Value *val = llvmBuilder->CreateZExtOrTrunc(relTail.GetLLVMValue(), llvmBuilder->getInt32Ty());
                    relTail.SetLLVMValue(val);
                }
                else if (relTail.GetType() == T_FLOAT)
                {
                    llvm::Value *val = llvmBuilder->CreateSIToFP(term.GetLLVMValue(), llvmBuilder->getFloatTy());
                    term.SetLLVMValue(val);
                    isFloatOp = true;
                }
            }
        }
        else if (term.GetType() == T_STRING)
        {

            isInterop = ((op->type == T_EQEQ || op->type == T_NOTEQ) && (relTail.GetType() == T_STRING));
        }

        if (!isInterop)
        {
            // make types readable
            YieldOpTypeCheckError("relational", std::to_string(term.GetType()), std::to_string(relTail.GetType()), *op);
            sym.SetIsValid(false);
            return sym;
        }

        llvm::Value *val ;
        if (term.GetType() == T_STRING)
        {
            llvm::Value *idxAddr = llvmBuilder->CreateAlloca(llvmBuilder->getInt32Ty());
            llvm::Value *idx = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 0, true));
            llvmBuilder->CreateStore(idx, idxAddr);

            llvm::BasicBlock *strCmpBlock = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);
            llvm::BasicBlock *strCmpBlockEnd = llvm::BasicBlock::Create(llvmContext, "", llvmCurrProc);

            llvmBuilder->CreateBr(strCmpBlock);
            llvmBuilder->SetInsertPoint(strCmpBlock);

            llvm::Value *inc = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt32Ty(), llvm::APInt(32, 1, true));
            idx = llvmBuilder->CreateLoad(llvmBuilder->getInt32Ty(), idxAddr);
            idx = llvmBuilder->CreateBinOp(llvm::Instruction::Add, idx, inc);

            llvmBuilder->CreateStore(idx, idxAddr);

            llvm::Value *termAddr = llvmBuilder->CreateGEP(term.GetLLVMValue(), idx);
            llvm::Value *termChar = llvmBuilder->CreateLoad(llvmBuilder->getInt8Ty(), termAddr);
            llvm::Value *tailAddr = llvmBuilder->CreateGEP(relTail.GetLLVMValue(), idx);
            llvm::Value *tailChar = llvmBuilder->CreateLoad(llvmBuilder->getInt8Ty(), tailAddr);
            llvm::Value *cmp = llvmBuilder->CreateICmpEQ(termChar, tailChar);

            llvm::Value *zero = llvm::ConstantInt::getIntegerValue(llvmBuilder->getInt8Ty(), llvm::APInt(8, 0, true));
            llvm::Value *noEnd = llvmBuilder->CreateICmpNE(termChar, zero);
            llvm::Value *cont = llvmBuilder->CreateAnd(cmp, noEnd);

            llvmBuilder->CreateCondBr(cont, strCmpBlock, strCmpBlockEnd);
            llvmBuilder->SetInsertPoint(strCmpBlockEnd);

            if (op->type == T_EQEQ)
            {
                val = cmp;
            }
            else
            {
                val = llvmBuilder->CreateNot(cmp);
            }
        }
        else
        {
            switch (op->type)
            {
                case T_EQEQ:
                    if (isFloatOp) {

                        val = llvmBuilder->CreateFCmpOEQ(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpEQ(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    break;
                case T_NOTEQ:
                    if (isFloatOp) {

                        val = llvmBuilder->CreateFCmpONE(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpNE(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    break;
                case T_LESSTHAN:
                    if (isFloatOp) {

                        val = llvmBuilder->CreateFCmpOLT(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSLT(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    break;
                case T_LTEQ:
                    if (isFloatOp) {

                        val = llvmBuilder->CreateFCmpOLE(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSLE(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    break;
                case T_GREATERTHAN:
                    if (isFloatOp) {

                        val = llvmBuilder->CreateFCmpOGT(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSGT(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    break;
                case T_GTEQ:
                    if (isFloatOp) {

                        val = llvmBuilder->CreateFCmpOGE(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    else
                    {
                        val = llvmBuilder->CreateICmpSGE(term.GetLLVMValue(), relTail.GetLLVMValue());
                    }
                    break;
                default:
                    std::cout << "Operation error in codegen" << std::endl;
            }
        }

        sym.SetLLVMValue(val);
        sym.SetType(T_BOOL);

        return sym;
    }
    else
    {
        return term;
    }
}

Symbol Parser::Term(Symbol expectedType)
{
    PrintDebugInfo("<term>");

    Symbol factor = Factor(expectedType);
    token_t *op = scanner.PeekToken();
    Symbol termTail = TermTail(expectedType);

    return ArithOpTypeCheck(expectedType, factor, termTail, op);
}

Symbol Parser::TermTail(Symbol expectedType)
{
    PrintDebugInfo("<term_tail>");

    if (ValidateToken(T_DIVIDE) || ValidateToken(T_MULTIPLY))
    {
        Symbol factor = Factor(expectedType);
        token_t *op = scanner.PeekToken();
        Symbol termTail = TermTail(expectedType);

        return ArithOpTypeCheck(expectedType, factor, termTail, op);
    }

    Symbol sym = Symbol();
    sym.SetIsValid(false);

    return sym;
}

Symbol Parser::Factor(Symbol expectedType)
{
    PrintDebugInfo("<factor>");

    Symbol sym = Symbol();
    if (ValidateToken(T_LPAREN))
    {
        sym = Expression(expectedType);
        if (!ValidateToken(T_RPAREN))
        {
            YieldMissingTokenError(")", *token);
            sym.SetIsValid(false);
            return sym;
        }
    }
    else if (ValidateToken(T_IDENTIFIER))
    {
        // Name or procedure call
    }
    else if (ValidateToken(T_SUBTRACT))
    {

    }
    else if (ValidateToken(T_INT_LITERAL) || ValidateToken(T_FLOAT_LITERAL))
    {
        sym = Number();
    }
    else if (ValidateToken(T_STRING_LITERAL))
    {

    }
    else if (ValidateToken(T_TRUE))
    {

    }
    else if (ValidateToken(T_FALSE))
    {

    }
    else
    {
        YieldError("Factor expected", *token);
        sym.SetIsValid(false);
        return sym;
    }

    return sym;
}

Symbol Parser::Number()
{
    PrintDebugInfo("<number>");

    Symbol sym = Symbol();
    llvm::Value *val;
    if (token->type == T_INT_LITERAL)
    {
        sym.SetType(T_INTEGER);
        val = llvm::ConstantInt::getIntegerValue(GetLLVMType(sym), llvm::APInt(32, token->val.intValue, true));
    }
    else if (token->type == T_FLOAT_LITERAL)
    {
        sym.SetType(T_FLOAT);
        val = llvm::ConstantFP::get(GetLLVMType(sym), llvm::APFloat(token->val.floatValue));
    }

    sym.SetLLVMValue(val);

    return sym;
}

llvm::Type *Parser::GetLLVMType(Symbol symbol) {
    int type = symbol.GetType();

    switch (type)
    {
        case T_BOOL:
            return llvmBuilder->getInt1Ty();
        case T_INTEGER:
            return llvmBuilder->getInt32Ty();
        case T_FLOAT:
            return llvmBuilder->getFloatTy();
        case T_STRING:
            return llvmBuilder->getInt8PtrTy();
        default:
            std::cout << "Unknown Type" << std::endl;
            return llvm::IntegerType::getInt1Ty(llvmModule->getContext());
    }
}