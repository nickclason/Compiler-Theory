//
// Created by Nick Clason on 1/21/21.
//

#include "../include/definitions.h"
#include "../include/Scanner.h"

#include <iostream>

using namespace std;

/* Constructor

History:
Date             Description
====================================================================
21-Jan-2021      Initial
*/
Scanner::Scanner()
{
}

/* Destructor

History:
Date             Description
====================================================================
21-Jan-2021      Initial
*/
Scanner::~Scanner()
{
    if (filePtr != nullptr)
    {
        fclose(filePtr);
    }
}


/*
Method: GenerateReservedTable()
Description: Map values to tokens defined in definitions.h

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
std::map<std::string, int> Scanner::GenerateReservedTable()
{
    std::map<std::string, int> map;

    map[";"]  = T_SEMICOLON;
    map["."]  = T_PERIOD;
    map["("]  = T_LPAREN;
    map[")"]  = T_RPAREN;
    map[","]  = T_COMMA;
    map["["]  = T_LBRACKET;
    map["]"]  = T_RBRACKET;
    map["{"]  = T_LBRACE;
    map["}"]  = T_RBRACE;
    map["&"]  = T_AND;
    map["|"]  = T_OR;
    map["+"]  = T_ADD;
    map["-"]  = T_SUBTRACT;
    map["*"]  = T_MULTIPLY;
    map["/"]  = T_DIVIDE;
    map["<"]  = T_LESSTHAN;
    map[">"]  = T_GREATERTHAN;
    map["<="] = T_LTEQ;
    map[">="] = T_GTEQ;
    map["=="] = T_EQEQ;
    map["!="] = T_NOTEQ;
    map[":="] = T_ASSIGNMENT;
    map[":"]  = T_COLON;

    map["PROGRAM"]  = T_PROGRAM;
    map["IS"]       = T_IS;
    map["BEGIN"]    = T_BEGIN;
    map["END"]      = T_END;
    map["GLOBAL"]   = T_GLOBAL;
    map["PROCEDURE"]= T_PROCEDURE;
    map["VARIABLE"] = T_VARIABLE;
    map["TYPE"]     = T_TYPE;
    map["IF"]       = T_IF;
    map["THEN"]     = T_THEN;
    map["ELSE"]     = T_ELSE;
    map["RETURN"]   = T_RETURN;
    map["NOT"]      = T_NOT;
    map["TRUE"]     = T_TRUE;
    map["FALSE"]    = T_FALSE;
    map["FOR"]      = T_FOR;

    map["INTEGER"]  = T_INTEGER;
    map["FLOAT"]    = T_FLOAT;
    map["STRING"]   = T_STRING;
    map["BOOL"]     = T_BOOL;
    map["ENUM"]     = T_ENUM;

    return map;
}


/*
Method: InitScanner()
Description: Initialize scanner and member variables

History:
Date             Description
====================================================================
21-Jan-2021      Initial
*/
bool Scanner::InitScanner(string fileName)
{
    lineCount = 1;
    colCount = 1;
    filePtr = fopen(fileName.c_str(), "r");

    // handle non-existent file pointer
    if (filePtr == nullptr)
    {
        cout << "File: " << fileName << " cannot be opened" << endl;
        return false;
    }

    reservedTable = this->GenerateReservedTable();

    return true;
}


/*
Method: ScanOneToken()
Description: Scan tokens one at a time

History:
Date             Description
====================================================================
21-Jan-2021      Initial
26-Jan-2021      Modify number literals to handle '_' in numbers
*/
int Scanner::ScanOneToken(FILE* filePtr, token_t* token)
{
    char ch, nextCh;

    ch = ScanNextChar();

    while (isSpace(ch)) // keep reading until non-space char
    {
        ch = ScanNextChar();
    }


    /* I think that technically, we can just eat the comments
     * and not return actual tokens for them, similar to how whitespace is just ignored.
     * For debug purposes, however, this is completely acceptable. */

    if (ch == '/') // comment or division
    {
        std::string commentStr = "";
        commentStr += ch;
        nextCh = ScanNextChar();

        if (nextCh == '/') // Single line comment
        {
            while (nextCh != '\n')
            {
                commentStr += nextCh;
                nextCh = ScanNextChar();
            }

            token->str = commentStr;
            for (int i = 0; i < commentStr.length(); i++)
            {
                token->val.stringValue[i] = commentStr[i];
            }

            return T_COMMENT;

        }
        else if (nextCh == '*') // Block comment and nested block comments
        {
            commentStr += nextCh;
            int nestedCount = 1;

            while (nestedCount > 0)
            {
                nextCh = ScanNextChar();
                commentStr += nextCh;

                if (nextCh == '*')
                {
                    nextCh = ScanNextChar();
                    commentStr += nextCh;
                    if (nextCh == '/')
                    {
                        nestedCount -= 1;
                    }
                }
                else if (nextCh == '/')
                {
                    nextCh = ScanNextChar();
                    commentStr += nextCh;
                    if (nextCh == '*')
                    {
                        nestedCount += 1;
                    }
                }
                else if (nextCh == EOF)
                {
                    // Not sure about this but if the block comment is never
                    // closed the code just gets stuck in an infinite loop.
                    //
                    return T_UNKNOWN;
                }
            }

            token->str = commentStr;
            for (int i = 0; i < commentStr.length(); i++)
            {
                token->val.stringValue[i] = commentStr[i];
            }

            return T_COMMENT;
        }
        else // Division
        {
            UndoScan(nextCh);
            token->str = ch;
            token->val.stringValue[0] = ch;

            return T_DIVIDE;
        }
    }
    else if (isNum(ch)) // number literal
    {
        bool isFloatingPoint = false;
        std::string numberStr = "";
        numberStr += ch;
        nextCh = ScanNextChar();

        while (isNum(nextCh) ||
               (nextCh == '.' && !isFloatingPoint) ||
               (nextCh == '_'))
        {
            if (nextCh != '_') // ignore underscores
            {
                numberStr += nextCh;
            }

            if (nextCh == '.')
            {
                isFloatingPoint = true;
            }

            nextCh = ScanNextChar();
        }

        UndoScan(nextCh);
        token->str = numberStr;

        if (isFloatingPoint)
        {
            token->val.floatValue = stof(numberStr);
            return T_FLOAT_LITERAL;
        }
        else
        {
            token->val.intValue = stoi(numberStr);
            return T_INT_LITERAL;
        }
    }
    else if (ch == '\"') // string literal
    {
        std::string stringStr = "";
        stringStr += ch;
        nextCh = ScanNextChar();

        int iCh = 0;
        while (nextCh != '\"') // just continue until end-quote
        {
            token->val.stringValue[iCh++] = nextCh;
            stringStr += nextCh;
            nextCh = ScanNextChar();
        }

        stringStr += nextCh;
        token->str = stringStr;

        return T_STRING_LITERAL;
    }
    else if (isAlpha(ch)) // identifiers
    {
        std::string letterStr = "";
        int iCh = 0;
        token->val.stringValue[iCh++] = toupper(ch);
        letterStr += toupper(ch);
        nextCh = ScanNextChar();
        while(isAlpha(nextCh) || isNum(nextCh) || nextCh == '_')
        {
            token->val.stringValue[iCh++] = toupper(nextCh);
            letterStr += toupper(nextCh);
            nextCh = ScanNextChar();
        }

        UndoScan(nextCh);
        token->str = letterStr;

        map<string,int>::iterator it;
        it = reservedTable.find(letterStr);
        if (it != reservedTable.end()) return reservedTable.find(letterStr)->second;
        else return T_IDENTIFIER;
    }
    else if (isSingleToken(ch)) // tokens/operators
    {
        std::string tokenStr = "";
        int iCh = 0;
        token->val.stringValue[iCh++] = ch;
        tokenStr += ch;

        if (ch == '>' || ch == '<' || ch == '=' || ch == '!' || ch == ':')
        {
            nextCh = ScanNextChar();
            if (nextCh == '=')
            {
                tokenStr += nextCh;
                token->val.stringValue[iCh++] = nextCh;
            }
            else
            {
                UndoScan(nextCh);
            }
        }

        token->str = tokenStr;

        map<string,int>::iterator it;
        it = reservedTable.find(tokenStr);
        if (it != reservedTable.end()) return reservedTable.find(tokenStr)->second;
        else return T_UNKNOWN;
    }
    else if (ch == EOF) // end of file
    {
        return T_EOF;
    }
    else // unknown
    {
        std::string unknownStr = "";
        unknownStr += ch;
        token->str = unknownStr;
        return T_UNKNOWN;
    }
}


/*
Method: GetToken()
Description: Gets a single token

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
token_t* Scanner::GetToken()
{
    token_t* newToken = new token_t();

    newToken->type = ScanOneToken(filePtr, newToken);
    newToken->line = lineCount;
    newToken->col = colCount;

    return newToken;
}


/*
Method: isNum()
Description: Returns true if input char is a number, false otherwise

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
bool Scanner::isNum(char c)
{
    return (c >= '0') && (c <= '9');
}


/*
Method: isAlpha()
Description: Returns true if input char is a letter, false otherwise

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
bool Scanner::isAlpha(char c)
{
    return ((c >= 'a') && (c <= 'z')) ||
           (c >= 'A') && (c <= 'Z');
}


/*
Method: isSingleToken()
Description: Returns true if input char is a single token, false otherwise

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
bool Scanner::isSingleToken(char c)
{
    switch (c)
    {
        case ';':
        case '.':
        case ',':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '&':
        case '|':
        case '+':
        case '-':
        case '*':
        case '<':
        case '>':
        case '=':
        case '!':
        case ':':
            return true;

        default:
            return false;
    }
}


/*
Method: isSpace()
Description: Returns true if input char is a space character, false otherwise

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
bool Scanner::isSpace(char c)
{
    int asciiValue = (int)c;

    if (asciiValue <= 32 && c != EOF)
    {
        return true;
    }

    return false;
}


/*
Method: ScanNextChar()
Description: Gets the next character and handles line/column counts

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
char Scanner::ScanNextChar()
{
    char ch = getc(filePtr);
    int asciiValue = (int)ch;

    if (ch == '\n')
    {
        lineCount++;
        prevColCount = colCount;
        colCount = 0;
    }

    colCount++;
    return ch;
}


/*
Method: UndoScan()
Description: Un-scans the input character and adjusts counters

History:
Date             Description
====================================================================
22-Jan-2021      Initial
*/
void Scanner::UndoScan(char c)
{
    if (c == '\n')
    {
        lineCount--;
        colCount = prevColCount;
        ungetc(c, filePtr);

        return;
    }
    colCount--;
    ungetc(c, filePtr);
}

token_t* Scanner::PeekToken()
{
    token_t* newToken = new token_t();

    // get starting position of the FILE*
    fpos_t pos;
    fgetpos(filePtr, &pos);

    int prevLineCount = lineCount;
    int prevColCount = colCount;

    newToken->type = ScanOneToken(filePtr, newToken);
    newToken->line = lineCount;
    newToken->col = colCount;

    // reset position
    fsetpos(filePtr, &pos);
    lineCount = prevLineCount;
    colCount = prevColCount;

    return newToken;
}