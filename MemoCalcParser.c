
/***********************************************************************
 *
 * FILE : MemoCalcParser.c
 * 
 * DESCRIPTION : Grammatical analyser for MemoCalc
 * 
 * COPYRIGHT : GNU GENERAL PUBLIC LICENSE
 * http://www.gnu.org/licenses/gpl.txt
 *
 ***********************************************************************/


#if defined(__INTEL__) || defined(__i386__) || defined(WIN32)

#include <malloc.h>
#include <stdio.h>

#define UInt16 unsigned short
#define UInt8 unsigned char
#define Char char

#define MemPtrNew malloc

#else

#include <PalmOS.h>

#endif

#include "MemoCalcLexer.h"
#include "MemoCalcParser.h"


/***********************************************************************
 *
 *	Non terminal LL(1) grammar production rules description
 *
 *	E -> T [epsilon | '+' E | '-' E]
 *	T -> F [epsilon | '*' T | '/' T]
 *	F -> X [epsilon | '^' F]
 *	X -> '-' N | N
 *	N -> number | string | string '(' E ')' | '(' E ')'
 *
 ***********************************************************************/

static UInt8 ruleE(TokenList * tokL);	// Expression
static UInt8 ruleT(TokenList * tokL);	// Term
static UInt8 ruleF(TokenList * tokL);	// Factor
static UInt8 ruleX(TokenList * tokL);	// eXponent
static UInt8 ruleN(TokenList * tokL);	// Negative


// E -> T [epsilon | '+' E | '-' E]
static UInt8 ruleE(TokenList * tokL)
{	
	UInt8 err = parseOk;

	err = ruleT(tokL);
	if (!tokL->cellP || err != parseOk)
		return err;

	switch (tokL->cellP->token)
	{
		case '+' :
		case '-' :
			tokL->cellP = tokL->cellP->nextP;
			err = ruleE(tokL);
		break;
	}

	return err;
}

// T -> F [epsilon | '*' T | '/' T]
static UInt8 ruleT(TokenList * tokL)
{	
	UInt8 err = parseOk;

	err = ruleF(tokL);
	if (!tokL->cellP || err != parseOk)
		return err;

	switch (tokL->cellP->token)
	{
		case '*' :
		case '/' :
			tokL->cellP = tokL->cellP->nextP;
			err = ruleT(tokL);
		break;
	}

	return err;
}

// F -> X [epsilon | '^' F]
static UInt8 ruleF(TokenList * tokL)
{	
	UInt8 err = parseOk;

	err = ruleX(tokL);
	if (!tokL->cellP || err != parseOk)
		return err;

	switch (tokL->cellP->token)
	{
		case '^' :
			tokL->cellP = tokL->cellP->nextP;
			err = ruleF(tokL);
		break;
	}

	return err;
}

// X -> '-' N | N
static UInt8 ruleX(TokenList * tokL)
{	
	UInt8 err = parseOk;

	if (!tokL->cellP)
		return parseError;

	if (tokL->cellP->token == '-')
		tokL->cellP = tokL->cellP->nextP;

	err = ruleN(tokL);

	return err;
}

// N -> number | string | string '(' E ')' | '(' E ')'
static UInt8 ruleN(TokenList * tokL)
{	
	UInt8 err = parseOk;

	if (!tokL->cellP)
		return parseError;

	switch (tokL->cellP->token)
	{
		case tInteger :
		case tFloat :
			tokL->cellP = tokL->cellP->nextP;
		break;

		case tName :
			tokL->cellP = tokL->cellP->nextP;
			if (!tokL->cellP || tokL->cellP->token != '(')
				break;

		case '(' :
			tokL->cellP = tokL->cellP->nextP;
			err = ruleE(tokL);
			if (tokL->cellP && tokL->cellP->token == ')')
				tokL->cellP = tokL->cellP->nextP;
			else
				err = parseError;
		break;

		default :
			err = parseError;
	}

	return err;
}



#if defined(__INTEL__) || defined(__i386__) || defined(WIN32)

void main()
{
	TokenList tokL = { NULL, NULL, "toto - fonc(2 ^-1) - 45.2" } ;
	UInt8 err;

	err = TokenizeExpression(&tokL);
	err = ruleE(&tokL);
}

#endif
