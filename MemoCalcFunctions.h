
/***********************************************************************
 *
 * FILE : MemoCalcFunctions.h
 * 
 * DESCRIPTION : MemoCalc function names and MathLib calls
 *
 * COPYRIGHT : GNU GENERAL PUBLIC LICENSE
 * http://www.gnu.org/licenses/gpl.txt
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

UInt8 GetFunc (FuncRef * funcRefP, Char * funcName, UInt16 len);

#endif // MEMOCALCFUNCTIONS_H

