//
// Created by Nick Clason on 2/1/21.
//
// Description:
//      Define macros
//      starting at 257 to avoid conflicts
//
#ifndef COMPILER_THEORY_DEFINITIONS_H
#define COMPILER_THEORY_DEFINITIONS_H

#include <iostream>

// Single Character ASCII Tokens
//
#define T_SEMICOLON      257     // ;
#define T_PERIOD         258     // .
#define T_LPAREN         259     // (
#define T_RPAREN         260     // )
#define T_COMMA          261     // ,
#define T_LBRACKET       262     // [
#define T_RBRACKET       263     // ]
#define T_LBRACE         264     // {
#define T_RBRACE         265     // }
#define T_AND            266     // &
#define T_OR             267     // |
#define T_ADD            268     // +
#define T_SUBTRACT       269     // -
#define T_MULTIPLY       270     // *
#define T_DIVIDE         271     // /
#define T_LESSTHAN       272     // <
#define T_GREATERTHAN    273     // >
#define T_LTEQ           274     // <=
#define T_GTEQ           275     // >=
#define T_EQEQ           276     // ==
#define T_NOTEQ          277     // !=
#define T_ASSIGNMENT     278     // :=
#define T_COLON          307     // :


// Reserved Words
//
#define T_PROGRAM        279     // "PROGRAM"
#define T_IS             280     // "IS"
#define T_BEGIN          281     // "BEGIN"
#define T_END            282     // "END"
#define T_GLOBAL         283     // "GLOBAL"
#define T_PROCEDURE      284     // "PROCEDURE"
#define T_VARIABLE       285     // "VARIABLE"
#define T_TYPE           286     // "TYPE"
#define T_IF             287     // "IF"
#define T_THEN           288     // "THEN"
#define T_ELSE           289     // "ELSE"
#define T_RETURN         290     // "RETURN"
#define T_NOT            291     // "NOT"
#define T_TRUE           292     // "TRUE"
#define T_FALSE          293     // "FALSE"
#define T_FOR            294     // "FOR"


// Types
//
#define T_INTEGER        295     // "INTEGER"
#define T_FLOAT          296     // "FLOAT"
#define T_STRING         297     // "STRING"
#define T_BOOL           298     // "BOOL"
#define T_ENUM           299     // "ENUM"


// Miscellaneous
//
#define T_IDENTIFIER     300      // identifier
#define T_INT_LITERAL    301      // integer literal
#define T_FLOAT_LITERAL  302      // float literal
#define T_STRING_LITERAL 303      // string literal
#define T_UNKNOWN        304      // unknown identifier/invalid characters
#define T_EOF            305      // end of file
#define T_COMMENT        306      // comment


// Token Structure
//
struct token_t
{
    int type;   // token defined above
    int col;    // starting column
    int line;   // line number
    std::string str;

    union {
        char stringValue[256];  // holds value if string/identifier
        int intValue;           // holds value if integer
        float floatValue;       // holds value if float
    } val;
};

#endif //COMPILER_THEORY_DEFINITIONS_H
