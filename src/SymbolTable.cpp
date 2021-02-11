//
// Created by Nick Clason on 2/2/21.
//

#include "../include/definitions.h"
#include "../include/SymbolTable.h"

SymbolTable::SymbolTable()
{
    curr = nullptr;
    prev = nullptr;
    top = nullptr;
}

SymbolTable::~SymbolTable()
{
}

void SymbolTable::AddScope()
{
    if (curr != nullptr)
    {
        Scope *temp = curr;
        curr = new Scope();
        curr->prevScope = temp;
    }
    else
    {
        // New Global Scope
        curr = new Scope();
        curr->prevScope = nullptr;
        top = curr;
    }
}

void SymbolTable::ExitScope()
{
    if (curr != nullptr)
    {
        curr->PrintScope();
        Scope* temp = curr;
        curr = curr->prevScope;
        delete temp;
    }
}

bool SymbolTable::AddSymbol(std::string id, Symbol symbol, bool isGlobal)
{
    if (curr != nullptr)
    {
        if (!curr->DoesSymbolExist(id, false))
        {
            curr->AddSymbol(symbol);
            return true;
        }

        return false;
    }

    return false;
}

bool SymbolTable::AddSymbolToPrevScope(std::string id, Symbol symbol, bool isGlobal)
{
    Scope* previousScope = curr->prevScope;
    if (previousScope != nullptr) // Only if a higher scope exists
    {
        if (!previousScope->DoesSymbolExist(id, false))
        {
            previousScope->AddSymbol(symbol);
            return true;
        }

        return false;
    }
    return false;
}

bool SymbolTable::DoesSymbolExist(std::string id, Symbol &symbol, bool &isGlobal)
{
    if (curr == nullptr)
    {
        // No scope to search
        return false;
    }

    bool isFound = curr->DoesSymbolExist(id, false);
    if (isFound)
    {
        isGlobal = false;
        symbol = curr->GetSymbol(id);
        return true;
    }
    else
    {
        isFound = top->DoesSymbolExist(id, true);
        if (isFound)
        {
            isGlobal = true;
            symbol = top->GetSymbol(id);
            return true;
        }

        return false;
    }

    return false;
}

void SymbolTable::ChangeScopeName(std::string id)
{
    curr->SetName(id);
}


