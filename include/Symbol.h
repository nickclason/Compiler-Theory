//
// Created by Nick Clason on 2/11/21.
//

#ifndef COMPILER_THEORY_SYMBOL_H
#define COMPILER_THEORY_SYMBOL_H

#include "definitions.h"

#include <string>
#include <vector>

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
        type = T_UNKNOWN;
        declarationType = T_UNKNOWN;
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

#endif //COMPILER_THEORY_SYMBOL_H
