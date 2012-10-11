
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
#include <stdlib.h>
#include <string.h>

typedef union { double d; double fd; } FlpCompDouble;
#define UInt32 unsigned int
#define UInt16 unsigned short
#define UInt8 unsigned char
#define Char char

#define nullChr 0

#define MemPtrNew malloc
#define MemPtrFree free
#define MemSet memset
#define FlpBufferAToF(f, a) *(f) = atof(a)
#define StrNCompare strncmp
#define StrCopy strcpy
#define StrLen strlen

#else

#include <PalmOS.h>
#include <FloatMgr.h>

#endif

#include "MemoCalcLexer.h"
#include "MemoCalcParser.h"

/***********************************************************************
 *
 *	Private definitions
 *
 ***********************************************************************/

// tokens

#define parseError	0xff

// structures

typedef struct ExprNode {
	struct ExprNode * leftP;
	struct ExprNode * rightP;
	double value;
	UInt8 token;
} ExprNode;

typedef struct ExprTree {
	ExprNode * rootP;
	ExprNode * nodeP;
} ExprTree;

// functions
static ExprNode * NewExprNode(ExprNode * leftP, ExprNode * rightP, double value, UInt8 token);


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

	err = ruleT(tokL, exprT);
	if (!tokL->cellP || err)
		return err;

	switch (tokL->cellP->token)
	{
		case '+' :
		case '-' :
			nodeP = NewExprNode(exprT->nodeP, NULL, 0, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
			err = ruleE(tokL, exprT);
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

	err = ruleF(tokL, exprT);
	if (!tokL->cellP || err)
		return err;

	switch (tokL->cellP->token)
	{
		case '*' :
		case '/' :
			nodeP = NewExprNode(exprT->nodeP, NULL, 0, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
			err = ruleT(tokL, exprT);
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

	err = ruleX(tokL, exprT);
	if (!tokL->cellP || err)
		return err;

	switch (tokL->cellP->token)
	{
		case '^' :
			nodeP = NewExprNode(exprT->nodeP, NULL, 0, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
			err = ruleF(tokL, exprT);
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
			node0 = NewExprNode(NULL, NULL, 0, tInteger);
		break;
	}

	err = ruleN(tokL, exprT);
	if (node0 && !err)
	{
		nodeP = NewExprNode(node0, exprT->nodeP, 0, '-');
		exprT->nodeP = NewExprNode(nodeP, NULL, 0, '(');
	}

	return err;
}

// N -> number | string | string '(' E ')' | '(' E ')'
static UInt8 ruleN (TokenList * tokL, ExprTree * exprT)
{	
	UInt8 err = 0;

	if (!tokL->cellP)
		return parseError;

	switch (tokL->cellP->token)
	{
		case tInteger :
		case tFloat :
			exprT->nodeP = NewExprNode(NULL, NULL, tokL->cellP->data.value, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
		break;

		case tName :
			exprT->nodeP = NewExprNode(NULL, NULL, tokL->cellP->data.value, tokL->cellP->token);
			tokL->cellP = tokL->cellP->nextP;
			if (!tokL->cellP || tokL->cellP->token != '(')
				break;
			// #### handle function call here

		case '(' :
			tokL->cellP = tokL->cellP->nextP;
			err = ruleE(tokL, exprT);
			if (err)
				break;
			exprT->nodeP = NewExprNode(exprT->nodeP, NULL, 0, '(');
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


/***********************************************************************
 *
 * FUNCTION:    NewExprNode 
 *
 * DESCRIPTION: Create a new ExprNode from default values
 *
 * PARAMETERS:  all ExprNode fields
 *
 * RETURNED:	pointer to a new ExprNode
 *
 ***********************************************************************/

static ExprNode * NewExprNode(ExprNode * leftP, ExprNode * rightP, double value, UInt8 token)
{
	ExprNode * nodeP;
	nodeP = MemPtrNew(sizeof(ExprNode));
	nodeP->leftP = leftP;
	nodeP->rightP = rightP;
	nodeP->value = value;
	nodeP->token = token;
	return nodeP;
}

/***********************************************************************
 *
 * FUNCTION:    NegRevertRightToLeft 
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

	NegRevertRightToLeft(nodeP->leftP);
	NegRevertRightToLeft(nodeP->rightP);

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
	}

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    MakeNegativeOperatorsLeftRecursive 
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
 * FUNCTION:    BuildExprTree 
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
	UInt8 err;

	err = ruleE(tokL, exprT);
	exprT->rootP = exprT->nodeP;
	if (err)
		return err;

	exprT->rootP = exprT->nodeP;
	err = MakeNegativeOperatorsLeftRecursive(exprT);
	return err;
}


/***********************************************************************
 *
 * FUNCTION:    RecurseExprNode 
 *
 * DESCRIPTION: Evaluates an expression tree.
 *
 * PARAMETERS:  Expression node.
 *
 * RETURNED:	result
 *
 ***********************************************************************/

double RecurseExprNode (ExprNode * nodeP)
{
	switch (nodeP->token)
	{
		case tInteger:
		case tFloat:
		case tName:
			return nodeP->value;
		break;

		case '(':
			return RecurseExprNode(nodeP->leftP);

		case '+':
			return RecurseExprNode(nodeP->leftP) + RecurseExprNode(nodeP->rightP);

		case '-':
			return RecurseExprNode(nodeP->leftP) - RecurseExprNode(nodeP->rightP);

		case '*':
			return RecurseExprNode(nodeP->leftP) * RecurseExprNode(nodeP->rightP);

		case '/':
			return RecurseExprNode(nodeP->leftP) / RecurseExprNode(nodeP->rightP);
	}
	
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    EvalExprTree 
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

	*resultP = RecurseExprNode(exprT->rootP);
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:    DeleteNodes 
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
 * FUNCTION:    Eval
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

	err = ParseVariables(&varL);
	if (err)
		goto CleanUp;
	err = TokenizeExpression(&tokL);
	if (err)
		goto CleanUp;
	err = AssignTokenValue(&tokL, &varL);
	if (err)
		goto CleanUp;
	err = BuildExprTree(&tokL, &exprT);
	if (err)
		goto CleanUp;
	err = EvalExprTree(&exprT, resultP);


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



#if defined(__INTEL__) || defined(__i386__) || defined(WIN32)

void main()
{
	TokenList tokL = { NULL, NULL, NULL } ;
	VarList varL = { NULL, NULL, NULL} ;
	ExprTree exprT = {NULL, NULL} ;
	UInt8 err;
	double result;

	tokL.exprStr = strdup( "toto * 40 - pipo - 50");
	varL.varsStr = strdup("toto = 6.25 pipo = 100");

	err = ParseVariables(&varL);
	err = TokenizeExpression(&tokL);
	err = AssignTokenValue(&tokL, &varL);
	err = BuildExprTree(&tokL, &exprT);
	err = EvalExprTree(&exprT, &result);
}

#endif
