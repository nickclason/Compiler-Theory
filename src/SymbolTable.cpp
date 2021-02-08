//
// Created by Nick Clason on 2/2/21.
//

#include "../include/definitions.h"
#include "../include/SymbolTable.h"

SymbolTable::SymbolTable()
{
    currScope = 0;
}

SymbolTable::~SymbolTable()
{

}

void SymbolTable::AddScope()
{
    if (symTableScopes.size() > 0)
    {
        currScope++;
    }

    std::map<std::string, Node> symbolTable;
    symTableScopes.push_back(symbolTable);
}

void SymbolTable::RemoveScope()
{
    symTableScopes.pop_back();
    currScope--;
}

//void SymbolTable::ChangeScope(int scope)
//{
//
//}

bool SymbolTable::AddSymbol(std::string id, int type, std::vector<Node> args, bool isGlobal)
{
    it = symTableScopes[0].find(id);
    if (it != symTableScopes[0].end() && symTableScopes[0][id].isGlobal)
    {
        // TODO: throw some kind of error??
        return false;
    }
    else
    {
        it = symTableScopes.back().find(id);
        if (it != symTableScopes.back().end())
        {
            // TODO: throw some kind of error??
            return false;
        }
        else
        {
            Node newNode;
            newNode.id = id;
            newNode.size = 0;
            newNode.scope = currScope;
            newNode.isGlobal = isGlobal;
            newNode.type = type;
            newNode.args = args;

            // TODO: remove
//            std::pair <std::map<std::string, Node>::iterator, bool> res;
//            res = symTableScopes.back().insert(std::pair<std::string, Node>(id, newNode));
            symTableScopes.back().insert(std::pair<std::string, Node>(id, newNode));

            if (type == T_PROCEDURE)
            {
                symTableScopes.end()[-2].insert(std::pair<std::string, Node>(id, newNode));
            }
        }
    }
}

bool SymbolTable::AddSymbolToParentScope(std::string id, int type, std::vector<Node> args, bool isGlobal)
{
    return false;
}

bool SymbolTable::DoesSymbolExist(std::string id, bool &isGlobal, Node &n)
{
    int type = T_UNKNOWN;
    it = symTableScopes[currScope].find(id);

    if (it != symTableScopes[currScope].end())
    {
        isGlobal = false; // TODO: I think?
        n = it->second;
        return true;
    }

    it = symTableScopes[0].find(id); // Global Scope
    if (it != symTableScopes[0].end() && symTableScopes[0][id].isGlobal) // In global scope and declared as global
    {
        isGlobal = true;
        n = it->second;
        return true;
    }

    // TODO: throw some kind of "undeclared identifier error"
    return false;
}

int SymbolTable::getScope()
{
    return currScope;
}

void SymbolTable::PrintScopes()
{
    return;
}
