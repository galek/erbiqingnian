//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#ifndef __E_FEATURELINE_H__
#define __E_FEATURELINE_H__

/*
	This is similar to the "sliders," but offers a finite number
	of distinct "stops" along a number-line-like ordered continuum,
	some or all of which will map (via a provided mapping function)
	to a set of user-friendly "choices."

	The following principles influenced the design of this system:

	*	The number and meaning of "stops" is controlled in code.

	*	The mapping matrix/function that maps "stops" to "choices"
		is implemented in code.

	*	The compatibility database can be used to adjust defaults
		based on the hardware.

	*	Stops are referred to by name (as opposed to ordinal number)
		to allow for insertion and removal of "stops" without
		invalidating external references (such as in the
		compatibility database).

	*	User's choices are stored persistenly by the "choice" name,
		not the "stop" name.  This abstracts the user-editable
		setting away from hardware and implemtation particulars,
		as well as facilitates user editing with friendly names.

	Here is an example of how this would work for different
	qualities of shadows:

	The Shadow Continuum
	--------------------
	(NOT a Babylon 5 episode.)

	Ord.	Cont.	Name	Description
	----	-----	----	-----------
	0		SHDW	NONE	No shadows
	1		SHDW	SDWL	half-res shadow map
	2		SHDW	SDWH	full-res shadow map

	(Ordinal numbers are used only internally at run-time.)

	These stops are made available to the mapping function.  The
	mapping function may offer the following choices:

		High
		Low
		Off
*/

// Includes.
#include "e_irefcount.h"
#include "globalindex.h"

// Macros.
#define E_FEATURELINE_FOURCC(fourcc) MAKEFOURCC(fourcc[0],fourcc[1],fourcc[2],fourcc[3])

// Forward declarations.
class FeatureLineMap;
class OptionState;
class SettingsExchange;

// Error codes.
const HRESULT E_NO_MORE_ITEMS = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
const HRESULT E_INSUFFICIENT_BUFFER = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
const HRESULT E_NOT_FOUND = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
const HRESULT E_ALREADY_REGISTERED = HRESULT_FROM_WIN32(ERROR_ALREADY_REGISTERED);

// Abstract base class for implementation of feature lines.
class FeatureLine : public IRefCount
{
	public:
		// Base destructor.
		virtual ~FeatureLine(void);

		// Enumerate the stops for this line.
		virtual bool EnumerateStops(unsigned nIndex, DWORD & nNameOut) = 0;

		// Perform mapping -- create the available choices and associate them with stops.
		virtual PRESULT MapChoices(FeatureLineMap * pMap, DWORD nLineName, const OptionState * pState) = 0;

		// Select a stop for use.
		// This actually performs the action of configuring the
		// other engine systems for this stop.
		virtual PRESULT SelectStop(DWORD nStopName, OptionState * pState) = 0;

	protected:
		// Call this during MapChoices().
		static PRESULT e_FeatureLine_AddChoice(FeatureLineMap * pMap, DWORD nLineName, DWORD nStopName, const char * pInternalChoiceName, GLOBAL_STRING eGlobalStr);
};

//
// The FeatureLineMap is a "snapshot" of the mapping of stops to
// user choices for a given option state.  The mapping uses the
// state to determine feature availability and map the stops
// accordingly.  Because of this, note that any changes to the
// state (including selection of stops that in turn adjust the
// state) invalidate the mapping and a new mapping must be
// created.
// CHB !!! - recreate mapping automatically??
//

class FeatureLineMap : public IRefCount
{
	//---
	// Choice functions - The game should concern itself only with
	// choices.  The choices reflect currently available features
	// and the user's persistent setting.  The game should not deal
	// with stops directly, which are exposed only for use by the
	// init DB.
	//---
	public:

		// The internal name is an ASCII (English) name user for persistent storage.
		// Internal names are not case-sensitive.
		PRESULT EnumerateChoiceInternalNames(DWORD nLineName, unsigned nChoiceIndex, char * pBufferOut, unsigned & nBufferChars) const;

		// The display name is the wide-character multi-lingual choice text.
		PRESULT EnumerateChoiceDisplayNames(DWORD nLineName, unsigned nChoiceIndex, wchar_t * pBufferOut, unsigned & nBufferChars) const;

		// Get the currently selected choice.
		PRESULT GetSelectedChoice(DWORD nLineName, unsigned & nChoiceIndexOut) const;

		// Select a choice.
		// This selects the choice to the OptionState given when opening the map.
		// The OptionState would need to be committed separately to actually take effect.
		PRESULT SelectChoice(DWORD nLineName, const char * pInternalChoiceName);
		//PRESULT SelectChoice(DWORD nLineName, const wchar_t * pDisplayChoiceName);
		PRESULT SelectChoice(DWORD nLineName, unsigned nChoiceIndex);

		// Select the choice closest to the point given 0 <= fPoint <= 1,
		// where 0 represents the lowest available graphic sophistication,
		// and 1 is the highest.
		PRESULT SelectPoint(DWORD nLineName, float fPoint);

		// Re-do the choice-to-stops mappings (such as after the state has changed).
		// State changes invalidate the mapping (see comment at the top of this class).
		PRESULT ReMap(void);

		// Load & save from persistent storage.
		PRESULT LoadFromFile(const OS_PATH_CHAR * pFileName, const char * pSectionName);
		PRESULT SaveToFile(const OS_PATH_CHAR * pFileName, const char * pSectionName) const;

		// Utilities.
		PRESULT Clone(FeatureLineMap * * ppMapOut) const;
		PRESULT GetState(OptionState * * ppStateOut) const;

	//---
	// Stops functions - For use by init DB only.
	// Functions below should not be used by the game.
	//---
	public:

		// Selects the highest available choice that is below or equal to the given stop.
		PRESULT SelectChoiceFromStopFloor(DWORD nLineName, DWORD nStopName);

		// Adjust and restrict the minimum stop as appropriate.
		// The function always takes the most restrictive action:
		//	*	If the desired minimum is less than or equal to the current minimum, no action is taken.
		//	*	If the desired minimum is greater than the current minimum, it becomes the new minimum.
		PRESULT AdjustMinimumStop(DWORD nLineName, DWORD nMinStopName);

		// Adjust and restrict the maximum stop as appropriate.
		// The function always takes the most restrictive action:
		//	*	If the desired maximum is greater than or equal to the current maximum, no action is taken.
		//	*	If the desired maximum is less than the current maximum, it becomes the new maximum.
		PRESULT AdjustMaximumStop(DWORD nLineName, DWORD nMaxStopName);

		// Retrieve the current minimum and maximum.
		PRESULT GetMinimumAndMaximumStops(DWORD nLineName, DWORD & nMinStopNameOut, DWORD & nMaxStopNameOut) const;

		// Get the currently selected choice.
		PRESULT GetSelectedStopName(DWORD nLineName, DWORD & nStopNameOut) const;

	//---
	// These functions are for internal use only, but are public
	// only for convenience of access.
	//---
	public:
		// Call this during MapChoices().
		PRESULT AddChoice(DWORD nLineName, DWORD nStopName, const char * pInternalChoiceName, GLOBAL_STRING eGlobalStr);

		// Helper for persistent storage and retrieval.
		void Exchange(SettingsExchange & se);

	protected:
		FeatureLineMap(void);
		virtual ~FeatureLineMap(void);

	private:
		PRESULT RemapAfterRangeRestriction(DWORD nLineName);

		struct Maps;
		Maps * pMaps;

	// Copying forbidden.
	private:
		FeatureLineMap(const FeatureLineMap & rhs);
		FeatureLineMap & operator=(const FeatureLineMap & rhs);

	friend PRESULT e_FeatureLine_OpenMap(OptionState * pState, FeatureLineMap * * pMapOut);
	friend PRESULT e_FeatureLine_CommitToActive(const FeatureLineMap * pNewState);
};

// Initialize & deinitialize.
PRESULT e_FeatureLine_Init(void);
PRESULT e_FeatureLine_Deinit(void);

// Add a feature line to the system.
// The passed-in object should be allocated via new,
// and the feature line system will "own" the pointer
// (be responsible for deleting it when appropriate).
PRESULT e_FeatureLine_Register(FeatureLine * pLine, DWORD nName);

// List the names of the registered feature lines.
bool e_FeatureLine_EnumerateLines(unsigned nIndex, DWORD & nNameOut);

// List the names of all stops on the given feature line.
PRESULT e_FeatureLine_EnumerateStopNames(DWORD nLineName, unsigned nIndex, DWORD & nNameOut);

// Retrieve the fourCC name of the currently selected stop on the given feature line.
PRESULT e_FeatureLine_GetSelectedStopName( DWORD nLineName, FeatureLineMap * pMap, DWORD & nNameOut);

// Retrieve the internal name of the currently selected stop on the given feature line.
PRESULT e_FeatureLine_GetSelectedStopInternalName( DWORD nLineName, FeatureLineMap * pMap, char * pszInternalName, unsigned int nBufLen );

// Compares two stops on a feature line, and returns:
//	* S_OK if nStopLHS is earlier on the feature line than nStopRHS.
//	* S_FALSE if nStopLHS is not earlier on the feature line than nStopRHS.
//	* Some other value in the event of an error.
PRESULT e_FeatureLine_IsStopLessThan(DWORD nLineName, DWORD nStopLHS, DWORD nStopRHS);

// Create a mapping for all registered lines based on the given state.
PRESULT e_FeatureLine_OpenMap(OptionState * pState, FeatureLineMap * * pMapOut);

// Records the map's selected stops to the "active" selected stops.
PRESULT e_FeatureLine_CommitToActive(const FeatureLineMap * pNewState);

#endif
