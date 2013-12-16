
/***********************************************************************
 *
 * FILE : MemoCalc.c
 * 
 * DESCRIPTION : PalmOS UI and DB related funcions for MemoCalc
 *
 * COPYRIGHT : (C) 2003 Luc Yriarte
 * 
 *
 ***********************************************************************/



#include <PalmOS.h>
#include <FloatMgr.h>
#include <TraceMgr.h>

#include "MathLib.h"
#include "MemoCalc.h"
#include "MemoCalcFunctions.h"
#include "MemoCalcLexer.h"
#include "MemoCalcParser.h"


/***********************************************************************
 *
 *	Defines, Macros and Constants
 *
 ***********************************************************************/

#define sysFileCMemoCalc			'MeCa'
#define memoDBType					'DATA'
#define memoCalcDefaultCategoryName	"MemoCalc"
#define memoCalcCurrRecFtrNum		0

#define kEditFormTitle				"Expression editor"
#define kVarsEditLabel				"Vars"
#define kVarsListLabel				"List"
#define kErrorStr					"Error"
#define kExprTag					"<--expr-->"
#define kVarsTag					"<--vars-->"
#define kExprTagLen					10
#define kVarsTagLen					10

#define editorSaveMemo				0
#define editorDeleteMemo			1
#define editorDiscardMemo			2
#define editorSaveEmptyMemo			3

#define resultBaseDecimal			0
#define resultBaseHexadecimal		1

#define kMemoHeaderSize				20
#define kMemoHeaderStr				"MemoCalc XXXXXX\n"
#define kMemoHeaderFormat			"MemoCalc %06d\n"

/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/

/* **** **** MathLib **** **** */
extern UInt16 MathLibRef;

/* **** **** Application  **** */
static EventType sAppEvent;

/* **** **** Memo DB **** **** */
static DmOpenRef sMemoDB;
static UInt16 sMemoCalcCategory;

/* **** **** List View ** **** */
static UInt16 sCurrentRecIndex, sTopVisibleRecIndex, sSavedRecIndex;

/* **** **** Edit View ** **** */
static MemHandle sMemoH;
static Char ** sVarsStrTbl;
static Char * sEditViewTitleStr;
static Int16 sNVars;
static UInt16 sEditFieldFocus;
static UInt8 sEditorSavePolicy;
static UInt8 sEditorResultBase;
static Boolean sVarsOk, sVarsList;
static Char sMemoHeaderBuf[kMemoHeaderSize];

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/


/***********************************************************************
 *
 * FUNCTION:	MemoCalcMathLibOpen
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  
 *
 * RETURNED:	
 *
 ***********************************************************************/

static UInt16 MemoCalcMathLibOpen (void)
{
	UInt16 err = errNone;

	err = SysLibLoad(sysFileTLibrary, MathLibCreator, &MathLibRef);
	if (err)
	{
		MathLibRef = NULL;
		goto Exit;
	}
	err = MathLibOpen(MathLibRef, MathLibVersion);
	ErrFatalDisplayIf(err, "Can't open MathLib");
Exit:
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	MemoCalcMathLibClose
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  
 *
 * RETURNED:	
 *
 ***********************************************************************/

static UInt16 MemoCalcMathLibClose (void)
{
	UInt16 usecount;
	UInt16 err = errNone;

	if (!MathLibRef)
		goto Exit;
	err = MathLibClose(MathLibRef, &usecount);
	ErrFatalDisplayIf(err, "Can't close MathLib");
	if (usecount == 0)
		SysLibRemove(MathLibRef); 
Exit:
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	MemoCalcDBOpen
 *
 * DESCRIPTION: Open the default Memo database in rw mode
 *		Check if category memoCalcDefaultCategoryName exists, otherwise
 *		use dmAllCategories
 *
 * PARAMETERS:  DmOpenRef *dbPP
 *
 * RETURNED:	error code
 *
 ***********************************************************************/

static UInt16 MemoCalcDBOpen (DmOpenRef *dbPP, UInt16 *memoCalcCategoryP)
{
	UInt16 err = errNone;
	*dbPP = DmOpenDatabaseByTypeCreator(memoDBType, sysFileCMemo, dmModeReadWrite);
	if (*dbPP == NULL)
	{
		err = dmErrCantOpen;
		goto Exit;
	}

	*memoCalcCategoryP = CategoryFind (*dbPP, memoCalcDefaultCategoryName);

Exit:
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	MemoCalcDBClose
 *
 * DESCRIPTION: Close the default Memo database
 *
 * PARAMETERS:  none
 *
 * RETURNED:	error code
 *
 ***********************************************************************/

static UInt16 MemoCalcDBClose (DmOpenRef *dbPP)
{
	UInt16 err;
	err = DmCloseDatabase(*dbPP);
	*dbPP = NULL;
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	StartApplication
 *
 * DESCRIPTION: Default initialization for the MemoCalc application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:	error code
 *
 ***********************************************************************/

static UInt16 StartApplication (void)
{
	UInt32 ftr = 0;
	UInt16 err = 0;

// Initializing globals
	// MathLib
	MathLibRef = NULL;
	// Application
	MemSet(&sAppEvent, sizeof(EventType), 0);
	// Memo DB
	sMemoDB = NULL;
	sMemoCalcCategory = dmAllCategories;

// Run init code
	err = MemoCalcMathLibOpen();
	err = MemoCalcDBOpen(&sMemoDB, &sMemoCalcCategory);
	if (err)
		goto Exit;

	// List View
	if (FtrGet(sysFileCMemoCalc, memoCalcCurrRecFtrNum, &ftr)
	|| DmFindRecordByID(sMemoDB, ftr, &sCurrentRecIndex))
		sCurrentRecIndex = dmMaxRecordIndex;
	sSavedRecIndex = sCurrentRecIndex;
	sTopVisibleRecIndex = 0;

Exit:
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	StopApplication
 *
 * DESCRIPTION: Default cleanup code for the MemoCalc application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:	error code
 *
 ***********************************************************************/

static UInt16 StopApplication (void)
{
	UInt16 err;
	FrmCloseAllForms();
	err = MemoCalcDBClose(&sMemoDB);
	MemoCalcMathLibClose();
	return err;
}


/***********************************************************************
 *
 * FUNCTION:	MemoCalcUIUpdateScrollBar
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void MemoCalcUIUpdateScrollBar (FormPtr frmP, UInt16 fieldID, UInt16 barID)
{
	UInt16 scrollPos, textHeight, fieldHeight, maxValue;
	FieldPtr fldP;
	ScrollBarPtr barP;

	fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, fieldID));
	barP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, barID));

	FldGetScrollValues (fldP, &scrollPos, &textHeight, &fieldHeight);

	if (textHeight > fieldHeight)
		maxValue = (textHeight - fieldHeight) + FldGetNumberOfBlankLines(fldP);
	else if (scrollPos)
		maxValue = scrollPos;
	else
		maxValue = 0;

	SclSetScrollBar(barP, (Int16)scrollPos, 0, (Int16)maxValue, fieldHeight-1);
}


/***********************************************************************
 *
 * FUNCTION:	MemoCalcUIScroll
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void MemoCalcUIScroll (FormPtr frmP, Int16 linesToScroll, UInt16 fieldID, UInt16 barID)
{
	FieldPtr fldP;

	fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, fieldID));

	if (linesToScroll < 0)
		FldScrollField (fldP, (UInt16)(-linesToScroll), winUp);
	else if (linesToScroll > 0)
		FldScrollField (fldP, linesToScroll, winDown);

	MemoCalcUIUpdateScrollBar(frmP, fieldID, barID);
}


/***********************************************************************
 *
 * FUNCTION:	EditViewToggleExprButtons
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void EditViewToggleExprButtons (FormPtr frmP)
{

	sEditFieldFocus = FrmGetObjectIndex(frmP, ExprField);
	if (FrmGetFocus(frmP) == sEditFieldFocus)
	{
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bpls));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, Btim));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bdiv));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bopn));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bcls));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bclr));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bequ));
		if (MathLibRef)
		{
			FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bexp));
			FrmShowObject(frmP, FrmGetObjectIndex(frmP, FunctionsTrigger));
		}
	}
	else
	{
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bpls));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Btim));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bdiv));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bopn));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bcls));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bclr));
		if (MathLibRef)
		{
			FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bexp));
			FrmHideObject(frmP, FrmGetObjectIndex(frmP, FunctionsTrigger));
		}
		if (FrmGetFocus(frmP) == FrmGetObjectIndex(frmP, VarsField))
		{
			sEditFieldFocus = FrmGetObjectIndex(frmP, VarsField);
			FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bequ));
		}
	}

	FrmUpdateForm(EditView, frmRedrawUpdateCode);
}


/***********************************************************************
 *
 * FUNCTION:	EditViewToggleVarsView
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void EditViewToggleVarsView (FormPtr frmP)
{
	ControlPtr varsCtlP;
	ListPtr varsLstP;
	FieldPtr varsFldP;
	Char * varsStr;
	Int16 iVar, iChar;
	UInt16 valStart, valEnd;

	if (sVarsStrTbl)
	{
		while(sNVars)
			MemPtrFree(sVarsStrTbl[--sNVars]);
		MemPtrFree(sVarsStrTbl);
		sVarsStrTbl = NULL;
	}

	varsCtlP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsButton));
	varsFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
	varsStr = FldGetTextPtr(varsFldP);
	sVarsOk = (MakeVarsStringList(varsStr, &sVarsStrTbl, &sNVars) == 0);
	sVarsList = sVarsList && sVarsOk && sNVars;

	varsLstP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsList));

	if (sVarsList)
	{
		if (sVarsStrTbl)
			LstSetListChoices(varsLstP, sVarsStrTbl, sNVars);

		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bequ));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, VarsField));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, VarsScrollBar));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, VarsLabel));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, VarsList));

		FrmSetFocus(frmP, (sEditFieldFocus = FrmGetObjectIndex(frmP, ExprField)));
		CtlSetLabel(varsCtlP, kVarsEditLabel);
	}
	else
	{
		valStart = valEnd = 0;
		iVar = LstGetSelection(varsLstP);
		if (sVarsOk && varsStr && iVar != noListSelection)
		{	// select variable value in the field
			iChar = 0;
			while (varsStr[iChar])
			{
				if (varsStr[iChar] == '=')
				{
					iChar++;
					if (iVar)
						iVar--;
					else
					{
						while (isSeparator(varsStr[iChar]))
							iChar++;
						valStart = iChar;
						while (isNumber(varsStr[iChar]) || varsStr[iChar] == '.' || varsStr[iChar] == '-')
							iChar++;
						valEnd = iChar;
						break;
					}
				}
				iChar++;
			}
		}

		FrmHideObject(frmP, FrmGetObjectIndex(frmP, VarsList));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, Bequ));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, VarsField));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, VarsScrollBar));
		FrmShowObject(frmP, FrmGetObjectIndex(frmP, VarsLabel));

		CtlSetLabel(varsCtlP, kVarsListLabel);

		if (FrmGetFocus(frmP) != FrmGetObjectIndex(frmP, ExprField))
			FrmSetFocus(frmP, (sEditFieldFocus = FrmGetObjectIndex(frmP, VarsField)));

		if (valStart < valEnd)
			FldSetSelection(varsFldP, valStart, valEnd);

		MemoCalcUIUpdateScrollBar(frmP, VarsField, VarsScrollBar);
	}

	EditViewToggleExprButtons(frmP);
}


/***********************************************************************
 *
 * FUNCTION:	EditViewEval
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void EditViewEval (FormPtr frmP)
{
	static Char resultBuf[kFlpBufSize];
	FieldPtr exprFldP, varsFldP, resultFldP;
	Char * exprStr, * varsStr;
	FlpCompDouble result;
	UInt8 err = 0;

	exprFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
	varsFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
	resultFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ResultField));

	exprStr = FldGetTextPtr(exprFldP);
	varsStr = FldGetTextPtr(varsFldP);

	if (exprStr == NULL || *exprStr == '\0')
		result.d = 0;
	else
		err = Eval(exprStr, varsStr, &(result.d));
	sVarsOk = !(err & missingVarError);

	if (err)
		StrCopy(resultBuf, kErrorStr);
	else {
		switch (sEditorResultBase) {
			case resultBaseDecimal:
				FlpCmpDblToA(&result, resultBuf);
			break;
			case resultBaseHexadecimal:
				StrPrintF(resultBuf,"0x%08lx",(long)result.d);
			break;
			default:
				StrCopy(resultBuf, kErrorStr);
		}
	}

	FldSetTextPtr(resultFldP, resultBuf);
	FrmUpdateForm(EditView, frmRedrawUpdateCode);
}


/***********************************************************************
 *
 * FUNCTION:	EditViewSave
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void EditViewSave (FormPtr frmP)
{
	MemHandle exprH, varsH;
	Char * memoStr, * exprStr, * varsStr, * tmpStr;
	FieldPtr exprFldP, varsFldP;
	UInt32 ftr;
	UInt16 memoLen, exprLen, varsLen, titleLen, attr;
	Boolean defaultTitle;

	memoLen = exprLen = varsLen = titleLen = 0;
	memoStr = exprStr = varsStr = tmpStr = NULL;
	defaultTitle = 0;

	if (sEditViewTitleStr)
	{
		FrmSetTitle(frmP, kEditFormTitle);
		MemPtrFree(sEditViewTitleStr);
		sEditViewTitleStr = NULL;
	}

	if (sMemoH)
	{
		memoStr = (Char*) MemHandleLock(sMemoH);
		titleLen = memoLen = StrLen(memoStr);
		tmpStr = StrStr(memoStr, kVarsTag);
		if (tmpStr)
			titleLen = (UInt32)tmpStr - (UInt32)memoStr;
		else
		{
			tmpStr = StrStr(memoStr, kExprTag);
			if (tmpStr)
				titleLen = (UInt32)tmpStr - (UInt32)memoStr;
		}
	}

	exprFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
	exprH = FldGetTextHandle(exprFldP);
	if (exprH)
	{
		exprStr = (Char*) MemHandleLock(exprH);
		exprLen = StrLen(exprStr);
	}

	varsFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
	varsH = FldGetTextHandle(varsFldP);
	if (varsH)
	{
		varsStr = (Char*) MemHandleLock(varsH);
		varsLen = StrLen(varsStr);
	}

	memoLen = titleLen + kVarsTagLen + varsLen + kExprTagLen + exprLen;

	if (sEditorSavePolicy == editorDiscardMemo)
		goto Cleanup;

	if (sCurrentRecIndex != dmMaxRecordIndex)
	{
		if (sMemoH)
			MemHandleUnlock(sMemoH);
		DmReleaseRecord(sMemoDB, sCurrentRecIndex, true);
		DmResizeRecord(sMemoDB, sCurrentRecIndex, memoLen + 1 );
		sMemoH = DmGetRecord(sMemoDB, sCurrentRecIndex);
	}
	else
	{
		if (titleLen == 0) {
			memoLen += titleLen = StrLen(kMemoHeaderStr);
			sMemoH = DmNewRecord(sMemoDB, &sCurrentRecIndex, memoLen + 1 );
			memoStr = (Char*) MemHandleLock(sMemoH);
			StrPrintF(sMemoHeaderBuf,kMemoHeaderFormat, sCurrentRecIndex);
			DmWrite(memoStr, 0, sMemoHeaderBuf, titleLen);
			MemHandleUnlock(sMemoH);
			defaultTitle = 1;
		}
		else
			sMemoH = DmNewRecord(sMemoDB, &sCurrentRecIndex, memoLen + 1 );
		if (sMemoCalcCategory != dmAllCategories)
		{
			DmRecordInfo(sMemoDB, sCurrentRecIndex, &attr, NULL, NULL);
			attr = attr & ~dmRecAttrCategoryMask;
			attr = attr | sMemoCalcCategory;
			DmSetRecordInfo(sMemoDB, sCurrentRecIndex, &attr, NULL);
		}
	}

	memoStr = (Char*) MemHandleLock(sMemoH);

	DmWrite(memoStr, titleLen, kVarsTag, kVarsTagLen);
	if (varsLen)
	{
		DmWrite(memoStr, titleLen + kVarsTagLen, varsStr, varsLen);
	}

	DmWrite(memoStr, titleLen + kVarsTagLen + varsLen, kExprTag, kExprTagLen);
	if (exprLen)
	{
		DmWrite(memoStr, titleLen + kVarsTagLen + varsLen + kExprTagLen, exprStr, exprLen);
	}

	DmSet(memoStr, memoLen, 1, 0);

Cleanup:
	if (exprH)
		MemHandleUnlock(exprH);
	if (varsH)
		MemHandleUnlock(varsH);
	if (sMemoH)
	{
		MemHandleUnlock(sMemoH);
		sMemoH = NULL;
	}
	if (sCurrentRecIndex == dmMaxRecordIndex
	||	DmRecordInfo(sMemoDB, sCurrentRecIndex, NULL, &ftr, NULL))
		ftr = (UInt32)(sCurrentRecIndex = dmMaxRecordIndex);
	FtrSet(sysFileCMemoCalc, memoCalcCurrRecFtrNum, ftr);
	if (sCurrentRecIndex != dmMaxRecordIndex)
	{
		DmReleaseRecord(sMemoDB, sCurrentRecIndex, true);
		if (sEditorSavePolicy == editorDeleteMemo 
		|| (sEditorSavePolicy == editorSaveMemo && (exprLen + varsLen == 0) && (titleLen == 0 || defaultTitle)))
		{
			DmDeleteRecord(sMemoDB, sCurrentRecIndex);
			DmMoveRecord(sMemoDB, sCurrentRecIndex, DmNumRecords(sMemoDB));
			FtrSet(sysFileCMemoCalc, memoCalcCurrRecFtrNum, (UInt32)dmMaxRecordIndex);
			sSavedRecIndex = dmMaxRecordIndex;
		}
		else
			sSavedRecIndex = sCurrentRecIndex;
		sCurrentRecIndex = dmMaxRecordIndex;
	}
	if (sVarsStrTbl)
	{
		while(sNVars)
			MemPtrFree(sVarsStrTbl[--sNVars]);
		MemPtrFree(sVarsStrTbl);
		sVarsStrTbl = NULL;
	}
}


/***********************************************************************
 *
 * FUNCTION:	EditViewInit
 *
 * DESCRIPTION: Initialize edit view form, load current record
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void EditViewInit (FormPtr frmP)
{
	ListPtr funcsLstP;
	MemHandle exprH, varsH;
	Char * memoStr, * exprStr, *varsStr, * tmpStr, ** funcsStrTbl;
	FieldPtr exprFldP, varsFldP;
	UInt16 titleLen = 0;
	Int16 nFuncs = 0;
	UInt8 err = 0;

	FrmSetTitle(frmP, kEditFormTitle);

	if (!MathLibRef)
	{
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, Bexp));
		FrmHideObject(frmP, FrmGetObjectIndex(frmP, FunctionsTrigger));
	}
	else
	{
		funcsLstP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, FunctionsList));
		GetFuncsStringList(&funcsStrTbl, &nFuncs);
		LstSetListChoices(funcsLstP, funcsStrTbl, nFuncs);
	}

	sMemoH = NULL;
	sVarsStrTbl = NULL;
	sEditViewTitleStr = NULL;
	sNVars = 0;
	sEditorSavePolicy = editorSaveMemo;
	sEditorResultBase = resultBaseDecimal;
	sVarsList = sVarsOk = true;

	if (sCurrentRecIndex != dmMaxRecordIndex)
	{	
		sMemoH = DmGetRecord(sMemoDB, sCurrentRecIndex);
		memoStr = (Char*) MemHandleLock(sMemoH);
		titleLen = StrLen(memoStr);
		exprStr = StrStr(memoStr, kExprTag);
		if (exprStr)
		{
			exprStr += kExprTagLen;
			exprH = MemHandleNew(1 + (UInt32)titleLen -((UInt32)exprStr - (UInt32)memoStr));
			tmpStr = MemHandleLock(exprH);
			StrNCopy(tmpStr, exprStr, (UInt32)titleLen -((UInt32)exprStr - (UInt32)memoStr));
			tmpStr[(UInt16)((UInt32)titleLen -((UInt32)exprStr - (UInt32)memoStr))] = nullChr;
			MemHandleUnlock(exprH);
			exprFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
			FldSetTextHandle(exprFldP, exprH);
			titleLen = (UInt16) ((UInt32)exprStr - kExprTagLen - (UInt32)memoStr);
		}
		varsStr = StrStr(memoStr, kVarsTag);
		if (varsStr && exprStr && varsStr < exprStr)
		{
			varsStr += kVarsTagLen;
			varsH = MemHandleNew(1 + (UInt32)exprStr - kExprTagLen - (UInt32)varsStr);
			tmpStr = MemHandleLock(varsH);
			StrNCopy(tmpStr, varsStr, (UInt32)exprStr - kExprTagLen - (UInt32)varsStr);
			tmpStr[(UInt16)((UInt32)exprStr - kExprTagLen - (UInt32)varsStr)] = nullChr;
			MemHandleUnlock(varsH);
			varsFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
			FldSetTextHandle(varsFldP, varsH);
			titleLen = (UInt16) ((UInt32)varsStr - kVarsTagLen - (UInt32)memoStr);
		}
		if (titleLen)
		{
			tmpStr = StrStr(memoStr, "\n");
			if (tmpStr && (UInt32)tmpStr < ((UInt32)memoStr + titleLen))
				titleLen = (UInt16) ((UInt32)tmpStr - (UInt32)memoStr);
			sEditViewTitleStr = MemPtrNew(1 + titleLen);
			StrNCopy(sEditViewTitleStr, memoStr, titleLen);
			sEditViewTitleStr[titleLen] = nullChr;
			FrmSetTitle(frmP, sEditViewTitleStr);
		}
		MemHandleUnlock(sMemoH);
	}

	MemoCalcUIUpdateScrollBar(frmP, ExprField, ExprScrollBar);
	FrmSetFocus(frmP, (sEditFieldFocus = FrmGetObjectIndex(frmP, ExprField)));
	EditViewToggleVarsView(frmP);
}


/***********************************************************************
 *
 * FUNCTION:	EditViewHandleEvent
 *
 * DESCRIPTION: Edit view form event handler
 *
 * PARAMETERS:  event pointer
 *
 * RETURNED:	true if the event was handled 
 *
 ***********************************************************************/

static Boolean EditViewHandleEvent (EventType * evtP)
{
	FormPtr frmP;
	FieldPtr fldP;
	Char * str;
	WChar ascii;
	UInt16 focus;
	Boolean handled = false;
	static UInt8 sEditViewTapCount;

	switch (evtP->eType)
	{
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			EditViewInit(frmP);
			FrmDrawForm(frmP);
			handled = true;
		break;

		case frmCloseEvent:
			frmP = FrmGetActiveForm();
			EditViewSave(frmP);
		break;

		case fldEnterEvent:
			frmP = FrmGetActiveForm();
			EditViewToggleExprButtons(frmP);
		break;

		case fldChangedEvent:
			if (evtP->data.fldChanged.fieldID != VarsField && evtP->data.fldChanged.fieldID != ExprField)
				break;
			frmP = FrmGetActiveForm();
			MemoCalcUIUpdateScrollBar(frmP, evtP->data.fldChanged.fieldID,
			evtP->data.fldChanged.fieldID == VarsField ? VarsScrollBar : ExprScrollBar);
			if (evtP->data.fldChanged.fieldID == VarsField)
				LstSetSelection(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsList)), noListSelection);
			handled = true;
		break;

		case penDownEvent:
			sEditViewTapCount = evtP->tapCount;
		break;

		case keyDownEvent:
			if (evtP->data.keyDown.chr != vchrPageUp && evtP->data.keyDown.chr != vchrPageDown)
				break;
			sEditorSavePolicy = editorSaveMemo;
			frmP = FrmGetActiveForm();
			EditViewSave(frmP);
			sCurrentRecIndex = sSavedRecIndex;
			if (DmSeekRecordInCategory(sMemoDB, &sCurrentRecIndex, 1, evtP->data.keyDown.chr == vchrPageDown ? dmSeekForward : dmSeekBackward, sMemoCalcCategory))
			{
				sCurrentRecIndex = sSavedRecIndex;
				SndPlaySystemSound(sndError);
			}	
			EditViewInit(frmP);
			handled = true;
		break;

		case popSelectEvent:
			if (MathLibRef)
			{
				str = LstGetSelectionText(evtP->data.popSelect.listP, evtP->data.popSelect.selection);
				frmP = FrmGetActiveForm();
				if (FrmGetFocus(frmP) != FrmGetObjectIndex(frmP, ExprField))
					FrmSetFocus(frmP, FrmGetObjectIndex(frmP, ExprField));
				while (str && *str)
				{
					EvtEnqueueKey((WChar) *str, 0, 0);
					str++;
				}
				EvtEnqueueKey((WChar) '(', 0, 0);
				EvtEnqueueKey((WChar) ')', 0, 0);
				EvtEnqueueKey((WChar) chrLeftArrow, 0, 0);
			}
			handled = true;
		break;

		case lstExitEvent:
			LstSetSelection(evtP->data.lstExit.pList, noListSelection);
			handled = true;
		break;

		case lstSelectEvent:
			if (sEditViewTapCount > 1 && evtP->data.lstSelect.listID == VarsList)
			{
				str = LstGetSelectionText(evtP->data.lstSelect.pList, evtP->data.lstSelect.selection);
				frmP = FrmGetActiveForm();
				if (FrmGetFocus(frmP) != FrmGetObjectIndex(frmP, ExprField))
					FrmSetFocus(frmP, FrmGetObjectIndex(frmP, ExprField));
				while (str && *str && *str != '=')
				{
					EvtEnqueueKey((WChar) *str, 0, 0);
					str++;
				}
				handled = true;
			}
		break;

		case sclRepeatEvent:
			frmP = FrmGetActiveForm();
			MemoCalcUIScroll(frmP, evtP->data.sclRepeat.newValue - evtP->data.sclRepeat.value,
			evtP->data.sclRepeat.scrollBarID == VarsScrollBar ? VarsField : ExprField,
						evtP->data.sclRepeat.scrollBarID);
		break;

		case ctlSelectEvent:
			ascii = 0;
			switch (evtP->data.ctlSelect.controlID)
			{
				case DoneButton:
					sEditorSavePolicy = editorSaveMemo;
					FrmGotoForm(ListView);
					handled = true;
				break;

				case EvalButton:
					frmP = FrmGetActiveForm();
					EditViewEval(frmP);
					handled = true;
				break;

				case VarsButton:
					sVarsList = !sVarsList;
					frmP = FrmGetActiveForm();
					if (!sVarsList)
						FrmSetFocus(frmP, noFocus);
					EditViewToggleVarsView(frmP);
					handled = true;
				break;

				case Bclr:
					frmP = FrmGetActiveForm();
					fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
					FldDelete(fldP, 0, FldGetTextLength(fldP));
					handled = true;
				break;

				case Bequ:
					ascii = (WChar) '=';
				break;
				case B0:
					ascii = (WChar) '0';
				break;
				case B1:
					ascii = (WChar) '1';
				break;
				case B2:
					ascii = (WChar) '2';
				break;
				case B3:
					ascii = (WChar) '3';
				break;
				case B4:
					ascii = (WChar) '4';
				break;
				case B5:
					ascii = (WChar) '5';
				break;
				case B6:
					ascii = (WChar) '6';
				break;
				case B7:
					ascii = (WChar) '7';
				break;
				case B8:
					ascii = (WChar) '8';
				break;
				case B9:
					ascii = (WChar) '9';
				break;
				case Bdot:
					ascii = (WChar) '.';
				break;
				case Bpls:
					ascii = (WChar) '+';
				break;
				case Bmin:
					ascii = (WChar) '-';
				break;
				case Btim:
					ascii = (WChar) '*';
				break;
				case Bdiv:
					ascii = (WChar) '/';
				break;
				case Bexp:
					ascii = (WChar) '^';
				break;
				case Bopn:
					ascii = (WChar) '(';
				break;
				case Bcls:
					ascii = (WChar) ')';
				break;
				case Bdel:
					ascii = (WChar) chrBackspace;
				break;
			}
			if (ascii)
			{
				frmP = FrmGetActiveForm();
				if (sVarsList)
					FrmSetFocus(frmP, (sEditFieldFocus = FrmGetObjectIndex(frmP, ExprField)));
				else
					FrmSetFocus(frmP, sEditFieldFocus);
				EvtEnqueueKey(ascii, 0, 0);
				handled = true;
			}
		break;

		case menuEvent:
			frmP = FrmGetActiveForm();
			focus = FrmGetFocus(frmP);
			switch (evtP->data.menu.itemID)
			{
				case EditViewRecordNewMenu:
					sEditorSavePolicy = editorSaveMemo;
					EditViewSave(frmP);
					fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
					FldDelete(fldP, 0, FldGetTextLength(fldP));
					fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
					FldDelete(fldP, 0, FldGetTextLength(fldP));
					EditViewInit(frmP);
					handled = true;
					break;

				case EditViewRecordEditMenu:
					sEditorSavePolicy = editorSaveEmptyMemo;
					FrmGotoForm(MemoView);
					handled = true;
					break;

				case EditViewRecordOpenMenu:
					sEditorSavePolicy = editorSaveMemo;
					FrmGotoForm(ListView);
					handled = true;
					break;

				case EditViewRecordDeleteMenu:
					sEditorSavePolicy = editorDeleteMemo;
					FrmGotoForm(ListView);
					handled = true;
					break;

				case EditViewRecordDiscardMenu:
					sEditorSavePolicy = editorDiscardMemo;
		 			MemSet(&sAppEvent, sizeof(EventType), 0);
			 		sAppEvent.eType = appStopEvent;
			 		EvtAddEventToQueue(&sAppEvent);
					handled = true;
					break;

				case EditViewEditCutMenu:
					if ((focus != noFocus) && (fldP = FrmGetObjectPtr(frmP, focus)))
						FldCut(fldP);
					handled = true;
					break;

				case EditViewEditCopyMenu:
					if ((focus != noFocus) && (fldP = FrmGetObjectPtr(frmP, focus)))
						FldCopy(fldP);
					handled = true;
					break;

				case EditViewEditPasteMenu:
					if ((focus != noFocus) && (fldP = FrmGetObjectPtr(frmP, focus)))
						FldPaste(fldP);
					handled = true;
					break;

				case EditViewEditSelectAllMenu:
					if ((focus != noFocus) && (fldP = FrmGetObjectPtr(frmP, focus)))
						FldSetSelection(fldP, 0, FldGetTextLength(fldP));
					handled = true;
					break;

				case EditViewEditUndoMenu:
					if ((focus != noFocus) && (fldP = FrmGetObjectPtr(frmP, focus)))
						FldUndo(fldP);
					handled = true;
					break;

				case EditViewEditKeyboardMenu:
					SysKeyboardDialog(kbdDefault);
					handled = true;
					break;

				case EditViewOptionsAboutMenu:
					MenuEraseStatus(NULL);
					frmP = FrmInitForm(AboutDialog);
					FrmDoDialog(frmP);
					FrmDeleteForm(frmP);
					handled = true;
				break;

				case EditViewOptionsHexMenu:
					sEditorResultBase = resultBaseHexadecimal;
					frmP = FrmGetActiveForm();
					EditViewEval(frmP);
					handled = true;
					break;

				case EditViewOptionsDecMenu:
					sEditorResultBase = resultBaseDecimal;
					frmP = FrmGetActiveForm();
					EditViewEval(frmP);
					handled = true;
					break;

			}
		break;
	}

	return handled;
}


/***********************************************************************
 *
 * FUNCTION:	ListViewDrawRecord
 *
 * DESCRIPTION: Table callback to draw records in the list view
 *
 * PARAMETERS:  tblP, row, col, rec¨P
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void ListViewDrawRecord (void * tblP, Int16 row, Int16 col, RectanglePtr recP)
{
	MemHandle memoH;
	Char * memoP, * memoTitleP, * tmpP;
	UInt16 recIndex, titleLen, titleStrLen;

	recIndex = TblGetRowID(tblP, row);

	if ((memoH = DmQueryRecord(sMemoDB, recIndex)) == NULL)
		return;
	memoP = MemHandleLock(memoH);
	tmpP = StrChr (memoP, linefeedChr);
	if (tmpP)
	{
		titleStrLen = (1 + tmpP - memoP) * sizeof(Char);
		memoTitleP = MemPtrNew(titleStrLen);
		StrNCopy(memoTitleP, memoP, titleStrLen);
		memoTitleP[titleStrLen-1] = nullChr;
	}
	else
		memoTitleP = memoP;

	titleLen = FntWordWrap(memoTitleP, recP->extent.x);

	WinDrawChars (memoTitleP, titleLen, recP->topLeft.x, recP->topLeft.y);
	MemHandleUnlock(memoH);
	if (tmpP)
		MemPtrFree(memoTitleP);
}


/***********************************************************************
 *
 * FUNCTION:	ListViewUpdateScrollers
 *
 * DESCRIPTION: Update the scroll arrows
 *
 * PARAMETERS:  Pointer to the list view form, top and bottom indexes
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void ListViewUpdateScrollers (FormPtr frmP, UInt16 topIndex, UInt16 bottomIndex)
{
	UInt16 recIndex;
	Boolean scrollableUp = false;
	Boolean scrollableDown = false;

	recIndex = topIndex;
	if (DmSeekRecordInCategory(sMemoDB, &recIndex, 1, dmSeekBackward, sMemoCalcCategory) == errNone)
		scrollableUp = true;

	recIndex = bottomIndex;
	if (DmSeekRecordInCategory(sMemoDB, &recIndex, 1, dmSeekForward, sMemoCalcCategory) == errNone)
		scrollableDown = true;

	FrmUpdateScrollers(frmP, FrmGetObjectIndex(frmP, UpButton), FrmGetObjectIndex(frmP, DownButton), scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:	ListViewLoadTable
 *
 * DESCRIPTION: Load MemoDB records into the list view form
 *
 * PARAMETERS:  Pointer to the list view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void ListViewLoadTable (FormPtr frmP)
{
	TablePtr tblP;
	MemHandle recH;
	UInt32 recID;
	UInt16 recIndex;
	UInt16 nRows, row;

	tblP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprTable));

	recIndex = sTopVisibleRecIndex;

	// Store record unique ID in table data (UInt32)
	nRows = TblGetNumberOfRows(tblP);
	for (row = 0; row < nRows; row++)
	{
		recH = DmQueryNextInCategory(sMemoDB, &recIndex, sMemoCalcCategory);
		// Set the table row usable only if the record was found
		if (recH)
		{
			TblSetRowID(tblP, row, recIndex);
			DmRecordInfo(sMemoDB, recIndex, NULL, &recID, NULL);

			// Initialize row and force redraw only if this row has changed
			if ((TblGetRowData(tblP, row) != recID) || (! TblRowUsable(tblP, row)))
			{
				TblSetRowUsable(tblP, row, true);
				TblSetRowData(tblP, row, recID);
				TblMarkRowInvalid(tblP, row);
			}
			recIndex++;
		}
		else
		{
			TblSetRowUsable(tblP, row, false);
		}
	}

	TblDrawTable(tblP);
	ListViewUpdateScrollers(frmP, sTopVisibleRecIndex, recIndex-1);
}


/***********************************************************************
 *
 * FUNCTION:	ListViewScroll
 *
 * DESCRIPTION: Scrolls list view
 *
 * PARAMETERS:  Form pointer, direction
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void ListViewScroll (FormPtr frmP, Int16 direction)
{
	TablePtr tblP;
	UInt16 offset;
	UInt16 recIndex = sTopVisibleRecIndex;

	tblP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprTable));
	offset = (UInt16) (TblGetNumberOfRows(tblP) - 1);

	if (DmSeekRecordInCategory(sMemoDB, &recIndex, offset, direction, sMemoCalcCategory) == errNone)
	{
		sTopVisibleRecIndex = recIndex;
		ListViewLoadTable(frmP);
	}
}


/***********************************************************************
 *
 * FUNCTION:	ListViewInit
 *
 * DESCRIPTION: Initialize list view globals, call ListViewLoadTable
 *
 * PARAMETERS:  Pointer to the list view form
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void ListViewInit (FormPtr frmP)
{
	TablePtr tblP;
	UInt16 lastVisibleRecIndex;
	UInt16 nRows, row;

	tblP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprTable));
	nRows = TblGetNumberOfRows(tblP);
	TblSetCustomDrawProcedure (tblP, 0, ListViewDrawRecord);
	TblSetColumnUsable(tblP, 0, true);

	// Make sure sCurrentRecIndex is between sTopVisibleRecIndex and lastVisibleRecIndex
	if (sCurrentRecIndex != dmMaxRecordIndex)
	{
		if (sTopVisibleRecIndex > sCurrentRecIndex)
			sTopVisibleRecIndex = sCurrentRecIndex;
		else
		{
			lastVisibleRecIndex = sTopVisibleRecIndex;
			DmSeekRecordInCategory (sMemoDB, &lastVisibleRecIndex, nRows-1, dmSeekForward, sMemoCalcCategory);
			if (lastVisibleRecIndex < sCurrentRecIndex)
				sTopVisibleRecIndex = sCurrentRecIndex;
		}
	}

	for (row = 0; row < nRows; row++)
	{
		TblSetItemStyle(tblP, row, 0, customTableItem);
	}

}


/***********************************************************************
 *
 * FUNCTION:	ListViewHandleEvent
 *
 * DESCRIPTION: List view form event handler
 *
 * PARAMETERS:  event pointer
 *
 * RETURNED:	true if the event was handled 
 *
 ***********************************************************************/

static Boolean ListViewHandleEvent (EventType * evtP)
{
	FormPtr frmP;
	TablePtr tblP;
	Boolean handled = false;

	switch (evtP->eType)
	{
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			ListViewInit(frmP);
			FrmDrawForm(frmP);
	ListViewLoadTable(frmP);
			handled = true;
		break;

		case tblSelectEvent:
			frmP = FrmGetActiveForm();
			tblP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprTable));
			sCurrentRecIndex = TblGetRowID(tblP, evtP->data.tblSelect.row);
			FrmGotoForm(EditView);
			handled = true;
		break;

		case ctlSelectEvent:
			switch (evtP->data.ctlSelect.controlID)
			{
				case NewButton:
					sCurrentRecIndex = dmMaxRecordIndex;
					FrmGotoForm(EditView);
					handled = true;
				break;
			}
		break;

		case ctlRepeatEvent:
			switch (evtP->data.ctlRepeat.controlID)
			{
				case UpButton:
					frmP = FrmGetActiveForm();
					ListViewScroll(frmP, dmSeekBackward);
				break;

				case DownButton:
					frmP = FrmGetActiveForm();
					ListViewScroll(frmP, dmSeekForward);
				break;
			}
		break;

		case menuEvent:
			switch (evtP->data.menu.itemID)
			{
				case ListViewRecordNewMenu:
					MenuEraseStatus(NULL);
					sCurrentRecIndex = dmMaxRecordIndex;
					FrmGotoForm(EditView);
					handled = true;
				break;

				case ListViewRecordQuitMenu:
		 			MemSet(&sAppEvent, sizeof(EventType), 0);
			 		sAppEvent.eType = appStopEvent;
			 		EvtAddEventToQueue(&sAppEvent);
					handled = true;
				break;

				case ListViewOptionsAboutMenu:
					MenuEraseStatus(NULL);
					frmP = FrmInitForm(AboutDialog);
					FrmDoDialog(frmP);
					handled = true;
				break;
			}
		break;
	}

	return handled;
}




/***********************************************************************
 *
 * FUNCTION:	MemoViewHandleEvent
 *
 * DESCRIPTION: Memo view form event handler
 *
 * PARAMETERS:  event pointer
 *
 * RETURNED:	true if the event was handled 
 *
 ***********************************************************************/

static Boolean MemoViewHandleEvent (EventType * evtP)
{
	FormPtr frmP;
	FieldPtr fldP;
	Boolean handled = false;

	switch (evtP->eType)
	{
		case frmOpenEvent:
			sCurrentRecIndex = sSavedRecIndex;
			sMemoH = DmGetRecord(sMemoDB, sCurrentRecIndex);
			frmP = FrmGetActiveForm();
			fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, MemoViewEditField));
			FldSetTextHandle(fldP, sMemoH);
			MemoCalcUIUpdateScrollBar(frmP, MemoViewEditField, MemoViewScrollBar);
			FrmSetFocus(frmP, FrmGetObjectIndex(frmP, MemoViewEditField));
			FldSetInsertionPoint(fldP, 0);
			FrmDrawForm(frmP);
			handled = true;
		break;

		case frmCloseEvent:
			frmP = FrmGetActiveForm();
			fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, MemoViewEditField));
			FldSetTextHandle(fldP, NULL);
			DmReleaseRecord(sMemoDB, sCurrentRecIndex, true);
		break;

		case fldChangedEvent:
			frmP = FrmGetActiveForm();
			MemoCalcUIUpdateScrollBar(frmP, MemoViewEditField, MemoViewScrollBar);
			handled = true;
		break;

		case sclRepeatEvent:
			frmP = FrmGetActiveForm();
			MemoCalcUIScroll(frmP, evtP->data.sclRepeat.newValue - evtP->data.sclRepeat.value,
				MemoViewEditField, MemoViewScrollBar);
		break;

		case ctlSelectEvent:
			switch (evtP->data.ctlSelect.controlID)
			{
				case MemoViewOkButton:
					FrmGotoForm(EditView);
					handled = true;
				break;
			}
		break;
	}

	return handled;
}


/***********************************************************************
 *
 * FUNCTION:	ApplicationHandleEvent
 *
 * DESCRIPTION: Load form resources and set the form event handler
 *
 * PARAMETERS:  event pointer
 *
 * RETURNED:	true if the event was handled 
 *
 ***********************************************************************/

static Boolean ApplicationHandleEvent (EventType * evtP)
{
	FormPtr frmP;
	UInt16 formID;

	if (evtP->eType != frmLoadEvent)
		return false;

	formID = evtP->data.frmLoad.formID;
	frmP = FrmInitForm(formID);
	FrmSetActiveForm(frmP);

	switch (formID)
	{
	case ListView:
		FrmSetEventHandler (frmP, ListViewHandleEvent);
		break;

	case EditView:
		FrmSetEventHandler (frmP, EditViewHandleEvent);
		break;

	case MemoView:
		FrmSetEventHandler (frmP, MemoViewHandleEvent);
		break;

	}
	return true;
}


/***********************************************************************
 *
 * FUNCTION:	EventLoop
 *
 * DESCRIPTION: Main event loop for the MemoCalc application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void EventLoop (void)
{
	EventType evt;
	UInt16 err;

	do
	{
		EvtGetEvent(&evt, evtWaitForever);
		if (! SysHandleEvent(&evt))
			if (! MenuHandleEvent (NULL, &evt, &err))
				if (! ApplicationHandleEvent(&evt))
					FrmDispatchEvent (&evt);
	}
	while (evt.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:	PilotMain
 *
 * DESCRIPTION: Main entry point for the MemoCalc application.
 *
 * PARAMETERS:  launch code, command block pointer, launch flags
 *
 * RETURNED:	error code
 *
 ***********************************************************************/

UInt32 PilotMain (UInt16 launchCode, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 err = errNone;

	switch (launchCode)
	{
		case sysAppLaunchCmdNormalLaunch:
			err = StartApplication();
			if (err != errNone)
				break;
			FrmGotoForm(EditView);
			EventLoop();
			err = StopApplication();
		break;
	}

	return (UInt32) err;
}

