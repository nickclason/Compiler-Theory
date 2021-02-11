//
// Created by Nick Clason on 2/2/21.
//

#ifndef COMPILER_THEORY_SYMBOLTABLE_H
#define COMPILER_THEORY_SYMBOLTABLE_H

#include <map>
#include <string>
#include <vector>

// TODO: Each procedure/"scope" needs its own symbol table
//       Determines if a variable/symbol has already been defined
//       Update variables, add new info
//       Type checking

struct Symbol
{
    int type;               // int, float, bool, enum, string
    int declarationType;    // variable declaration, procedure declaration, enum declaration
    int size;               // size of arrays, 0 non-arrays
    bool isGlobal;          // True if this symbol is in the global scope; false otherwise

    std::string id;
    std::vector<Symbol> parameters; // For procedures

    Symbol()
    {
        type = -1;
        declarationType = -1;
        size = -1;
        isGlobal = false;
        id = "";
        parameters = std::vector<Symbol>();
    }

    Symbol(int &type_, int &decType_, int &size_, bool isGlobal_, std::string &id_, std::vector<Symbol> parameters_)
    {
        type = type_;
        declarationType = decType_;
        size = size_;
        isGlobal = isGlobal_;
        id = id_;
        parameters = parameters_;
    }
};


class SymbolTable {

    public:

        SymbolTable();

        void AddScope();
        void RemoveScope();

        bool AddSymbol(Symbol symbol);
        bool AddSymbolToScope(Symbol symbol, int scope);
        bool DoesSymbolExist(std::string &id, Symbol &symbol);

        int GetScope();
        void SetScope(int &scope);

        // Debug Methods
        void PrintScopes();
        void PrintScope(int idx);

    private:
        int scope;
        std::vector<std::map<std::string, Symbol> > scopes;
        std::map<std::string, Symbol>::iterator it;



};

#endif //COMPILER_THEORY_SYMBOLTABLE_H
