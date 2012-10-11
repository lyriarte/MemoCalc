
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

#include "MemoCalc.h"
#include <PalmOS.h>
#include <TraceMgr.h>


/***********************************************************************
 *
 *	Defines, Macros and Constants
 *
 ***********************************************************************/

#define sysFileCMemoCalc			'MeCa'
#define memoDBType					'DATA'
#define memoCalcDefaultCategoryName	"MemoCalc"

/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/

/* **** **** Memo DB **** **** */
static DmOpenRef sMemoDB;
static UInt16 sMemoCalcCategory;

/* **** **** List View **** **** */
static UInt16 sCurrentRecIndex;
static UInt16 sTopVisibleRecIndex;

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/


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
	sMemoDB = NULL;
	sMemoCalcCategory = dmAllCategories;
	sCurrentRecIndex = dmMaxRecordIndex;
	sTopVisibleRecIndex = 0;
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
	return err;
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
	MemHandle memoH;
	FieldPtr fldP;

	if (sCurrentRecIndex == dmMaxRecordIndex)
		return;
	
	fldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
	memoH = DmQueryRecord(sMemoDB, sCurrentRecIndex);
	FldSetTextHandle (fldP, memoH);
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
			exprFldP = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ExprField));
			FldSetTextHandle (exprFldP, NULL);
		break;


		case ctlSelectEvent:
			switch (evtP->data.ctlSelect.controlID)
			{
				case DoneButton:
					FrmGotoForm(ListView);
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

