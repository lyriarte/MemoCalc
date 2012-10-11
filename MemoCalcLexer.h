
/***********************************************************************
 *
 * FILE : MemoCalcLexer.h
 * 
 * DESCRIPTION : Lexical analyser headers for MemoCalc
 * 
 * COPYRIGHT : GNU GENERAL PUBLIC LICENSE
 * http://www.gnu.org/licenses/gpl.txt
 *
 ***********************************************************************/

#ifndef MEMOCALCLEXER_H
#define MEMOCALCLEXER_H

// tokens

#define tInteger		1
#define tFloat			2
#define tName			3

// structures

typedef struct TokenCell {
	struct TokenCell * nextP;	// next token in the list
	UInt16 iStart;				// start index of token associated value in TokenList expression string
	UInt16 iEnd;				// end index of token associated value in TokenList expression string
	UInt8 token;				// token defined above or char matched (no associated value)
} TokenCell;

typedef struct TokenList {
	TokenCell * headP;			// head of list
	TokenCell * cellP;			// current cell
	Char * exprStr;				// expression string
} TokenList;

// functions

UInt8 TokenizeExpression (TokenList * tokL);

#endif // MEMOCALCLEXER_H