
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

// error codes

#define parseError			0x01
#define missingVarError		0x02
#define missingFuncError	0x04

// unassigned data masks

#define mValue   			0x01
#define mVariable			0x10
#define mFunction			0x20

// data types

#define tNumber   			(mValue)
#define tVariable			(mVariable | mValue)
#define tFunction			(mFunction | mValue)

// tokens
#define tName				(mVariable | mFunction)

// types and structures
typedef union {
	struct IndexPair {
		UInt16 iStart;		// start index of token associated value in TokenList expression string
		UInt16 iEnd;		// end index of token associated value in TokenList expression string
		UInt32 padding;
	} indexPair ;
	FuncRef funcRef ;
	double value;			// constant or variable value
} TokenData;

typedef struct TokenCell {
	struct TokenCell * nextP;	// next token in the list
	TokenData data;
	UInt8 dataType;				// assigned or unassigned data
	UInt8 token;				// token defined above or char matched (no associated value)
} TokenCell;

typedef struct TokenList {
	TokenCell * headP;			// head of list
	TokenCell * cellP;			// current cell
	Char * exprStr;				// expression string
} TokenList;

typedef struct VarCell {
	struct VarCell * nextP;
	Char * name;
	double value;
} VarCell;

typedef struct VarList {
	VarCell * headP;			// head of list
	VarCell * cellP;			// current cell
	Char * varsStr;				// variables declaration string
} VarList;


// functions

UInt8 TokenizeExpression (TokenList * tokL);
UInt8 ParseVariables (VarList * varL);
UInt8 AssignTokenValue (TokenList * tokL, VarList * varL);

// transitions

#define isNumber(c)		(c >= '0' && c <= '9')
#define isLetter(c)		((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define isDot(c)		(c == '.')
#define isOpen(c)		(c == '(')
#define isClose(c)		(c == ')')
#define isOperator(c)	(c == '+' || c == '-' || c == '*' || c == '/' || c == '^')

#define isSeparator(c)	(c == ' ' || c == '\t' || c == '\n' || c == 0x0d || c == 0x0a)


#endif // MEMOCALCLEXER_H
