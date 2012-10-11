
/***********************************************************************
 *
 * FILE : MemoCalc.c
 * 
 * DESCRIPTION : PalmOS UI and DB related funcions for MemoCalc
 * 
 * COPYRIGHT : GNU GENERAL PUBLIC LICENSE
 * http://www.gnu.org/licenses/gpl.txt
 *
 ***********************************************************************/

#define TRACE_OUTPUT TRACE_OUTPUT_ON

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

#define kFlpBufSize					64
#define kErrorStr					"Error"
#define kExprTag					"<--expr-->"
#define kVarsTag					"<--vars-->"
#define kExprTagLen					10
#define kVarsTagLen					10

/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/

/* **** **** MathLib **** **** */
extern UInt16 MathLibRef;

/* **** **** Memo DB **** **** */
static DmOpenRef sMemoDB;
static UInt16 sMemoCalcCategory;

/* **** **** List View **** **** */
static UInt16 sCurrentRecIndex, sTopVisibleRecIndex;

/* **** **** Edit View **** **** */
static MemHandle sMemoH;

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/


/***********************************************************************
 *
 * FUNCTION:    MemoCalcMathLibOpen
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
TraceOutput(TL(appErrorClass, "MemoCalcMathLibOpen err %x", err));
	return err;
}


/***********************************************************************
 *
 * FUNCTION:    MemoCalcMathLibClose
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
 * FUNCTION:    MemoCalcDBOpen
 *
 * DESCRIPTION: Open the default Memo database in rw mode
 *		Check if category memoCalcDefaultCategoryName exists, otherwise
 *		use dmAllCategories
 *
 * PARAMETERS:  DmOpenRef *dbPP
 *
 * RETURNED:    error code
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
 * FUNCTION:    MemoCalcDBClose
 *
 * DESCRIPTION: Close the default Memo database
 *
 * PARAMETERS:  none
 *
 * RETURNED:    error code
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
 * FUNCTION:    StartApplication
 *
 * DESCRIPTION: Default initialization for the MemoCalc application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    error code
 *
 ***********************************************************************/

static UInt16 StartApplication (void)
{
	UInt16 err;
	MathLibRef = NULL;
	sMemoDB = NULL;
	sMemoCalcCategory = dmAllCategories;
	sCurrentRecIndex = dmMaxRecordIndex;
	sTopVisibleRecIndex = 0;
	MemoCalcMathLibOpen();
	err = MemoCalcDBOpen(&sMemoDB, &sMemoCalcCategory);

TraceOutput(TL(appErrorClass, "StartApplication DB %lx category %d err %x", sMemoDB, sMemoCalcCategory, err));

	return err;
}


/***********************************************************************
 *
 * FUNCTION:    StopApplication
 *
 * DESCRIPTION: Default cleanup code for the MemoCalc application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    error code
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
 * FUNCTION:    EditViewEval
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/

static void EditViewEval (FormPtr frmP)
{
	static Char resultBuf[kFlpBufSize];
	FieldPtr exprFldP, varsFldP, resultFldP;
	Char * exprStr, * varsStr;
	FlpCompDouble result;
	UInt8 err;

	exprFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
	varsFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
	resultFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ResultField));

	exprStr = FldGetTextPtr(exprFldP);
	varsStr = FldGetTextPtr(varsFldP);

	err = Eval(exprStr, varsStr, &(result.d));

	if (err)
		StrCopy(resultBuf, kErrorStr);
	else
		FlpFToA(result.fd, resultBuf);

	FldSetTextPtr(resultFldP, resultBuf);
	FrmUpdateForm(EditView, frmRedrawUpdateCode);
}


/***********************************************************************
 *
 * FUNCTION:    EditViewSave
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/

static void EditViewSave (FormPtr frmP)
{

	MemHandle exprH, varsH;
	Char * memoStr, * exprStr, * varsStr, * tmpStr;
	FieldPtr exprFldP, varsFldP;
	UInt16 memoLen, exprLen, varsLen, titleLen, attr;

	memoLen = exprLen = varsLen = titleLen = 0;
	memoStr = exprStr = varsStr = tmpStr = NULL;

TraceOutput(TL(appErrorClass, "EditViewSave memoLen: %d exprLen: %d varsLen: %d titleLen: %d ",
                              memoLen, exprLen, varsLen, titleLen));

	if (sMemoH)
	{
		memoStr = (Char*) MemHandleLock(sMemoH);
TraceOutput(TL(appErrorClass, "EditViewSave memoStr : %s", memoStr));
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

TraceOutput(TL(appErrorClass, "EditViewSave memoLen: %d exprLen: %d varsLen: %d titleLen: %d ",
                              memoLen, exprLen, varsLen, titleLen));
	exprFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
	exprH = FldGetTextHandle(exprFldP);
	if (exprH)
	{
		exprStr = (Char*) MemHandleLock(exprH);
		exprLen = StrLen(exprStr);
TraceOutput(TL(appErrorClass, "EditViewSave expr_%d_%s", exprLen, exprStr));
	}

TraceOutput(TL(appErrorClass, "EditViewSave memoLen: %d exprLen: %d varsLen: %d titleLen: %d ",
                              memoLen, exprLen, varsLen, titleLen));
	varsFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
	varsH = FldGetTextHandle(varsFldP);
	if (varsH)
	{
		varsStr = (Char*) MemHandleLock(varsH);
		varsLen = StrLen(varsStr);
TraceOutput(TL(appErrorClass, "EditViewSave varsStr_%d_%s", varsLen, varsStr));
	}

	memoLen = titleLen + kVarsTagLen + varsLen + kExprTagLen + exprLen;

TraceOutput(TL(appErrorClass, "EditViewSave memoLen: %d exprLen: %d varsLen: %d titleLen: %d ",
                              memoLen, exprLen, varsLen, titleLen));
	if (sCurrentRecIndex != dmMaxRecordIndex)
	{
		if (sMemoH)
			MemHandleUnlock(sMemoH);
		DmReleaseRecord(sMemoDB, sCurrentRecIndex, false);
		DmResizeRecord(sMemoDB, sCurrentRecIndex, memoLen + 1 );
		sMemoH = DmGetRecord(sMemoDB, sCurrentRecIndex);
	}
	else
	{
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
TraceOutput(TL(appErrorClass, "EditViewSave memoStr : %s", memoStr));

	DmWrite(memoStr, titleLen, kVarsTag, kVarsTagLen);
TraceOutput(TL(appErrorClass, "_%d_%s", titleLen, kVarsTag));
	if (varsLen)
	{
		DmWrite(memoStr, titleLen + kVarsTagLen, varsStr, varsLen);
TraceOutput(TL(appErrorClass, "_%d_%s", titleLen + kVarsTagLen, varsStr));
	}
TraceOutput(TL(appErrorClass, "EditViewSave memoVarsStr : %s", memoStr));

	DmWrite(memoStr, titleLen + kVarsTagLen + varsLen, kExprTag, kExprTagLen);
TraceOutput(TL(appErrorClass, "_%d_%s", titleLen + kVarsTagLen + varsLen, kExprTag));
	if (exprLen)
	{
		DmWrite(memoStr, titleLen + kVarsTagLen + varsLen + kExprTagLen, exprStr, exprLen);
TraceOutput(TL(appErrorClass, "_%d_%s", titleLen + kVarsTagLen + varsLen + kExprTagLen, exprStr));
	}
TraceOutput(TL(appErrorClass, "EditViewSave memoVarsExprStr : %s", memoStr));

	DmSet(memoStr, memoLen, 1, 0);
TraceOutput(TL(appErrorClass, "EditViewSave memoStr : %s", memoStr));

	if (exprH)
		MemHandleUnlock(exprH);
	if (varsH)
		MemHandleUnlock(varsH);
	if (sMemoH)
	{
		MemHandleUnlock(sMemoH);
		sMemoH = NULL;
	}
	if (sCurrentRecIndex != dmMaxRecordIndex)
	{
		DmReleaseRecord(sMemoDB, sCurrentRecIndex, false);
		sCurrentRecIndex = dmMaxRecordIndex;
	}
}


/***********************************************************************
 *
 * FUNCTION:    EditViewInit
 *
 * DESCRIPTION: Initialize edit view form, load current record
 *
 * PARAMETERS:  Pointer to the edit view form
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/

static void EditViewInit (FormPtr frmP)
{
	MemHandle exprH, varsH;
	Char * memoStr, * exprStr, *varsStr, * tmpStr;
	FieldPtr exprFldP, varsFldP;

	sMemoH = NULL;
	if (sCurrentRecIndex == dmMaxRecordIndex)
		return;
	
	sMemoH = DmGetRecord(sMemoDB, sCurrentRecIndex);
	memoStr = (Char*) MemHandleLock(sMemoH);
TraceOutput(TL(appErrorClass, "EditViewInit memo : %s", memoStr));
	exprStr = StrStr(memoStr, kExprTag);
	if (exprStr)
	{
		exprStr += kExprTagLen;
TraceOutput(TL(appErrorClass, "EditViewInit expr : %s", exprStr));
		exprH = MemHandleNew(1 + (UInt32)exprStr - (UInt32)memoStr);
		tmpStr = MemHandleLock(exprH);
		StrNCopy(tmpStr, exprStr, (UInt32)exprStr - (UInt32)memoStr);
		tmpStr[(UInt16)((UInt32)exprStr - (UInt32)memoStr)] = nullChr;
		MemHandleUnlock(exprH);
		exprFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
		FldSetTextHandle (exprFldP, exprH);
	}
	varsStr = StrStr(memoStr, kVarsTag);
	if (varsStr && exprStr && varsStr < exprStr)
	{
		varsStr += kVarsTagLen;
TraceOutput(TL(appErrorClass, "EditViewInit vars : %s", varsStr));
		varsH = MemHandleNew(1 + (UInt32)exprStr - kExprTagLen - (UInt32)varsStr);
		tmpStr = MemHandleLock(varsH);
		StrNCopy(tmpStr, varsStr, (UInt32)exprStr - kExprTagLen - (UInt32)varsStr);
		tmpStr[(UInt16)((UInt32)exprStr - kExprTagLen - (UInt32)varsStr)] = nullChr;
		MemHandleUnlock(varsH);
		varsFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, VarsField));
		FldSetTextHandle (varsFldP, varsH);
	}

	MemHandleUnlock(sMemoH);
}


/***********************************************************************
 *
 * FUNCTION:    EditViewHandleEvent
 *
 * DESCRIPTION: Edit view form event handler
 *
 * PARAMETERS:  event pointer
 *
 * RETURNED:    true if the event was handled 
 *
 ***********************************************************************/

static Boolean EditViewHandleEvent (EventType * evtP)
{
	FormPtr frmP;
	FieldPtr exprFldP;
	Boolean handled = false;

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


		case ctlSelectEvent:
			switch (evtP->data.ctlSelect.controlID)
			{
				case DoneButton:
					FrmGotoForm(ListView);
					handled = true;
				break;

				case EvalButton:
					frmP = FrmGetActiveForm();
					EditViewEval(frmP);
					handled = true;
				break;
			}
		break;

		case menuEvent:
			switch (evtP->data.menu.itemID)
			{
				case EditViewOptionsAboutMenu:
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
 * FUNCTION:    ListViewDrawRecord
 *
 * DESCRIPTION: Table callback to draw records in the list view
 *
 * PARAMETERS:  tblP, row, col, rec¨P
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/

static void ListViewDrawRecord (void * tblP, Int16 row, Int16 col, RectanglePtr recP)
{
	MemHandle memoH;
	Char * memoP, * memoTitleP, * tmpP;
	UInt16 recIndex, titleLen, titleStrLen;

	recIndex = TblGetRowID(tblP, row);

TraceOutput(TL(appErrorClass, "ListViewDrawRecord recP %lx row %d index %d", recP, row, recIndex));

	memoH = DmQueryRecord(sMemoDB, recIndex);
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

TraceOutput(TL(appErrorClass, "ListViewDrawRecord len %d : %s", titleLen, memoTitleP));

	WinDrawChars (memoTitleP, titleLen, recP->topLeft.x, recP->topLeft.y);
	MemHandleUnlock(memoH);
	if (tmpP)
		MemPtrFree(memoTitleP);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewUpdateScrollers
 *
 * DESCRIPTION: Update the scroll arrows
 *
 * PARAMETERS:  Pointer to the list view form, top and bottom indexes
 *
 * RETURNED:    nothing
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

TraceOutput(TL(appErrorClass, "ListViewUpdateScrollers top %d bottom %d up %s down %s", topIndex, bottomIndex, 
			scrollableUp ? "1" : "0", scrollableDown ? "1" : "0"));

	FrmUpdateScrollers(frmP, FrmGetObjectIndex(frmP, UpButton), FrmGetObjectIndex(frmP, DownButton), scrollableUp, scrollableDown);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewLoadTable
 *
 * DESCRIPTION: Load MemoDB records into the list view form
 *
 * PARAMETERS:  Pointer to the list view form
 *
 * RETURNED:    nothing
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

TraceOutput(TL(appErrorClass, "ListViewLoadTable row %d index %d uid %lx", row, recIndex, recID));

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

TraceOutput(TL(appErrorClass, "ListViewLoadTable row %d empty", row));

			TblSetRowUsable(tblP, row, false);
		}
	}

	TblDrawTable(tblP);
	ListViewUpdateScrollers(frmP, sTopVisibleRecIndex, recIndex-1);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewScroll
 *
 * DESCRIPTION: Scrolls list view
 *
 * PARAMETERS:  Form pointer, direction
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/

static void ListViewScroll (FormPtr frmP, Int16 direction)
{
	TablePtr tblP;
	UInt16 offset;
	UInt16 recIndex = sTopVisibleRecIndex;

	tblP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprTable));
	offset = (UInt16) (TblGetNumberOfRows(tblP) - 1);

	TraceOutput(TL(appErrorClass, "ListViewScroll top %d offset %d direction %d", recIndex, offset, direction));

	if (DmSeekRecordInCategory(sMemoDB, &recIndex, offset, direction, sMemoCalcCategory) == errNone)
	{
TraceOutput(TL(appErrorClass, "ListViewScroll new top %d", recIndex));
		sTopVisibleRecIndex = recIndex;
		ListViewLoadTable(frmP);
	}
}


/***********************************************************************
 *
 * FUNCTION:    ListViewInit
 *
 * DESCRIPTION: Initialize list view globals, call ListViewLoadTable
 *
 * PARAMETERS:  Pointer to the list view form
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/

static void ListViewInit (FormPtr frmP)
{
	TablePtr tblP;
	UInt16 lastVisibleRecIndex;
	UInt16 nRows, row;

	tblP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprTable));
	nRows = TblGetNumberOfRows(tblP);

TraceOutput(TL(appErrorClass, "ListViewInit form 0x%lx table 0x%lx rows %d", frmP, tblP, nRows));


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

TraceOutput(TL(appErrorClass, "ListViewInit current %d top %d last %d ", sCurrentRecIndex, sTopVisibleRecIndex, lastVisibleRecIndex));

	for (row = 0; row < nRows; row++)
	{
		TblSetItemStyle(tblP, row, 0, customTableItem);
	}

	ListViewLoadTable(frmP);
	TblSetCustomDrawProcedure (tblP, 0, ListViewDrawRecord);
	TblSetColumnUsable(tblP, 0, true);
}


/***********************************************************************
 *
 * FUNCTION:    ListViewHandleEvent
 *
 * DESCRIPTION: List view form event handler
 *
 * PARAMETERS:  event pointer
 *
 * RETURNED:    true if the event was handled 
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
TraceOutput(TL(appErrorClass, "ListViewHandleEvent : init done, drawing form"));
			FrmDrawForm(frmP);
TraceOutput(TL(appErrorClass, "ListViewHandleEvent : form drawn"));
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
 * FUNCTION:    ApplicationHandleEvent
 *
 * DESCRIPTION: Load form resources and set the form event handler
 *
 * PARAMETERS:  event pointer
 *
 * RETURNED:    true if the event was handled 
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

	}
	return true;
}


/***********************************************************************
 *
 * FUNCTION:    EventLoop
 *
 * DESCRIPTION: Main event loop for the MemoCalc application.
 *
 * PARAMETERS:  none
 *
 * RETURNED:    nothing
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
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: Main entry point for the MemoCalc application.
 *
 * PARAMETERS:  launch code, command block pointer, launch flags
 *
 * RETURNED:    error code
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
			FrmGotoForm(ListView);
			EventLoop();
			err = StopApplication();
		break;
	}

	return (UInt32) err;
}

