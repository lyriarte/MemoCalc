
/***********************************************************************
 *
 * FILE : MemoCalcParser.h
 * 
 * DESCRIPTION : Grammatical analyser headers for MemoCalc
 * 
 * COPYRIGHT : (C) 2003 Luc Yriarte
 * 
 *
 ***********************************************************************/

#ifndef MEMOCALCPARSER_H
#define MEMOCALCPARSER_H

// functions

UInt8 Eval (Char * exprStr, Char * varsStr, double * resultP);
UInt8 MakeVarsStringList (Char * varsStr, Char *** strTblP, Int16 * nStr);

UInt8 FlpCmpDblToA(FlpCompDouble *f, Char *s);

#endif // MEMOCALCPARSER_H
