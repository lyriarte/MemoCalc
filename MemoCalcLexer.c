
/***********************************************************************
 *
 * FILE : MemoCalcLexer.c
 * 
 * DESCRIPTION : Lexical analyser for MemoCalc
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


/***********************************************************************
 *
 *	Finished state automata description
 *
 ***********************************************************************/

// states

#define qStop			0		// parsed string terminator
#define qStart			1		// initial state
#define qInteger		2		// parsed an integer
#define qFloat			3		// parsed a float
#define qName			4		// parsed a name
#define qOpen			5		// parsed an open parenthesis
#define qClose			6		// parsed a close parenthesis
#define qOperator		7		// parsed an operator
#define qInvalidState	0xff

#define dataState(q)		(q >= qInteger && q <= qName)
#define tokenState(q)		(q >= qOpen && q <= qOperator)

// transitions

#define isNumber(c)		(c >= '0' && c <= '9')
#define isLetter(c)		((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define isDot(c)		(c == '.')
#define isOpen(c)		(c == '(')
#define isClose(c)		(c == ')')
#define isOperator(c)	(c == '+' || c == '-' || c == '*' || c == '/' || c == '^')

#define isSeparator(c)	(c == ' ' || c == '\t' || c == '\n' || c == 0x0d || c == 0x0a)


/***********************************************************************
 *
 * FUNCTION:    GetNextState
 *
 * DESCRIPTION: Finished state automata transition function. Given the 
 *		current state and input char, returns the next state.
 *
 * PARAMETERS:  Current state, input char
 *
 * RETURNED:    Next state
 *
 ***********************************************************************/

static UInt8 GetNextState (UInt8 q, Char c)
{
	if (!c)
		return qStop;

	switch (q)
	{
		case qStart:
			if (isNumber(c)) 
				return qInteger;
			if (isLetter(c)) 
				return qName;
			if (isOpen(c)) 
				return qOpen;
			if (isOperator(c)) 
				return qOperator;
		break;

		case qInteger:
			if (isNumber(c)) 
				return qInteger;
			if (isDot(c)) 
				return qFloat;
			if (isOperator(c)) 
				return qOperator;
			if (isClose(c)) 
				return qClose;
		break;

		case qFloat:
			if (isNumber(c)) 
				return qFloat;
			if (isOperator(c)) 
				return qOperator;
			if (isClose(c)) 
				return qClose;
		break;

		case qName:
			if (isLetter(c)) 
				return qName;
			if (isOpen(c)) 
				return qOpen;
			if (isOperator(c)) 
				return qOperator;
			if (isClose(c)) 
				return qClose;
		break;

		case qOpen:
			if (isNumber(c)) 
				return qInteger;
			if (isLetter(c)) 
				return qName;
			if (isOpen(c)) 
				return qOpen;
			if (isOperator(c)) 
				return qOperator;
		break;

		case qClose:
			if (isOperator(c)) 
				return qOperator;
			if (isClose(c)) 
				return qClose;
		break;

		case qOperator:
			if (isNumber(c)) 
				return qInteger;
			if (isLetter(c)) 
				return qName;
			if (isOpen(c)) 
				return qOpen;
			if (isOperator(c)) 
				return qOperator;
		break;

	}

	return qInvalidState;
}


/***********************************************************************
 *
 * FUNCTION:    TokenizeExpression 
 *
 * DESCRIPTION: Generate a tokenList from an expression string
 *
 * PARAMETERS:  Pointer to a TokenList structure. The expression string
 *		must be set, and the token list empty
 *
 * RETURNED:	0 (qStop) if no error occurred
 *
 ***********************************************************************/

UInt8 TokenizeExpression (TokenList * tokL)
{
	TokenCell * exprP, * lastP;
	UInt16 iStart, iNext, iEnd;
	UInt8 lastState, nextState;

	if (!tokL || !tokL->exprStr)
		return qInvalidState ;

	exprP = lastP = tokL->headP;
	iStart = iNext = iEnd = 0;
	lastState = nextState = qStart;

	while (nextState != qStop && nextState != qInvalidState)
	{
		// eat separators
		while(isSeparator(tokL->exprStr[iNext]))
			++iNext;

		nextState = GetNextState(lastState, tokL->exprStr[iNext]);

		// character at iStart begins the string value in exprStr
		if (!dataState(lastState) && dataState(nextState))
			iStart = iNext;

		// add a new token for lastState if a dataState is completed
		if (dataState(lastState) && lastState != nextState
		&& !(lastState == qInteger && nextState == qFloat))
		{
			exprP = MemPtrNew(sizeof(TokenCell));
			exprP->nextP = NULL;
			if (!lastP)
				lastP = tokL->headP = exprP;
			else 
				lastP->nextP = exprP;
			// shift list
			lastP = exprP;
			// set the token
			switch (lastState)
			{
				case qInteger:
					exprP->token = tInteger;
				break;
				case qFloat:
					exprP->token = tFloat;
				break;
				case qName:
					exprP->token = tName;
				break;
			}
			// match string value in exprStr
			exprP->iStart = iStart;
			exprP->iEnd = iEnd;
		}

		// add a new token for nextState if it is not a dataState
		if (tokenState(nextState))
		{
			exprP = MemPtrNew(sizeof(TokenCell));
			exprP->nextP = NULL;
			if (!lastP)
				lastP = tokL->headP = exprP;
			else 
				lastP->nextP = exprP;
			// shift list
			lastP = exprP;
			// set the the matched char as token
			exprP->token = tokL->exprStr[iNext];
			// match token in exprStr
			exprP->iStart = exprP->iEnd = iNext;
		}

		// shift state
		lastState = nextState;
		// character at iEnd is the last non-separator
		iEnd = iNext ;
		++iNext;
	}

	// reset current cell and return last state
	tokL->cellP = tokL->headP;
	return lastState;
}
