
/***********************************************************************
 *
 * FILE : MemoCalcFunctions.c
 * 
 * DESCRIPTION : MemoCalc function names and MathLib calls
 *
 * COPYRIGHT : (C) 2003 Luc Yriarte
 * 
 *
 ***********************************************************************/

#include <PalmOS.h>

#include "MathLib.h"
#include "MemoCalcFunctions.h"

extern UInt16 MathLibRef;

/***********************************************************************
 *
 *	Function names and references at the same indices
 *
 ***********************************************************************/

static Char * funcNames[] = {
/***************************
 * Trigonometric functions *
 ***************************/
"acos",				// Arc cosine of x
"asin",				// Arc sine of x
"atan",				// Arc tangent of x
"cos",				// Cosine of x
"sin",				// Sine of x
"tan",				// Tangent of x

/************************
 * Hyperbolic functions	*
 ************************/
"cosh",				// Hyperbolic cosine of x
"sinh",				// Hyperbolic sine of x
"tanh",				// Hyperbolic tangent of x
"acosh",				// Hyperbolic arc cosine of x
"asinh"	,			// Hyperbolic arc sine of x
"atanh",				// Hyperbolic arc tangent of x

/*****************************************
 * Exponential and logarithmic functions *
 *****************************************/
"exp",					// Exponential function of x [pow(e,x)]
"log",					// Natural logarithm of x
"log10",					// Base 10 logarithm of x
"log2",					// Base 2 logarithm of x
NULL
};

static FuncType * funcRefs[] = {
/***************************
 * Trigonometric functions *
 ***************************/
&acos,				// Arc cosine of x
&asin,				// Arc sine of x
&atan,				// Arc tangent of x
&cos,				// Cosine of x
&sin,				// Sine of x
&tan,				// Tangent of x

/************************
 * Hyperbolic functions	*
 ************************/
&cosh,				// Hyperbolic cosine of x
&sinh,				// Hyperbolic sine of x
&tanh,				// Hyperbolic tangent of x
&acosh,				// Hyperbolic arc cosine of x
&asinh,			// Hyperbolic arc sine of x
&atanh,				// Hyperbolic arc tangent of x

/*****************************************
 * Exponential and logarithmic functions *
 *****************************************/
&exp,					// Exponential function of x [pow(e,x)]
&log,					// Natural logarithm of x
&log10,					// Base 10 logarithm of x
&log2					// Base 2 logarithm of x
};


/***********************************************************************
 *
 *	Constants names and values at the same indices
 *
 ***********************************************************************/

static Char * constNames[] = {
"e",
"pi",
"g",
"c",
NULL
};

static double constValues[] = {
2.718281828,
3.141592654,
9.80665,
299792458
};


/***********************************************************************
 *
 * FUNCTION:	GetConst
 *
 * DESCRIPTION: Returns a function pointer from the name
 *
 * PARAMETERS:  a funcRef struct (I name / O func)
 *
 * RETURNED:	0 if found
 *
 ***********************************************************************/

UInt8 GetConst (double * valueP, Char * constName, UInt16 len)
{
	UInt16 i=0;

	while (constNames[i])
	{
		if (StrNCompare(constName, constNames[i], len) == 0)
		{
			* valueP = constValues[i];
			return 0;
		}
		++i;
	}

	return 1;
}


/***********************************************************************
 *
 * FUNCTION:	GetFunc
 *
 * DESCRIPTION: Returns a function pointer from the name
 *
 * PARAMETERS:  a funcRef struct (I name / O func)
 *
 * RETURNED:	0 if found
 *
 ***********************************************************************/

UInt8 GetFunc (FuncRef * funcRefP, Char * funcName, UInt16 len)
{
	UInt16 i=0;

	if (!MathLibRef)
		 return 1;

	while (funcNames[i])
	{
		if (StrNCompare(funcName, funcNames[i], len) == 0)
		{
			funcRefP->name = funcNames[i];
			funcRefP->func = funcRefs[i];
			return 0;
		}
		++i;
	}

	return 1;
}


/***********************************************************************
 *
 * FUNCTION:	GetFuncsStringList
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  
 *
 * RETURNED:	0 if no error
 *
 ***********************************************************************/

UInt8 GetFuncsStringList (Char *** strTblP, Int16 * nStr)
{
	*nStr = 0;
	*strTblP = NULL;

	if (!MathLibRef)
		 return 1;

	while (funcNames[*nStr])
	{
		(*nStr)++;
	}

	*strTblP = funcNames;
	return 0;
}
