
/***********************************************************************
 *
 * FILE : MemoCalcFunctions.c
 * 
 * DESCRIPTION : MemoCalc function names and MathLib calls
 *
 * COPYRIGHT : GNU GENERAL PUBLIC LICENSE
 * http://www.gnu.org/licenses/gpl.txt
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
 * FUNCTION:    GetFunc
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

