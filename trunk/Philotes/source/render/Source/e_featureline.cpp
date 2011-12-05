//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_featureline.h"
#include "e_optionstate.h"
#include "stringtable.h"	// used by e_FeatureLine_AddChoice()
#include "settingsexchange.h"
#include "safe_vector.h"
#include <map>
#include "memoryallocator_stl.h"
#include <string>

using namespace std;
using namespace FSCommon;

//-----------------------------------------------------------------------------

namespace
{

//-----------------------------------------------------------------------------

// Holds all the data pertinent to a choice: its name, associated stop, etc.
struct ChoiceData
{
	DWORD nStopName;
	std::string InternalName;
	std::wstring DisplayName;
};
typedef safe_vector<ChoiceData> ChoiceDataVector;

//-----------------------------------------------------------------------------

// Housekeeping for registered line objects.
struct LineData
{
	CComPtr<FeatureLine> pLineObj;

	LineData(FeatureLine * pLineObj);
	~LineData(void);
};
typedef map<DWORD, LineData, less<DWORD>, CMemoryAllocatorSTL<pair<DWORD, LineData> > > LineDataMap;

//-----------------------------------------------------------------------------

// Holds state data for a line -- i.e., which stop is currently selected.
struct LineStateData
{
	DWORD nSelectedStop;
	int nMinimumStopIndex;
	int nMaximumStopIndex;

	void GetMinMaxStopIndex(DWORD nLineName, int & nMinIndexOut, int & nMaxIndexOut) const;

	LineStateData(void);
};
typedef map<DWORD, LineStateData, less<DWORD>, CMemoryAllocatorSTL<pair<DWORD, LineStateData> > > LineStateDataMap;

//-----------------------------------------------------------------------------

// Holds the mapping of choices to stops for a feature line.
struct Mapping
{
	PRESULT GetChoiceIndexFromStopName(DWORD nName, unsigned & nIndexOut) const;
	PRESULT GetChoiceIndexFromInternalName(const char * pName, unsigned & nIndexOut) const;
	PRESULT GetChoiceIndexFromDisplayName(const wchar_t * pName, unsigned & nIndexOut) const;

	ChoiceDataVector Choices;
	bool bMapping;

	Mapping(void);
	~Mapping(void);
};

//-----------------------------------------------------------------------------

// Encapsulates the module-wide global data.
struct Module
{
	Module() :
		Lines(LineDataMap::key_compare(), LineDataMap::allocator_type(g_StaticAllocator)),
		LineStates(LineStateDataMap::key_compare(), LineStateDataMap::allocator_type(g_StaticAllocator))
	{
	}

	LineDataMap Lines;
	LineStateDataMap LineStates;
};
Module * s_pModule = 0;

//-----------------------------------------------------------------------------

LineData::LineData(FeatureLine * pLineObjIn)
	: pLineObj(pLineObjIn)
{
}

LineData::~LineData(void)
{
}

//-----------------------------------------------------------------------------

bool e_FeatureLine_Find(DWORD nLineName, LineDataMap::iterator & iOut)
{
	iOut = s_pModule->Lines.find(nLineName);
	return iOut != s_pModule->Lines.end();
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Unregister(DWORD nLineName)
{
	LineDataMap::iterator i;
	if (!e_FeatureLine_Find(nLineName, i))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	s_pModule->Lines.erase(i);
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT e_FeaturLine_GetStopIndexFromStopName(DWORD nLineName, DWORD nStopName, int & nIndexOut)
{
	for (int i = 0; /**/; ++i)
	{
		DWORD nName = 0;
		PRESULT nResult = e_FeatureLine_EnumerateStopNames(nLineName, i, nName);
		ASSERT_RETVAL(nResult != E_NO_MORE_ITEMS, E_NOT_FOUND);	// Stop name not found.
		V_RETURN(nResult);
		if (nStopName == nName)
		{
			// Found.
			nIndexOut = i;
			return S_OK;
		}
	}
	return E_NOT_FOUND;
}

//-----------------------------------------------------------------------------

PRESULT Mapping::GetChoiceIndexFromStopName(DWORD nName, unsigned & nIndexOut) const
{
	for (unsigned i = 0; i < Choices.size(); ++i)
	{
		if (nName == Choices[i].nStopName)
		{
			nIndexOut = i;
			return S_OK;
		}
	}
	return E_NOT_FOUND;
}

PRESULT Mapping::GetChoiceIndexFromInternalName(const char * pName, unsigned & nIndexOut) const
{
	for (unsigned i = 0; i < Choices.size(); ++i)
	{
		if (PStrICmp(pName, Choices[i].InternalName.c_str()) == 0)
		{
			nIndexOut = i;
			return S_OK;
		}
	}
	return E_NOT_FOUND;
}

PRESULT Mapping::GetChoiceIndexFromDisplayName(const wchar_t * pName, unsigned & nIndexOut) const
{
	for (unsigned i = 0; i < Choices.size(); ++i)
	{
		if (PStrICmp(pName, Choices[i].DisplayName.c_str()) == 0)
		{
			nIndexOut = i;
			return S_OK;
		}
	}
	return E_NOT_FOUND;
}

Mapping::Mapping(void)
{
}

Mapping::~Mapping(void)
{
}

//-----------------------------------------------------------------------------

void LineStateData::GetMinMaxStopIndex(DWORD nLineName, int & nMinIndexOut, int & nMaxIndexOut) const
{
	if (nMinimumStopIndex < 0)
	{
		nMinIndexOut = 0;
	}
	else
	{
		nMinIndexOut = nMinimumStopIndex;
	}

	if (nMaximumStopIndex < 0)
	{
		for (int i = 0; /**/; ++i)
		{
			DWORD nName;
			if (FAILED(e_FeatureLine_EnumerateStopNames(nLineName, i, nName)))
			{
				nMaxIndexOut = i - 1;
				break;
			}
		}
	}
	else
	{
		nMaxIndexOut = nMaximumStopIndex;
	}
}

LineStateData::LineStateData(void)
{
	nSelectedStop = 0;
	nMinimumStopIndex = -1;
	nMaximumStopIndex = -1;
}

//-----------------------------------------------------------------------------

};


//-----------------------------------------------------------------------------
//
// Support functions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

struct FeatureLineMap::Maps
{
	typedef map<DWORD, Mapping, less<DWORD>, CMemoryAllocatorSTL<pair<DWORD, Mapping> > > MappingMap;	// yeah yeah, I know

	bool Find(DWORD nLineName, MappingMap::iterator & iOut);
	PRESULT SelectChoice(DWORD nLineName, unsigned nChoiceIndex);
#if 0
	PRESULT AdjustSelectedChoice(DWORD nLineName);
#endif

	CComPtr<OptionState> pState;
	MappingMap Mappings;
	LineStateDataMap LineStates;
};

bool FeatureLineMap::Maps::Find(DWORD nLineName, MappingMap::iterator & iOut)
{
	iOut = Mappings.find(nLineName);
	return iOut != Mappings.end();
}

PRESULT FeatureLineMap::Maps::SelectChoice(DWORD nLineName, unsigned nChoiceIndex)
{
	MappingMap::iterator iMap;
	if (!Find(nLineName, iMap))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = iMap->second;
	ASSERT_RETVAL(!m.bMapping, E_FAIL);	// inside MapChoices()

	if (nChoiceIndex >= m.Choices.size())
	{
		// Past end of choices.
		return E_INVALIDARG;
	}

	LineDataMap::iterator iLine;
	if (!e_FeatureLine_Find(nLineName, iLine))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}

	const DWORD nStopName = m.Choices[nChoiceIndex].nStopName;
	PRESULT nResult = iLine->second.pLineObj->SelectStop(nStopName, pState);
	if (SUCCEEDED(nResult))
	{
		LineStates[nLineName].nSelectedStop = nStopName;
	}
	return nResult;
}

#if 0
PRESULT FeatureLineMap::Maps::AdjustSelectedChoice(DWORD nLineName)
{
	LineStateData & lsd = LineStates[nLineName];
	if (lsd.nSelectedStop == 0)
	{
		// No selection yet.
		return S_OK;
	}

	// Retrieve stop indices.
	int nSelectedStopIndex, nMinStopIndex, nMaxStopIndex;
	V_RETURN(e_FeaturLine_GetStopIndexFromStopName(nLineName, lsd.nSelectedStop, nSelectedStopIndex));
	lsd.GetMinMaxStopIndex(nLineName, nMinStopIndex, nMaxStopIndex);

	// Get mapping.
	MappingMap::iterator i;
	if (!Find(nLineName, i))
	{
		// Line doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = i->second;
	ASSERT_RETVAL(!m.bMapping, E_FAIL);	// inside MapChoices()

	// Check lower bound.
	if (nSelectedStopIndex < nMinStopIndex)
	{
		// Too low.  Adjust upward to the lowest available stop.
		for (int i = nMinStopIndex; i <= nMaxStopIndex; ++i)
		{
			// Get stop's name.
			DWORD nStopName;
			V_RETURN(e_FeatureLine_EnumerateStopNames(nLineName, i, nStopName));
			// See if that choice is available.
			unsigned nChoiceIndex;
			if (SUCCEEDED(m.GetChoiceIndexFromStopName(nStopName, nChoiceIndex)))
			{
				return SelectChoice(nLineName, nChoiceIndex);
			}
		}
		// None found!
		return E_FAIL;
	}

	// Check upper bound.
	if (nSelectedStopIndex > nMaxStopIndex)
	{
		// Too high.  Adjust down to the highest available stop.
		for (int i = nMaxStopIndex; i >= nMinStopIndex; --i)
		{
			// Get stop's name.
			DWORD nStopName;
			V_RETURN(e_FeatureLine_EnumerateStopNames(nLineName, i, nStopName));
			// See if that choice is available.
			unsigned nChoiceIndex;
			if (SUCCEEDED(m.GetChoiceIndexFromStopName(nStopName, nChoiceIndex)))
			{
				return SelectChoice(nLineName, nChoiceIndex);
			}
		}
		// None found!
		return E_FAIL;
	}

	// Already in bounds.
	return S_OK;
}
#endif

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::GetMinimumAndMaximumStops(DWORD nLineName, DWORD & nMinStopNameOut, DWORD & nMaxStopNameOut) const
{
	int nMinStopIndex, nMaxStopIndex;
	pMaps->LineStates[nLineName].GetMinMaxStopIndex(nLineName, nMinStopIndex, nMaxStopIndex);
	V_RETURN(e_FeatureLine_EnumerateStopNames(nLineName, nMinStopIndex, nMinStopNameOut));
	V_RETURN(e_FeatureLine_EnumerateStopNames(nLineName, nMaxStopIndex, nMaxStopNameOut));
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::RemapAfterRangeRestriction(DWORD nLineName)
{
	// Restricting the stops can affect the choices available, so we'll
	// need to remap.  Before we do that though, remember the current
	// choice so we can preserve it if possible.
	DWORD nSelectedStop = pMaps->LineStates[nLineName].nSelectedStop;

	// Remap now -- rebuild the choice-to-stops mapping now that the
	// range has been restricted.
	V_RETURN(ReMap());	

	// Select a now-current choice close to our previous selection.
	if (nSelectedStop != 0)
	{
		V_RETURN(SelectChoiceFromStopFloor(nLineName, nSelectedStop));
	}

	// Made it!
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::AdjustMinimumStop(DWORD nLineName, DWORD nStopName)
{
	int nStopIndex, nMinStopIndex, nMaxStopIndex;
	V_RETURN(e_FeaturLine_GetStopIndexFromStopName(nLineName, nStopName, nStopIndex));
	pMaps->LineStates[nLineName].GetMinMaxStopIndex(nLineName, nMinStopIndex, nMaxStopIndex);
	if (nStopIndex > nMinStopIndex)
	{
		ASSERT(nStopIndex <= nMaxStopIndex);	// attempt to set a minimum greater than the current maximum
		pMaps->LineStates[nLineName].nMinimumStopIndex = MIN(nStopIndex, nMaxStopIndex);
		return RemapAfterRangeRestriction(nLineName);
	}
	// No adjustment made.
	return S_FALSE;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::AdjustMaximumStop(DWORD nLineName, DWORD nStopName)
{
	int nStopIndex, nMinStopIndex, nMaxStopIndex;
	V_RETURN(e_FeaturLine_GetStopIndexFromStopName(nLineName, nStopName, nStopIndex));
	pMaps->LineStates[nLineName].GetMinMaxStopIndex(nLineName, nMinStopIndex, nMaxStopIndex);
	if (nStopIndex < nMaxStopIndex)
	{
		ASSERT(nStopIndex >= nMinStopIndex);	// attempt to set a maximum less than the current minimum
		pMaps->LineStates[nLineName].nMaximumStopIndex = MAX(nStopIndex, nMinStopIndex);
		return RemapAfterRangeRestriction(nLineName);
	}
	// No adjustment made.
	return S_FALSE;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::AddChoice(DWORD nLineName, DWORD nStopName, const char * pInternalChoiceName, GLOBAL_STRING eGlobalStr )
{
	const wchar_t * pDisplayChoiceName = GlobalStringGet( eGlobalStr );

	Maps::MappingMap::iterator i;
	if (!pMaps->Find(nLineName, i))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = i->second;

	ASSERT_RETVAL(m.bMapping, E_FAIL);	// not inside MapChoices()

	// Ensure all stops added are within the allowed range.
	{
		int nStopIndex, nMinStopIndex, nMaxStopIndex;
		V_RETURN(e_FeaturLine_GetStopIndexFromStopName(nLineName, nStopName, nStopIndex));
		pMaps->LineStates[nLineName].GetMinMaxStopIndex(nLineName, nMinStopIndex, nMaxStopIndex);
		if ((nStopIndex < nMinStopIndex) || (nStopIndex > nMaxStopIndex))
		{
			// Not an error, we'll just ignore it.
			return S_FALSE;
		}
	}

	//
	unsigned nIndex;

	// Check for duplicate stop name.
	// This prevents two choices from mapping to the same stop.
	// This may or may not be desirable.
	if (SUCCEEDED(m.GetChoiceIndexFromStopName(nStopName, nIndex)))
	{
		return E_ALREADY_REGISTERED;
	}

	// Check for duplicate internal name.
	if (SUCCEEDED(m.GetChoiceIndexFromInternalName(pInternalChoiceName, nIndex)))
	{
		return E_ALREADY_REGISTERED;
	}

	// Check for duplicate display name.
	if (SUCCEEDED(m.GetChoiceIndexFromDisplayName(pDisplayChoiceName, nIndex)))
	{
		return E_ALREADY_REGISTERED;
	}

	// Choices MUST be in order of decreasing stops.
	if (!m.Choices.empty())
	{
		if (e_FeatureLine_IsStopLessThan(nLineName, nStopName, m.Choices.back().nStopName) != S_OK)
		{
			char szLine[5], szStop[5], szPrev[5];
			FOURCCTOSTR( nLineName, szLine );
			FOURCCTOSTR( nStopName, szStop );
			FOURCCTOSTR( m.Choices.back().nStopName, szPrev );
			ASSERTV_MSG("Featureline options MUST be in order of decreasing stops!\nLine: %s   Stop: %s   Prev: %s", szLine, szStop, szPrev );
			return E_FAIL;
		}
	}

	// Okay to add.
	ChoiceData cd;
	cd.nStopName = nStopName;
	cd.InternalName = pInternalChoiceName;
	cd.DisplayName = pDisplayChoiceName;
	m.Choices.push_back(cd);
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::EnumerateChoiceInternalNames(DWORD nLineName, unsigned nChoiceIndex, char * pBufferOut, unsigned & nBufferChars) const
{
	Maps::MappingMap::iterator i;
	if (!pMaps->Find(nLineName, i))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = i->second;

	ASSERT_RETVAL(!m.bMapping, E_FAIL);	// inside MapChoices()

	if (nChoiceIndex >= m.Choices.size())
	{
		// Past end of choices.
		return E_NO_MORE_ITEMS;
	}
	if (m.Choices[nChoiceIndex].InternalName.size() >= nBufferChars)
	{
		nBufferChars = static_cast<unsigned>(m.Choices[nChoiceIndex].InternalName.size() + 1);
		return E_INSUFFICIENT_BUFFER;
	}
	PStrCopy(pBufferOut, m.Choices[nChoiceIndex].InternalName.c_str(), nBufferChars);
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::EnumerateChoiceDisplayNames(DWORD nLineName, unsigned nChoiceIndex, wchar_t * pBufferOut, unsigned & nBufferChars) const
{
	Maps::MappingMap::iterator i;
	if (!pMaps->Find(nLineName, i))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = i->second;

	ASSERT_RETVAL(!m.bMapping, E_FAIL);	// inside MapChoices()

	if (nChoiceIndex >= m.Choices.size())
	{
		// Past end of choices.
		return E_NO_MORE_ITEMS;
	}
	if (m.Choices[nChoiceIndex].DisplayName.size() >= nBufferChars)
	{
		nBufferChars = static_cast<unsigned>(m.Choices[nChoiceIndex].DisplayName.size() + 1);
		return E_INSUFFICIENT_BUFFER;
	}
	PStrCopy(pBufferOut, m.Choices[nChoiceIndex].DisplayName.c_str(), nBufferChars);
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::SelectChoice(DWORD nLineName, unsigned nChoiceIndex)
{
	return pMaps->SelectChoice(nLineName, nChoiceIndex);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::SelectChoice(DWORD nLineName, const char * pInternalChoiceName)
{
	Maps::MappingMap::iterator iMap;
	if (!pMaps->Find(nLineName, iMap))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = iMap->second;
	ASSERT_RETVAL(!m.bMapping, E_FAIL);	// inside MapChoices()

	unsigned nIndex;
	PRESULT nResult = m.GetChoiceIndexFromInternalName(pInternalChoiceName, nIndex);
	if (FAILED(nResult))
	{
		return nResult;
	}

	return SelectChoice(nLineName, nIndex);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::SelectPoint(DWORD nLineName, float fPoint)
{
	// Check point given.
	ASSERT_RETVAL((0.0f <= fPoint) && (fPoint <= 1.0f), E_INVALIDARG);

	// Get mapping object.
	Maps::MappingMap::iterator iMap;
	if (!pMaps->Find(nLineName, iMap))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = iMap->second;
	ASSERT_RETVAL(!m.bMapping, E_FAIL);	// inside MapChoices()

	// Make sure at least one choice.
	int nChoices = static_cast<int>(m.Choices.size());
	ASSERT_RETVAL(nChoices > 0, E_FAIL);

	// Get integer choice index and clamp to valid range.
	int nChoice = static_cast<int>(nChoices * (1.0f - fPoint));
	nChoice = max(nChoice, 0);
	nChoice = min(nChoice, nChoices - 1);

	// Select it.
	return SelectChoice(nLineName, nChoice);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::SelectChoiceFromStopFloor(DWORD nLineName, DWORD nStopName)
{
	// Get mapping object.
	Maps::MappingMap::iterator iMap;
	if (!pMaps->Find(nLineName, iMap))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = iMap->second;
	ASSERT_RETVAL(!m.bMapping, E_FAIL);	// inside MapChoices()

	// Go over the choices and find the highest one that does not
	// exceed the desired stop.  If the desired stop is already
	// below the minimum, select the lowest choice.
	// Note that choices are in order of decreasing stop position.
	unsigned nBestChoice = 0;
	if (m.Choices.size() > 0)
	{
		nBestChoice = static_cast<unsigned>(m.Choices.size() - 1);	// default to the lowest stop
		for (unsigned nChoiceIndex = 0; nChoiceIndex < m.Choices.size(); ++nChoiceIndex)
		{
			PRESULT nResult = e_FeatureLine_IsStopLessThan(nLineName, nStopName, m.Choices[nChoiceIndex].nStopName);
			V(nResult);
			if (nResult == S_FALSE)
			{
				nBestChoice = nChoiceIndex;
				break;
			}
		}
	}

	// Select it.
	return SelectChoice(nLineName, nBestChoice);
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::GetSelectedStopName(DWORD nLineName, DWORD & nStopNameOut) const
{
	LineStateDataMap::const_iterator iState = pMaps->LineStates.find(nLineName);
	if (iState == pMaps->LineStates.end())
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	nStopNameOut = iState->second.nSelectedStop;
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::GetSelectedChoice(DWORD nLineName, unsigned & nChoiceIndexOut) const
{
	// First, find out which stop is selected.
	DWORD nSelectedStop = 0;
	V_RETURN(GetSelectedStopName(nLineName, nSelectedStop));

	// Second, figure out which choice correlates to this stop.
	Maps::MappingMap::iterator iMap;
	if (!pMaps->Find(nLineName, iMap))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	Mapping & m = iMap->second;

	unsigned nIndex;
	PRESULT nResult = m.GetChoiceIndexFromStopName(nSelectedStop, nIndex);
	if (FAILED(nResult))
	{
		return nResult;
	}

	nChoiceIndexOut = nIndex;
	return S_OK;
}

//-----------------------------------------------------------------------------

void FeatureLineMap::Exchange(SettingsExchange & se)
{
	for (Maps::MappingMap::iterator iMap = pMaps->Mappings.begin(); iMap != pMaps->Mappings.end(); ++iMap)
	{
		const DWORD nLineName = iMap->first;

		char szLineName[8];
		*static_cast<DWORD *>(static_cast<void *>(szLineName)) = nLineName;
		szLineName[4] = '\0';
		char szInternalName[32];
		szInternalName[0] = '\0';

		if (se.IsSaving())
		{
			unsigned nChoiceIndex;
			V_CONTINUE(GetSelectedChoice(nLineName, nChoiceIndex));
			unsigned nChars = _countof(szInternalName);
			V_CONTINUE(EnumerateChoiceInternalNames(nLineName, nChoiceIndex, szInternalName, nChars));
			SettingsExchange_Do(se, szInternalName, _countof(szInternalName), szLineName);
		}
		else
		{
			SettingsExchange_Do(se, szInternalName, _countof(szInternalName), szLineName);
			if (szInternalName[0] != '\0')
			{
				// Silently ignore errors; an option we selected
				// earlier may no longer be available.
				SelectChoice(nLineName, szInternalName);
			}
		}
	}
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::Clone(FeatureLineMap * * ppMapOut) const
{
	// Create & initialize a new map object.
	CComPtr<FeatureLineMap> pMap = new FeatureLineMap;
	*pMap->pMaps = *pMaps;
	pMap->pMaps->pState = 0;
	V_RETURN(e_OptionState_Clone(pMaps->pState, &pMap->pMaps->pState));

	// Do the mapping.
	V_RETURN(pMap->ReMap());

	// Return it to the caller.
	*ppMapOut = pMap;
	pMap.p->AddRef();
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::GetState(OptionState * * ppStateOut) const
{
	ASSERT_RETVAL(ppStateOut != 0, E_INVALIDARG);
	*ppStateOut = pMaps->pState;
	pMaps->pState.p->AddRef();
	return S_OK;
}

//-----------------------------------------------------------------------------

static
void e_FeatureLineMap_Exchange(SettingsExchange & se, void * pContext)
{
	FeatureLineMap * pMap = static_cast<FeatureLineMap *>(pContext);
	pMap->Exchange(se);
}

PRESULT FeatureLineMap::LoadFromFile(const OS_PATH_CHAR * pFileName, const char * pSectionName)
{
	V_RETURN(SettingsExchange_LoadFromFile(pFileName, pSectionName, &e_FeatureLineMap_Exchange, this));
	return S_OK;
}

PRESULT FeatureLineMap::SaveToFile(const OS_PATH_CHAR * pFileName, const char * pSectionName) const
{
	V_RETURN(SettingsExchange_SaveToFile(pFileName, pSectionName, &e_FeatureLineMap_Exchange, const_cast<FeatureLineMap *>(this)));
	return S_OK;
}

//-----------------------------------------------------------------------------

FeatureLineMap::FeatureLineMap(void)
	: pMaps(new Maps)
{
}

FeatureLineMap::~FeatureLineMap(void)
{
	delete pMaps;
}


//-----------------------------------------------------------------------------
//
// Public functions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

PRESULT FeatureLine::e_FeatureLine_AddChoice(FeatureLineMap * pMap, DWORD nLineName, DWORD nStopName, const char * pInternalChoiceName, GLOBAL_STRING eGlobalStr )
{
	ASSERT_RETVAL(pMap != 0, E_INVALIDARG);
	return pMap->AddChoice(nLineName, nStopName, pInternalChoiceName, eGlobalStr);
}

//-----------------------------------------------------------------------------

FeatureLine::~FeatureLine(void)
{
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_IsStopLessThan(DWORD nLineName, DWORD nStopLHS, DWORD nStopRHS)
{
	int nIndexLHS, nIndexRHS;
	PRESULT nResult;

	nResult = e_FeaturLine_GetStopIndexFromStopName(nLineName, nStopLHS, nIndexLHS);
	if (FAILED(nResult))
	{
		return nResult;
	}

	nResult = e_FeaturLine_GetStopIndexFromStopName(nLineName, nStopRHS, nIndexRHS);
	if (FAILED(nResult))
	{
		return nResult;
	}

	return (nIndexLHS < nIndexRHS) ? S_OK : S_FALSE;
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_CommitToActive(const FeatureLineMap * pNewState)
{
	s_pModule->LineStates = pNewState->pMaps->LineStates;
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT FeatureLineMap::ReMap(void)
{
	ASSERT_RETFAIL( pMaps );
	pMaps->Mappings.clear();

	for (LineDataMap::const_iterator i = s_pModule->Lines.begin(); i != s_pModule->Lines.end(); ++i)
	{
		// Init mapping object.
		Mapping & m = pMaps->Mappings[i->first];

		// Set flag.
		m.bMapping = true;

		// Do the mapping.
		V_RETURN(i->second.pLineObj->MapChoices(this, i->first, pMaps->pState));

		// Clear flag.
		m.bMapping = false;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_OpenMap(OptionState * pState, FeatureLineMap * * pMapOut)
{
	// Create & initialize a new map object.
	CComPtr<FeatureLineMap> pMap = new FeatureLineMap;
	pMap->pMaps->pState = pState;
	pMap->pMaps->LineStates = s_pModule->LineStates;

	// Do the mapping.
	V_RETURN(pMap->ReMap());

	// Return it to the caller.
	*pMapOut = pMap;
	pMap.p->AddRef();
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Register(FeatureLine * pLine, DWORD nName)
{
	// See if one of this name already exists.
	LineDataMap::iterator i;
	if (e_FeatureLine_Find(nName, i))
	{
		// Already exists.
		return E_ALREADY_REGISTERED;
	}

	// Add it to the list.
	s_pModule->Lines.insert(std::make_pair(nName, LineData(pLine)));

	// Okay.
	return S_OK;
}

//-----------------------------------------------------------------------------

bool e_FeatureLine_EnumerateLines(unsigned nIndex, DWORD & nNameOut)
{
	if (nIndex >= s_pModule->Lines.size())
	{
		return false;
	}
	LineDataMap::const_iterator i = s_pModule->Lines.begin();
	while (nIndex > 0)
	{
		if (i == s_pModule->Lines.end())
		{
			return false;
		}
		++i;
		--nIndex;
	}
	if (i == s_pModule->Lines.end())
	{
		return false;
	}
	nNameOut = i->first;
	return true;
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_EnumerateStopNames(DWORD nLineName, unsigned nIndex, DWORD & nNameOut)
{
	LineDataMap::iterator i;
	if (!e_FeatureLine_Find(nLineName, i))
	{
		// Doesn't exist.
		return E_NOT_FOUND;
	}
	return i->second.pLineObj->EnumerateStops(nIndex, nNameOut) ? S_OK : E_NO_MORE_ITEMS;
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_GetSelectedStopName( DWORD nLineName, FeatureLineMap * pMap, DWORD & nNameOut)
{
	ASSERT_RETINVALIDARG( pMap );
	return pMap->GetSelectedStopName(nLineName, nNameOut);
}

PRESULT e_FeatureLine_GetSelectedStopInternalName( DWORD nLineName, FeatureLineMap * pMap, char * pszInternalName, unsigned int nBufLen )
{
	ASSERT_RETINVALIDARG( pMap );
	unsigned nChoiceIndex;
	V_RETURN(pMap->GetSelectedChoice(nLineName, nChoiceIndex));
	return pMap->EnumerateChoiceInternalNames(nLineName, nChoiceIndex, pszInternalName, nBufLen);
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Init(void)
{
	delete s_pModule;
	s_pModule = new Module;
	return S_OK;
}

//-----------------------------------------------------------------------------

PRESULT e_FeatureLine_Deinit(void)
{
	delete s_pModule;
	s_pModule = 0;
	return S_OK;
}
