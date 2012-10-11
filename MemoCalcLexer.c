
/***********************************************************************
 *
 * FILE : MemoCalcLexer.c
 * 
 * DESCRIPTION : Lexical analyser for MemoCalc
 * 
 * COPYRIGHT : (C) 2003 Luc Yriarte
 * 
 *
 ***********************************************************************/

#define TRACE_OUTPUT TRACE_OUTPUT_ON

#include <PalmOS.h>
#include <FloatMgr.h>
#include <TraceMgr.h>

#include "MemoCalcFunctions.h"
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
#define qOpen			5		// read an open parenthesis
#define qClose			6		// read a close parenthesis
#define qOperator		7		// read an operator
#define qInvalidState	0xff

#define dataState(q)		(q >= qInteger && q <= qName)
#define tokenState(q)		(q >= qOpen && q <= qOperator)

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
			if (isNumber(c)) 
				return qName;
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
 *		must be set. If the list is non empty, tokens are appened
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
				case qFloat:
					exprP->token = tNumber;
				break;
				case qName:
					exprP->token = tName;
				break;
			}
			// match string value in exprStr
			exprP->data.indexPair.iStart = iStart;
			exprP->data.indexPair.iEnd = iEnd;
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
			exprP->data.indexPair.iStart = exprP->data.indexPair.iEnd = iNext;
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


/***********************************************************************
 *
 * FUNCTION:    ParseVariables 
 *
 * DESCRIPTION: Generate a varList from a vars declaration string. The
 *		variables declaration automata is simple enough there's no need
 *		for a transition function. The corresponding regexp is :
 *		[A-Za-z]+\s*\=\s*\-?\s+[0-9]+\.?[0-9]*
 *
 * PARAMETERS:  Pointer to a VarList structure. The vars string
 *		must be set, and the list empty.
 *
 * NOTE: The vars string is modified by this function! A null char is
 *		set at the end of each var name, and the varCell->name pointer
 *		directly refers to it.
 *
 * RETURNED:	0 if no error occurred
 *
 ***********************************************************************/

UInt8 ParseVariables (VarList * varL)
{
	FlpCompDouble tmpF;
	VarCell * varP, * lastP;
	UInt16 iStart, iNext, iEnd;
	Char tmpC;
	UInt8 err = 0;

	if (! varL->varsStr)
		return err;

	iStart = iNext = iEnd = 0;
	varP = lastP = varL->headP;

	while (varL->varsStr[iNext])
	{
		while(isSeparator(varL->varsStr[iNext]))
			++iNext;

		// error or end of buffer
		if (!(varL->varsStr[iNext]))
			break;
		err = parseError | missingVarError;
		if (!isLetter(varL->varsStr[iNext]))
			break;

		// add a new varCell
		varP = MemPtrNew(sizeof(VarCell));
		varP->nextP = NULL;
		if (!lastP)
			lastP = varL->headP = varP;
		else 
			lastP->nextP = varP;
		// shift list
		lastP = varP;

		// read a new variable 
		iStart = iNext;
		while (isLetter(varL->varsStr[iNext]))
			iNext++;
		iEnd = iNext;
		while(isSeparator(varL->varsStr[iNext]))
			++iNext;

		// check '=' for affectation
		if (varL->varsStr[iNext] != '=')
			break;
		++iNext;
		while(isSeparator(varL->varsStr[iNext]))
			++iNext;

		// replace '=' or first whitespace after varname by a null char and set pointer
		varL->varsStr[iEnd] = nullChr;
		varP->name = varL->varsStr + iStart;

		// read variable value
		iStart = iNext;
		if (varL->varsStr[iNext] == '-')
			++iNext;
		if (!isNumber(varL->varsStr[iNext]))
			break;
		while (isNumber(varL->varsStr[iNext]))
			iNext++;
		if (isDot(varL->varsStr[iNext]))
		{
			iNext++;
			while (isNumber(varL->varsStr[iNext]))
				iNext++;
		}
		iEnd = iNext;

		// set a temporary null char to read the number string
		tmpC = varL->varsStr[iEnd];
		varL->varsStr[iEnd] = nullChr;
		FlpBufferAToF(&(tmpF.fd), varL->varsStr + iStart);

		varP->value = tmpF.d;
		varL->varsStr[iEnd] = tmpC;

		err = 0;
	}
	
	// reset current cell and return end of buffer
	varL->cellP = varL->headP;
	return err;
}


/***********************************************************************
 *
 * FUNCTION:    AssignTokenValue 
 *
 * DESCRIPTION: Assign a value for numeric tokens, names corresponding
 *		to a variable, or a function.
 *		Set token data types.
 *
 * NOTE: The expr string has to be in write access.
 *
 * PARAMETERS:  token list, variables list.
 *
 * RETURNED:	0
 *
 ***********************************************************************/

UInt8 AssignTokenValue (TokenList * tokL, VarList * varL)
{
	FlpCompDouble tmpF;
	UInt8 err = 0;
	Char tmpC;

	tokL->cellP = tokL->headP;
	while (tokL->cellP)
	{
		switch (tokL->cellP->token)
		{
			case tNumber:
				tokL->cellP->dataType = tNumber;
				tmpC = tokL->exprStr[tokL->cellP->data.indexPair.iEnd+1];
				tokL->exprStr[tokL->cellP->data.indexPair.iEnd+1] = 0;
				FlpBufferAToF(&(tmpF.fd), tokL->exprStr + tokL->cellP->data.indexPair.iStart);
				tokL->exprStr[tokL->cellP->data.indexPair.iEnd+1] = tmpC;
				tokL->cellP->data.value = tmpF.d;
			break;

			case tName:
			    if (tokL->cellP->nextP && tokL->cellP->nextP->token == '(')
			    {
           			tokL->cellP->dataType = mFunction;
        			if (GetFunc(&(tokL->cellP->data.funcRef), tokL->exprStr + tokL->cellP->data.indexPair.iStart,
        				1 + tokL->cellP->data.indexPair.iEnd - tokL->cellP->data.indexPair.iStart) == 0)
                    {
                        tokL->cellP->dataType |= mValue;
                    }	
					else
						err |= missingFuncError;
			    }
                else
                {
           			tokL->cellP->dataType = mVariable;
        			varL->cellP = varL->headP;
        			while (varL->cellP)
        			{
        				if (StrNCompare(varL->cellP->name, tokL->exprStr + tokL->cellP->data.indexPair.iStart,
        					1 + tokL->cellP->data.indexPair.iEnd - tokL->cellP->data.indexPair.iStart) == 0)
        				{
							tokL->cellP->data.value = varL->cellP->value;
							tokL->cellP->dataType |= mValue;
							break;
        				}
        				varL->cellP = varL->cellP->nextP;
        			}
					if (!(tokL->cellP->dataType & mValue))
					{
						if (GetConst(&(tokL->cellP->data.value), tokL->exprStr + tokL->cellP->data.indexPair.iStart,
        					1 + tokL->cellP->data.indexPair.iEnd - tokL->cellP->data.indexPair.iStart) == 0)
							tokL->cellP->dataType = tConstant;
						else
							err |= missingVarError;
					}
				}
			break;
		}
		tokL->cellP = tokL->cellP->nextP;
	}

	tokL->cellP = tokL->headP;
	return err;
}

