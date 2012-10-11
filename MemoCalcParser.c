
/***********************************************************************
 *
 * FILE : MemoCalcParser.c
 * 
 * DESCRIPTION : Grammatical analyser for MemoCalc
 * 
 * COPYRIGHT : (C) 2003 Luc Yriarte
 * 
 *
 ***********************************************************************/

#define TRACE_OUTPUT TRACE_OUTPUT_ON

#include <PalmOS.h>
#include <FloatMgr.h>
#include <TraceMgr.h>

#include "MathLib.h"
#include "MemoCalcFunctions.h"
#include "MemoCalcLexer.h"
#include "MemoCalcParser.h"

extern UInt16 MathLibRef;

/***********************************************************************
 *
 *	Private definitions
 *
 ***********************************************************************/

// structures

typedef struct ExprNode {
	struct ExprNode * leftP;
	struct ExprNode * rightP;
	TokenData data;
	UInt8 dataType;   // value for leaf nodes, funcRef or NULL for '(' nodes
	UInt8 token;
} ExprNode;

typedef struct ExprTree {
	ExprNode * rootP;
	ExprNode * nodeP;
} ExprTree;

// functions
static ExprNode * NewExprNode(ExprNode * leftP, ExprNode * rightP, double value, UInt8 dataType, UInt8 token);


/***********************************************************************
 *
 *	Non terminal LL(1) grammar production rules description
 *
 *	E -> T [epsilon | '+' E | '-' E]
 *	T -> F [epsilon | '*' T | '/' T]
 *	F -> X [epsilon | '^' F]
 *	X -> '-' N | N
 *	N -> number | name | name '(' E ')' | '(' E ')'
 *
 ***********************************************************************/

static UInt8 ruleE (TokenList * tokL, ExprTree * exprT);	// Expression
static UInt8 ruleT (TokenList * tokL, ExprTree * exprT);	// Term
static UInt8 ruleF (TokenList * tokL, ExprTree * exprT);	// Factor
static UInt8 ruleX (TokenList * tokL, ExprTree * exprT);	// eXponent
static UInt8 ruleN (TokenList * tokL, ExprTree * exprT);	// Negative

// E -> T [epsilon | '+' E | '-' E]
static UInt8 ruleE (TokenList * tokL, ExprTree * exprT)
{	
	ExprNode * nodeP;
	UInt8 err = 0;

	err |= ruleT(tokL, exprT);
	if (!tokL->cellP || err)
		return err;

	switch (tokL->cellP->token)
	{
		case '+' :
		case '-' :
			nodeP = NewExprNode(exprT->nodeP, NULL, 0, 0, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
			err |= ruleE(tokL, exprT);
			if (err)
				break;
			nodeP->rightP = exprT->nodeP;
			exprT->nodeP = nodeP;
		break;
	}

	return err;
}

// T -> F [epsilon | '*' T | '/' T]
static UInt8 ruleT (TokenList * tokL, ExprTree * exprT)
{	
	ExprNode * nodeP;
	UInt8 err = 0;

	err |= ruleF(tokL, exprT);
	if (!tokL->cellP || err)
		return err;

	switch (tokL->cellP->token)
	{
		case '*' :
		case '/' :
			nodeP = NewExprNode(exprT->nodeP, NULL, 0, 0, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
			err |= ruleT(tokL, exprT);
			if (err)
				break;
			nodeP->rightP = exprT->nodeP;
			exprT->nodeP = nodeP;
		break;
	}

	return err;
}

// F -> X [epsilon | '^' F]
static UInt8 ruleF (TokenList * tokL, ExprTree * exprT)
{	
	ExprNode * nodeP;
	UInt8 err = 0;

	err |= ruleX(tokL, exprT);
	if (!tokL->cellP || err)
		return err;

	switch (tokL->cellP->token)
	{
		case '^' :
			nodeP = NewExprNode(exprT->nodeP, NULL, 0, 0, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
			err |= ruleF(tokL, exprT);
			if (err)
				break;
			nodeP->rightP = exprT->nodeP;
			exprT->nodeP = nodeP;
		break;
	}

	return err;
}

// X -> '-' N | N
static UInt8 ruleX (TokenList * tokL, ExprTree * exprT)
{	
	ExprNode * nodeP , * node0;
	UInt8 err = 0;

	if (!tokL->cellP)
		return parseError;

	nodeP = node0 = NULL ;
	switch (tokL->cellP->token)
	{
		case '-' :
			// replace "- foo" by "(0 - foo)"
			tokL->cellP = tokL->cellP->nextP;
			node0 = NewExprNode(NULL, NULL, 0, tNumber, tNumber);
		break;
	}

	err |= ruleN(tokL, exprT);
	if (node0 && !err)
	{
		nodeP = NewExprNode(node0, exprT->nodeP, 0, 0, '-');
		exprT->nodeP = NewExprNode(nodeP, NULL, 0, 0, '(');
	}

	return err;
}

// N -> number | name | name '(' E ')' | '(' E ')'
static UInt8 ruleN (TokenList * tokL, ExprTree * exprT)
{	
	TokenCell * funcCell = NULL;
	UInt8 err = 0;

	if (!tokL->cellP)
		return parseError;

	switch (tokL->cellP->token)
	{
		case tNumber :
			exprT->nodeP = NewExprNode(NULL, NULL, tokL->cellP->data.value, tNumber, tNumber);
			tokL->cellP = tokL->cellP->nextP;
		break;

		case tName :
			if (tokL->cellP->dataType & (mConstant | mVariable))
			{
				exprT->nodeP = NewExprNode(NULL, NULL, tokL->cellP->data.value, tokL->cellP->dataType, tokL->cellP->token);
				tokL->cellP = tokL->cellP->nextP;
				break;
			}
			if (tokL->cellP->dataType & mFunction)
			{
				funcCell = tokL->cellP;
				tokL->cellP = tokL->cellP->nextP;
			}
			// add function dataType to the '(' exprNode
		case '(' :
			tokL->cellP = tokL->cellP->nextP;
			err |= ruleE(tokL, exprT);
			if (err)
				break;
			exprT->nodeP = NewExprNode(exprT->nodeP, NULL, 0, 0, '(');
			if (funcCell)
			{
				exprT->nodeP->data.funcRef = funcCell->data.funcRef;
				exprT->nodeP->dataType = funcCell->dataType;
			}
			if (tokL->cellP && tokL->cellP->token == ')')
				tokL->cellP = tokL->cellP->nextP;
			else
				err |= parseError;

		break;

		default :
			err |= parseError;
	}

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	NewExprNode 
 *
 * DESCRIPTION: Create a new ExprNode from default values
 *
 * PARAMETERS:  all ExprNode fields
 *
 * RETURNED:	pointer to a new ExprNode
 *
 ***********************************************************************/

static ExprNode * NewExprNode(ExprNode * leftP, ExprNode * rightP, double value, UInt8 dataType, UInt8 token)
{
	ExprNode * nodeP;
	nodeP = MemPtrNew(sizeof(ExprNode));
	nodeP->leftP = leftP;
	nodeP->rightP = rightP;
	nodeP->data.value = value;
	nodeP->dataType = dataType;
	nodeP->token = token;
	return nodeP;
}

/***********************************************************************
 *
 * FUNCTION:	NegRevertRightToLeft 
 *
 * DESCRIPTION: a - [b + c] => [a - b] + c
 *
 * PARAMETERS:  Expression node.
 *
 * RETURNED:	0
 *
 ***********************************************************************/

UInt8 NegRevertRightToLeft (ExprNode * nodeP)
{
	ExprNode * tmpNodeP;
	UInt8 tmpToken;

	if (!nodeP)
		return 0;

	if ((nodeP->token == '-' && (nodeP->rightP->token == '-' || nodeP->rightP->token == '+'))
	||  (nodeP->token == '/' && (nodeP->rightP->token == '/' || nodeP->rightP->token == '*')))
	{
		// swap tokens : a - [b + c] => a + [b - c]
		tmpToken = nodeP->token; nodeP->token = nodeP->rightP->token; nodeP->rightP->token = tmpToken;

		// swap nodes : a + [b - c] => [a - b] + c
		tmpNodeP = nodeP->rightP->leftP;		// a + [b - c] [b]	
		nodeP->rightP->leftP = nodeP->leftP;		// a + [a - c] [b]
		nodeP->leftP = nodeP->rightP->rightP;		// c + [a - c] [b]
		nodeP->rightP->rightP = tmpNodeP;		// c + [a - b] [b] 
		tmpNodeP = nodeP->leftP;			// c + [a - b] [c]
		nodeP->leftP = nodeP->rightP;			// [a - b] + [a - b] [c]
		nodeP->rightP = tmpNodeP;			// [a - b] + c

		NegRevertRightToLeft(nodeP);
	}
	else
	{
		NegRevertRightToLeft(nodeP->leftP);
		NegRevertRightToLeft(nodeP->rightP);
	}

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	MakeNegativeOperatorsLeftRecursive 
 *
 * DESCRIPTION: Since the LL(1) parser is right-recursive, expressions
 *		spelled "a - b + c" are evaluated as "a - [b + c]" but
 *		should be evaluated as "[a - b] + c". Reverse the tree for
 *		substractions and divisions.
 *
 * PARAMETERS:  expression tree.
 *
 * RETURNED:	0
 *
 ***********************************************************************/

UInt8 MakeNegativeOperatorsLeftRecursive (ExprTree * exprT)
{
	return NegRevertRightToLeft(exprT->rootP);
}


/***********************************************************************
 *
 * FUNCTION:	BuildExprTree 
 *
 * DESCRIPTION: Builds an expression tree from a token list. The token
 *		values must have been already assigned.
 *
 * PARAMETERS:  token list, expression tree.
 *
 * RETURNED:	0
 *
 ***********************************************************************/

UInt8 BuildExprTree (TokenList * tokL, ExprTree * exprT)
{
	UInt8 err = 0;

	err |= ruleE(tokL, exprT);
	exprT->rootP = exprT->nodeP;
	err |= MakeNegativeOperatorsLeftRecursive(exprT);
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	RecurseExprNode 
 *
 * DESCRIPTION: Evaluates an expression tree.
 *
 * PARAMETERS:  Expression node.
 *
 * RETURNED:	result
 *
 ***********************************************************************/

UInt8 RecurseExprNode (ExprNode * nodeP, double * resultP)
{
	double left, right;
	UInt8 err = 0;

	switch (nodeP->token)
	{
		case tNumber:
			* resultP = nodeP->data.value;
		break;

		case tName:
			if (nodeP->dataType & mValue)
				* resultP = nodeP->data.value;
			else
				err |= missingVarError;
		break;

		case '(':
			if (!(err |= RecurseExprNode(nodeP->leftP, resultP))
			&& nodeP->dataType & mFunction)
			{
				if (nodeP->dataType == tFunction)
					* resultP = nodeP->data.funcRef.func(* resultP);
				else
					err |= missingFuncError;
			}
		break;

		case '+':
			if (!((err |= RecurseExprNode(nodeP->leftP, &left)) || (err |= RecurseExprNode(nodeP->rightP, &right))))
				* resultP = left + right;
		break;

		case '-':
			if (!((err |= RecurseExprNode(nodeP->leftP, &left)) || (err |= RecurseExprNode(nodeP->rightP, &right))))
				* resultP = left - right;
		break;

		case '*':
			if (!((err |= RecurseExprNode(nodeP->leftP, &left)) || (err |= RecurseExprNode(nodeP->rightP, &right))))
				* resultP = left * right;
		break;

		case '/':
			if (!((err |= RecurseExprNode(nodeP->leftP, &left)) || (err |= RecurseExprNode(nodeP->rightP, &right))))
				* resultP = left / right;
		break;

		case '^':
			if (!MathLibRef)
				err |= missingFuncError;
			else if (!((err |= RecurseExprNode(nodeP->leftP, &left)) || (err |= RecurseExprNode(nodeP->rightP, &right))))
				* resultP = pow(left, right);
		break;
	}

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EvalExprTree 
 *
 * DESCRIPTION: Evaluates an expression tree.
 *
 * PARAMETERS:  Expression tree, result.
 *
 * RETURNED:	0 if no  error
 *
 ***********************************************************************/

UInt8 EvalExprTree (ExprTree * exprT, double * resultP)
{
	if (!exprT->rootP)
		return parseError;

	return RecurseExprNode(exprT->rootP, resultP);
}


/***********************************************************************
 *
 * FUNCTION:	DeleteNodes 
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  expression tree.
 *
 * RETURNED:
 *
 ***********************************************************************/

void DeleteNodes (ExprNode * nodeP)
{
	if (!nodeP)
		return;
	DeleteNodes(nodeP->leftP);
	DeleteNodes(nodeP->rightP);
	MemPtrFree(nodeP);
}


/***********************************************************************
 *
 * FUNCTION:	Eval
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Expression, variables assignations, result.
 *
 * RETURNED:	0 if no error
 *
 ***********************************************************************/

UInt8 Eval (Char * exprStr, Char * varsStr, double * resultP)
{
	TokenList tokL;
	VarList varL;
	ExprTree exprT;
	UInt8 err = 0;

	MemSet(&tokL, sizeof(TokenList), 0);
	MemSet(&varL, sizeof(VarList), 0);
	MemSet(&exprT, sizeof(ExprTree), 0);

	if (exprStr)
	{
		tokL.exprStr = MemPtrNew(1 + StrLen(exprStr));
		StrCopy(tokL.exprStr, exprStr);
	}
	if (varsStr)
	{
		varL.varsStr = MemPtrNew(1 + StrLen(varsStr));
		StrCopy(varL.varsStr, varsStr);
	}

	err |= ParseVariables(&varL);
	if (err)
		goto CleanUp;
	err |= TokenizeExpression(&tokL);
	if (err)
		goto CleanUp;
	err |= AssignTokenValue(&tokL, &varL);
	if (err)
		goto CleanUp;
	err |= BuildExprTree(&tokL, &exprT);
	if (err)
		goto CleanUp;
	err |= EvalExprTree(&exprT, resultP);


CleanUp:
	DeleteNodes(exprT.rootP);

	while (tokL.headP)
	{
		tokL.cellP = tokL.headP;
		tokL.headP = tokL.headP->nextP;
		MemPtrFree(tokL.cellP);
	}
	if (tokL.exprStr)
		MemPtrFree(tokL.exprStr);

	while (varL.headP)
	{
		varL.cellP = varL.headP;
		varL.headP = varL.headP->nextP;
		MemPtrFree(varL.cellP);
	}
	if (varL.varsStr)
		MemPtrFree(varL.varsStr);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	MakeVarsStringList
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  
 *
 * RETURNED:	0 if no error
 *
 ***********************************************************************/

UInt8 MakeVarsStringList (Char * varsStr, Char *** strTblP, Int16 * nStr)
{
	VarList varL;
	FlpCompDouble tmpF;
	Int16 i, len;
	UInt8 err = 0;

	* strTblP = NULL;
	* nStr = 0;

	MemSet(&varL, sizeof(VarList), 0);
	if (varsStr)
	{
		varL.varsStr = MemPtrNew(1 + StrLen(varsStr));
		StrCopy(varL.varsStr, varsStr);
	}

	err |= ParseVariables(&varL);
	if (err)
		goto CleanUp;

	varL.cellP = varL.headP;
	while (varL.cellP)
	{
		(* nStr)++;
		varL.cellP = varL.cellP->nextP;
	}

	* strTblP = MemPtrNew((* nStr) * sizeof(Char**));
	i = 0;
	varL.cellP = varL.headP;
	while (varL.cellP)
	{
		len = StrLen(varL.cellP->name);
		(* strTblP)[i] = MemPtrNew(len + 2 + kFlpBufSize);
		MemSet((* strTblP)[i], len + 2 + kFlpBufSize, 0);
		StrCopy((* strTblP)[i], varL.cellP->name);
		(* strTblP)[i][len] = '=';
		tmpF.d = varL.cellP->value;
		FlpCmpDblToA(&tmpF, (* strTblP)[i] + len + 1);
		varL.cellP = varL.cellP->nextP;
		+++i;
	}

CleanUp:
	while (varL.headP)
	{
		varL.cellP = varL.headP;
		varL.headP = varL.headP->nextP;
		MemPtrFree(varL.cellP);
	}
	if (varL.varsStr)
		MemPtrFree(varL.varsStr);
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	FlpCmpDblToA
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:
 *
 * RETURNED:	0 if no error
 *
 ***********************************************************************/

UInt8 FlpCmpDblToA(FlpCompDouble *f, Char *s)
{
	FlpDouble a;
	UInt32 mantissa;
	Int32 signedMantissa;
	Int16 i, exponent, sign;
	UInt8 err;

	if (f->d > 2000000000 || f->d < -2000000000)
		return (UInt8) FlpFToA(f->fd, s);

	if (err = (UInt8) FlpBase10Info(f->fd, &mantissa, &exponent, &sign))
		return err;

// from FlpBase10Info reference : 
// The mantissa is normalized so it contains at least 8 significant digits when printed as an integer value.
	if (exponent < -8 || exponent > 2)
		return (UInt8) FlpFToA(f->fd, s);

	signedMantissa = mantissa * (sign ? -1 : 1);

	s ++;
	s = StrIToA(s, signedMantissa);
	s --;
	if (s[1]=='-')
	{
		s[0] = '-';
		s[1] = '0';
	}
	else
		s[0] = ' ';

	if (!signedMantissa || !exponent)
		return (UInt8) 0;

	i = StrLen(s);

	if (exponent < 0)
	{
		s[i+1] = nullChr;
		while (exponent)
		{
		 	s[i] = s[i-1];
		 	i--;
		 	exponent++;
		}
		s[i] = '.';
	}
	if (exponent > 0)
	{
		while (exponent)
		{
		 	s[i] = '0';
		 	i++;
		 	exponent--;
		}
		s[i] = nullChr;
	}

	if (s[1] == '.')
		s[0] = '0';

	return 0;
}
