
/***********************************************************************
 *
 * FILE : MemoCalcLexer.h
 * 
 * DESCRIPTION : Lexical analyser headers for MemoCalc
 * 
 * COPYRIGHT : (C) 2003 Luc Yriarte
 * 
 *
 ***********************************************************************/

#ifndef MEMOCALCLEXER_H
#define MEMOCALCLEXER_H

// error codes

#define parseError			0x01
#define missingVarError		0x02
#define missingFuncError	0x04
#define mathError			0x08

// unassigned data masks

#define mValue   			0x01
#define mConstant  			0x02
#define mVariable			0x10
#define mFunction			0x20

// data types

#define tNumber   			(mValue)
#define tConstant			(mConstant | mValue)
#define tVariable			(mVariable | mValue)
#define tFunction			(mFunction | mValue)

// tokens
#define tName				(mConstant | mVariable | mFunction)

// atof, ftoa
#define kFlpBufSize			80

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
#define isHex(c)		((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define isHexNumber(c)	(isNumber(c) || isHex(c))
#define isHexTag(c)		(c == 'x' || c == 'X')
#define isDot(c)		(c == '.')
#define isOpen(c)		(c == '(')
#define isClose(c)		(c == ')')
#define isArithmetic(c)	(c == '+' || c == '-' || c == '*' || c == '/' || c == '^')
#define isBoolean(c)	(c == '&' || c == '|' || c == '~')
#define isOperator(c)	(isBoolean(c) || isArithmetic(c))

#define isSeparator(c)	(c == ' ' || c == '\t' || c == '\n' || c == 0x0d || c == 0x0a)


#endif // MEMOCALCLEXER_H
