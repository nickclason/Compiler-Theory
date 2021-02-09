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

struct Node
{
    int type;
    int size; // size of arrays, 0 for anything that's not an array?
    int scope; // not sure about this

    bool isGlobal;

    std::string id;
    std::vector<Node> args;
};


class SymbolTable {

    public:

        SymbolTable();
        ~SymbolTable();

        void AddScope();
        void RemoveScope();

        bool AddSymbol(std::string id, int type, std::vector<Node> args, bool isGlobal); // maybe make this bool
        bool AddSymbolToParentScope(std::string id, int type, std::vector<Node> args, bool isGlobal); // maybe make this bool
        bool DoesSymbolExist(std::string id, bool &isGlobal, Node &n);

        int getScope();


    private:
        int currScope;
        std::vector<std::map<std::string, Node> > symTableScopes;
        std::map<std::string, Node>::iterator it;

        // Debug Methods
        void PrintScopes();
        //void ChangeScope(int scope);
};


#endif //COMPILER_THEORY_SYMBOLTABLE_H
