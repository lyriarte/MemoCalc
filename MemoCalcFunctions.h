
/***********************************************************************
 *
 * FILE : MemoCalcFunctions.h
 * 
 * DESCRIPTION : MemoCalc function names and MathLib calls
 *
 * COPYRIGHT : (C) 2003 Luc Yriarte
 * 
 *
 ***********************************************************************/

#ifndef MEMOCALCFUNCTIONS_H
#define MEMOCALCFUNCTIONS_H

// types and structures

typedef double FuncType (double x);

typedef struct FuncRef {
	Char * name;
	FuncType * func;
} FuncRef ;


// functions

UInt8 GetConst (double * valueP, Char * constName, UInt16 len);
UInt8 GetFunc (FuncRef * funcRefP, Char * funcName, UInt16 len);
UInt8 GetFuncsStringList (Char *** strTblP, Int16 * nStr);

#endif // MEMOCALCFUNCTIONS_H

