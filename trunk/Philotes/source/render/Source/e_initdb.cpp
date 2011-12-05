//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "appcommon.h"
#include "e_initdb.h"
#include "e_optionstate.h"
#include "e_featureline.h"
#include "e_caps.h"
#include "dxC_caps.h"
#include "datatables.h"
#include <map>
#include "memoryallocator_stl.h"
#include "debugtextlog.h"
#include "filetextlog.h"
#include "filepaths.h"
#include "stringhashkey.h"
#include <string>


using namespace std;
using namespace FSCommon;

//-----------------------------------------------------------------------------

#ifdef _DEBUG
#define DEBUG_ASSERT(e) ASSERT(e)
#else
#define DEBUG_ASSERT(e) ((void)0)
#endif

//-----------------------------------------------------------------------------

#if E_INITDB_DEVELOPMENT
#define E_INITDB_ENABLE_WARNINGS	1	// set to 1 to enable run-time warnings about database issues
#define E_INITDB_ENABLE_AUDIT		1	// set to 1 to enable run-time diagnostics output to debug
#endif

//-----------------------------------------------------------------------------

namespace
{

//-----------------------------------------------------------------------------

// Holds the values of the various criteria.
// If this starts to get large, could break off a separate e_initdb_def.h file.
typedef INITDB_TABLE::CRITERIA_TYPE CRITERIA_TYPE;
struct Criteria
{
	CRITERIA_TYPE nCPUSpeedInMHz;
	CRITERIA_TYPE nSystemRAMInMB;
	CRITERIA_TYPE nPhysVideoRAMInMB;
	CRITERIA_TYPE nTotalVideoRAMInMB;
	CRITERIA_TYPE nPixelShaderModel;
	CRITERIA_TYPE nOSVersion;
	CRITERIA_TYPE nVertexShaderPower;
	CRITERIA_TYPE nPixelShaderPower;
	CRITERIA_TYPE nSysBusSpeedInMHz;
	CRITERIA_TYPE nOSPlatform;
};

//-----------------------------------------------------------------------------

// Used to map FourCC names to struct member.
struct CriteriaInfo
{
	DWORD nName;
	unsigned nStructIndex;
#if E_INITDB_DEVELOPMENT
	const char * pDescription;
#endif
};
#if E_INITDB_DEVELOPMENT
#define DEF_STR(s) , s
#else
#define DEF_STR(s)
#endif
#define DEF_CRITERIA(n, m, s) { E_FEATURELINE_FOURCC(#n), OFFSET(Criteria, m) / sizeof(CRITERIA_TYPE) DEF_STR(s) },
const CriteriaInfo s_tCriteria[] =
{
	DEF_CRITERIA(	CPUS,	nCPUSpeedInMHz,		"CPU speed (MHz)"			)
	DEF_CRITERIA(	SYSM,	nSystemRAMInMB,		"system RAM (MB)"			)
	DEF_CRITERIA(	PHVM,	nPhysVideoRAMInMB,	"physical video RAM (MB)"	)
	DEF_CRITERIA(	TOVM,	nTotalVideoRAMInMB, "total video RAM (MB)"		)
	DEF_CRITERIA(	PSHD,	nPixelShaderModel,	"pixel shader model"		)
	DEF_CRITERIA(	OSVN,	nOSVersion,			"operating system"			)
	DEF_CRITERIA(	VSPW,	nVertexShaderPower,	"vertex shader power"		)
	DEF_CRITERIA(	PSPW,	nPixelShaderPower,	"pixel shader power"		)
	DEF_CRITERIA(	CLCK,	nSysBusSpeedInMHz,	"system bus speed (MHz)"	)
	DEF_CRITERIA(	OSPF,	nOSPlatform,		"operating system platform"	)
};
#undef DEF_CRITERIA
#undef DEF_STR

//-----------------------------------------------------------------------------

// Used to track initialization information.
struct InitInfo
{
	// Line name, selected init
	typedef map<DWORD, DWORD, less<DWORD>, CMemoryAllocatorSTL<pair<DWORD, DWORD> > > FeatureInitMap;
	FeatureInitMap FeatureInits;

	// State name, value
	typedef map<std::string, float, less<std::string>, CMemoryAllocatorSTL<pair<std::string, float> > > NumericInitMap;
	NumericInitMap NumericInits;
};

//-----------------------------------------------------------------------------

#if E_INITDB_DEVELOPMENT
struct InitDBModule
{
	TextLogA * pLog;
	bool bAudit;
	bool bWarn;
	bool bDumpToFile;
	bool bIgnoreDB;

	InitDBModule(void);
	~InitDBModule(void);
};
InitDBModule s_tModule;
#endif

//-----------------------------------------------------------------------------

#if E_INITDB_DEVELOPMENT
InitDBModule::InitDBModule(void)
{
	pLog = 0;
	bAudit = true;
	bWarn = true;
	bDumpToFile = false;
	bIgnoreDB = false;
}
#endif

#if E_INITDB_DEVELOPMENT
InitDBModule::~InitDBModule(void)
{
	delete pLog;
}
#endif

//-----------------------------------------------------------------------------

void e_InitDB_AuditV(const char * pFmt, va_list args)
{
#if E_INITDB_ENABLE_AUDIT
	if ((s_tModule.pLog != 0) && s_tModule.bAudit)
	{
		s_tModule.pLog->WriteVA(pFmt, args);
	}
#else
	REF(pFmt);
	REF(args);
#endif
}

void e_InitDB_Audit(const char * pFmt, ...)
{
#if E_INITDB_ENABLE_AUDIT
	va_list args;
	va_start(args, pFmt);
	e_InitDB_AuditV(pFmt, args);
#else
	REF(pFmt);
#endif
}

//-----------------------------------------------------------------------------

void e_InitDB_Warn(int nRow, const char * pFmt, ...)
{
	va_list args;
	va_start(args, pFmt);
	e_InitDB_AuditV(pFmt, args);
	e_InitDB_Audit("\n");
#if E_INITDB_ENABLE_WARNINGS
	if (s_tModule.bWarn)
	{
		char Title[64];
		PStrPrintf(Title, _countof(Title), "initdb.xls row %d", nRow);
		char Msg[256];
		PStrVprintf(Msg, _countof(Msg), pFmt, args);
		DoWarn(
			Title,
			Msg,
			"initdb.xls",
			nRow,
			"" );
		//::MessageBoxA(	// CHB 2007.08.01 - String audit: commented
		//	AppCommonGetHWnd(),
		//	Msg,
		//	Title,
		//	MB_ICONWARNING | MB_OK
		//);
	}
#else
	REF(nRow);
#endif
}

//-----------------------------------------------------------------------------

#if E_INITDB_DEVELOPMENT
struct FourCCString
{
	FourCCString(DWORD n)
	{
		FOURCCTOSTR(n, s);
	}
	const char * c_str(void) const
	{
		return s;
	}
	operator const char *(void) const
	{
		return c_str();
	}
	char s[6];
};
#define _FCCS(n) FourCCString(n).c_str()
#else
#define _FCCS(n) 0
#endif

//-----------------------------------------------------------------------------

bool e_InitDB_IsValidCriteria(DWORD nName)
{
	for (unsigned i = 0; i < _countof(s_tCriteria); ++i)
	{
		if (nName == s_tCriteria[i].nName)
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

bool sHasOne(const INITDB_TABLE * pRow, bool bFeatKnob)
{
	if (bFeatKnob)
	{
		return
			(pRow->nFeatMin != INITDB_TABLE::INVALID_FOURCC) ||
			(pRow->nFeatMax != INITDB_TABLE::INVALID_FOURCC) ||
			(pRow->nFeatInit != INITDB_TABLE::INVALID_FOURCC);
	}
	else
	{
		return
			(!Math_IsNaN(pRow->fNumMin)) ||
			(!Math_IsNaN(pRow->fNumMax)) ||
			(!Math_IsNaN(pRow->fNumInit));
	}
}

bool e_InitDB_IsRowValid(const INITDB_TABLE * pRow, int nRow)
{
	// Check for valid criteria.
	if (!e_InitDB_IsValidCriteria(pRow->nCriteria))
	{
		e_InitDB_Warn(nRow, "invalid criteria field value");
		return false;
	}

	// Make sure at least one of the range values is defined.
	if ((pRow->nRangeLow == INITDB_TABLE::INVALID_RANGE) && (pRow->nRangeHigh == INITDB_TABLE::INVALID_RANGE))
	{
		e_InitDB_Warn(nRow, "at least one criteria range value must be defined");
		return false;
	}

	// Check for a knob.
	if ((pRow->szNumKnob[0] == '\0') && (pRow->nFeatKnob == INITDB_TABLE::INVALID_FOURCC))
	{
		e_InitDB_Warn(nRow, "exactly one knob must be defined, but none is");
		return false;
	}

	// Check for too many knobs.
	if ((pRow->szNumKnob[0] != '\0') && (pRow->nFeatKnob != INITDB_TABLE::INVALID_FOURCC))
	{
		e_InitDB_Warn(nRow, "exactly one knob must be defined, but both are");
		return false;
	}

	// Which is the defined knob?
	bool bFeatKnob = (pRow->nFeatKnob != INITDB_TABLE::INVALID_FOURCC);
	bool bThisFilled = sHasOne(pRow, bFeatKnob);
	bool bOtherFilled = sHasOne(pRow, !bFeatKnob);

	// Make sure the values for the undefined knobs are blank.
	if (bOtherFilled)
	{
		e_InitDB_Warn(nRow, "one or more values for the unused knob are not blank");
		return false;
	}

	// Make sure our knob has at least one defined.
	if (!bThisFilled)
	{
		e_InitDB_Warn(nRow, "at least one of min, max, and init must be defined");
		return false;
	}

	// Passed.
	return true;
}

//-----------------------------------------------------------------------------

bool e_InitDB_TestCriteria(DWORD nName, CRITERIA_TYPE nMin, CRITERIA_TYPE nMax, const Criteria & Specs)
{
	// Find the criteria.
	for (unsigned i = 0; i < _countof(s_tCriteria); ++i)
	{
		if (nName == s_tCriteria[i].nName)
		{
			// Found it.
			// Get the index.
			const unsigned nIndex = s_tCriteria[i].nStructIndex;
			// Treat struct as an array of CRITERIA_TYPE.
			const CRITERIA_TYPE * pArray = static_cast<const CRITERIA_TYPE *>(static_cast<const void *>(&Specs));
			// Get value.
			DEBUG_ASSERT(nIndex * sizeof(CRITERIA_TYPE) < sizeof(Specs));
			const CRITERIA_TYPE nVal = pArray[nIndex];
			// Check minimum.
			if ((nMin != INITDB_TABLE::INVALID_RANGE) && (nVal < nMin))
			{
				e_InitDB_Audit("miss  %s  %4d below min %4d", _FCCS(nName), nVal, nMin);
				return false;
			}
			// Check maximum.
			if ((nMax != INITDB_TABLE::INVALID_RANGE) && (nVal > nMax))
			{
				e_InitDB_Audit("miss  %s  %4d above max %4d", _FCCS(nName), nVal, nMax);
				return false;
			}
			// Match!
			e_InitDB_Audit("hit   %s  ", _FCCS(nName));
			if (nMin != INITDB_TABLE::INVALID_RANGE)
			{
				e_InitDB_Audit("%4d <= ", nMin);
			}
			else
			{
				e_InitDB_Audit("        ");
			}
			e_InitDB_Audit("%4d", nVal);
			if (nMax != INITDB_TABLE::INVALID_RANGE)
			{
				e_InitDB_Audit(" <= %4d", nMax);
			}
			else
			{
				e_InitDB_Audit("        ");
			}
			return true;
		}
	}
	DEBUG_ASSERT(false);	// This shouldn't be possible, because e_InitDB_IsRowValid() should have picked up on it.
	return false;
}

//-----------------------------------------------------------------------------

void e_InitDB_TakeActionNumeric(const INITDB_TABLE * pRow, int nRow, FeatureLineMap * pMap, OptionState * pState, InitInfo & tInits)
{
	REF(pMap);

	// Check for existence of this numeric knob name.
	if (!pState->HasValueNamed(pRow->szNumKnob))
	{
		e_InitDB_Warn(nRow, "unrecognized numeric knob; see e_optionstate_def.h");
		return;
	}
	e_InitDB_Audit("  %s", pRow->szNumKnob);

	// Get min & max.
	float fCurrentMin = 0.0f;
	DEBUG_ASSERT(pState->GetMinByNameAuto(pRow->szNumKnob, fCurrentMin));
	float fCurrentMax = 0.0f;
	DEBUG_ASSERT(pState->GetMaxByNameAuto(pRow->szNumKnob, fCurrentMax));

	// Setting a minimum?
	if (!Math_IsNaN(pRow->fNumMin))
	{
		float fNewMin = MAX(pRow->fNumMin, fCurrentMin);	// only allow minimum to become more restrictive
		fNewMin = MIN(fNewMin, fCurrentMax);				// don't allow min to go past max
		DEBUG_ASSERT(pState->SetMinByNameAuto(pRow->szNumKnob, fNewMin));
		e_InitDB_Audit("  %4.0f", fNewMin);
		fCurrentMin = fNewMin;
	}
	else
	{
		e_InitDB_Audit("      ");
	}

	// Setting a maximum?
	if (!Math_IsNaN(pRow->fNumMax))
	{
		float fNewMax = MIN(pRow->fNumMax, fCurrentMax);	// only allow maximum to become more restrictive
		fNewMax = MAX(fNewMax, fCurrentMin);				// don't allow max to go past min
		DEBUG_ASSERT(pState->SetMaxByNameAuto(pRow->szNumKnob, fNewMax));
		e_InitDB_Audit("  %4.0f", fNewMax);
		fCurrentMax = fNewMax;
	}
	else
	{
		e_InitDB_Audit("      ");
	}

	// Setting an initial?
	if (!Math_IsNaN(pRow->fNumInit))
	{
		std::string LineName(pRow->szNumKnob);
		float fNewVal = PIN(pRow->fNumInit, fCurrentMin, fCurrentMax);
		bool bKeep = true;

		// Does an initialization entry exist for this yet?
		InitInfo::NumericInitMap::iterator iInit = tInits.NumericInits.find(LineName);
		if (iInit != tInits.NumericInits.end())
		{
			// One exists. Choose the lesser of the two.
			bKeep = fNewVal < tInits.NumericInits[LineName];
		}

		// Save it.
		if (bKeep)
		{
			tInits.NumericInits[LineName] = pRow->fNumInit;
			e_InitDB_Audit("  %4.0f", fNewVal);
		}
		else
		{
			e_InitDB_Audit("  %4.0f <ignored>", fNewVal);
		}
	}
}

//-----------------------------------------------------------------------------

void e_InitDB_TakeActionFeature(const INITDB_TABLE * pRow, int nRow, FeatureLineMap * pMap, OptionState * pState, InitInfo & tInits)
{
	DWORD nLineName = pRow->nFeatKnob;
	e_InitDB_Audit("  %s", _FCCS(nLineName));

	// Setting a minimum?
	if (pRow->nFeatMin != INITDB_TABLE::INVALID_FOURCC)
	{
		V_DO_FAILED(pMap->AdjustMinimumStop(nLineName, pRow->nFeatMin))
		{
			e_InitDB_Warn(nRow, "problem setting feature minimum");
		}
		e_InitDB_Audit("  %s", _FCCS(pRow->nFeatMin));
	}
	else
	{
		e_InitDB_Audit("      ");
	}

	// Setting a maximum?
	if (pRow->nFeatMax != INITDB_TABLE::INVALID_FOURCC)
	{
		V_DO_FAILED(pMap->AdjustMaximumStop(nLineName, pRow->nFeatMax))
		{
			e_InitDB_Warn(nRow, "problem setting feature maximum");
		}
		e_InitDB_Audit("  %s", _FCCS(pRow->nFeatMax));
	}
	else
	{
		e_InitDB_Audit("      ");
	}

	// Setting an initial?
	if (pRow->nFeatInit != INITDB_TABLE::INVALID_FOURCC)
	{
		bool bKeep = true;

		// Does an initialization entry exist for this yet?
		InitInfo::FeatureInitMap::iterator iInit = tInits.FeatureInits.find(nLineName);
		if (iInit != tInits.FeatureInits.end())
		{
			// One exists. Choose the lesser of the two options.
			PRESULT nResult = e_FeatureLine_IsStopLessThan(nLineName, pRow->nFeatInit, iInit->second);
			bKeep = (nResult == S_OK);
			V_DO_FAILED(nResult)
			{
				e_InitDB_Warn(nRow, "problem setting feature initial");
			}
		}

		// Save it.
		if (bKeep)
		{
			tInits.FeatureInits[nLineName] = pRow->nFeatInit;
			e_InitDB_Audit("  %s", _FCCS(pRow->nFeatInit));
		}
		else
		{
			e_InitDB_Audit("  <ignored>");
		}
	}
}

//-----------------------------------------------------------------------------

void e_InitDB_TakeAction(const INITDB_TABLE * pRow, int nRow, FeatureLineMap * pMap, OptionState * pState, InitInfo & tInits)
{
	bool bFeatKnob = (pRow->nFeatKnob != INITDB_TABLE::INVALID_FOURCC);
	if (bFeatKnob)
	{
		e_InitDB_TakeActionFeature(pRow, nRow, pMap, pState, tInits);
	}
	else
	{
		e_InitDB_TakeActionNumeric(pRow, nRow, pMap, pState, tInits);
	}
}

//-----------------------------------------------------------------------------

const Criteria e_InitDB_GetSystemSpecs(void)
{
	Criteria Specs;

	Specs.nCPUSpeedInMHz = e_CapGetValue(CAP_CPU_SPEED_MHZ);
	Specs.nSystemRAMInMB = e_CapGetValue(CAP_SYSTEM_MEMORY_MB);
	Specs.nPhysVideoRAMInMB = e_CapGetValue(CAP_PHYSICAL_VIDEO_MEMORY_MB);
	Specs.nTotalVideoRAMInMB = e_CapGetValue(CAP_TOTAL_ESTIMATED_VIDEO_MEMORY_MB);
	Specs.nSysBusSpeedInMHz = e_CapGetValue(CAP_SYSTEM_CLOCK_SPEED_MHZ);

	// Use shader versions without the decimal point.  For example,
	// shader model 1.1 is '11', and 2.0 is '20'.
	DWORD nPSVer = dx9_CapsGetAugmentedMaxPixelShaderVersion();	// CHB 2007.07.25
	Specs.nPixelShaderModel = D3DSHADER_VERSION_MAJOR(nPSVer) * 10 + D3DSHADER_VERSION_MINOR(nPSVer);

	if ( WindowsIsXP() )
	{
		Specs.nOSVersion = OS_XP_ID;
	}
	else if ( WindowsIsVistaOrLater() )
	{
		Specs.nOSVersion = OS_VISTA_ID;
	}
	else
	{
		Specs.nOSVersion = 0;
	}

	Specs.nVertexShaderPower = dx9_CapGetValue(DX9CAP_VS_POWER);
	Specs.nPixelShaderPower  = dx9_CapGetValue(DX9CAP_PS_POWER);

#ifdef _WIN64
	Specs.nOSPlatform = 64;
#else
	Specs.nOSPlatform = 32;
#endif

	return Specs;
}

//-----------------------------------------------------------------------------

void e_InitDB_Audit(const Criteria & Specs)
{
#if E_INITDB_DEVELOPMENT
	e_InitDB_Audit("\nCurrent system specifications:\n");
	for (unsigned i = 0; i < _countof(s_tCriteria); ++i)
	{
		// Get the index.
		const unsigned nIndex = s_tCriteria[i].nStructIndex;
		// Treat struct as an array of CRITERIA_TYPE.
		const CRITERIA_TYPE * pArray = static_cast<const CRITERIA_TYPE *>(static_cast<const void *>(&Specs));
		// Dump it.
		e_InitDB_Audit("%s = %5u [%s]\n", _FCCS(s_tCriteria[i].nName), pArray[nIndex], s_tCriteria[i].pDescription);
	}
#else
	REF(Specs);
#endif
}

//-----------------------------------------------------------------------------

void e_InitDB_Apply(FeatureLineMap * pMap, OptionState * pState)
{
	// Fill in criteria.
	const Criteria ThisSystem = e_InitDB_GetSystemSpecs();
	e_InitDB_Audit(ThisSystem);

	// Initialization info.
	InitInfo tInits;

	// Audit header & initialize.
	e_InitDB_Audit("\nLine trace:\n");
#if E_INITDB_DEVELOPMENT
	if (s_tModule.bIgnoreDB)
	{
		e_InitDB_Audit("  (database ignored)\n");
		return;
	}
#endif
	e_InitDB_Audit("Row   Actn  Spec  Criteria              Feat  Min   Max   Init\n");
	e_InitDB_Audit("----  ----  ----  --------------------  ----  ----  ----  ----\n");
	const DWORD nTable = DATATABLE_INITDB;
	// nRows and nRow are in terms of row values as displayed in Excel.
	int nRows = ExcelGetCount(EXCEL_CONTEXT(), nTable) + 1;
	// Start with row 5 (the rest are headers).
	for (int nRow = 5; nRow <= nRows; ++nRow)
	{
		// -1 for header, -1 for 0-based indexing
		const INITDB_TABLE * pRow = static_cast<const INITDB_TABLE *>(ExcelGetData(EXCEL_CONTEXT(), nTable, nRow - 2));
		e_InitDB_Audit("%4d  ", nRow);

		// Check to see if the rule is valid.
		if (!e_InitDB_IsRowValid(pRow, nRow))
		{
			continue;
		}

		// Check to see if the rule is disabled.
		if (pRow->bSkip)
		{
			e_InitDB_Audit("skip\n");
			continue;
		}

		// Check criteria.
		if (!e_InitDB_TestCriteria(pRow->nCriteria, pRow->nRangeLow, pRow->nRangeHigh, ThisSystem))
		{
			e_InitDB_Audit("\n");
			continue;
		}

		// Perform action.
		e_InitDB_TakeAction(pRow, nRow, pMap, pState, tInits);
		e_InitDB_Audit("\n");
	}

	// Apply accumulated initial values.
	e_InitDB_Audit("\nInitial values:\n");
	for (InitInfo::FeatureInitMap::const_iterator i = tInits.FeatureInits.begin(); i != tInits.FeatureInits.end(); ++i)
	{
		e_InitDB_Audit("%s = %s\n", _FCCS(i->first), _FCCS(i->second));
		V(pMap->SelectChoiceFromStopFloor(i->first, i->second));
	}
	for (InitInfo::NumericInitMap::const_iterator i = tInits.NumericInits.begin(); i != tInits.NumericInits.end(); ++i)
	{
		const char * pName = i->first.c_str();
		float fValue = i->second;
		e_InitDB_Audit("%s = %.0f\n", pName, fValue);
		ASSERT_CONTINUE(pState->SetByNameAuto(pName, fValue));
	}

	// Warning one time around is enough.
	#if E_INITDB_ENABLE_WARNINGS
	s_tModule.bWarn = false;
	#endif
}

//-----------------------------------------------------------------------------

void e_InitDB_DeinitLog(void)
{
#if E_INITDB_DEVELOPMENT
	delete s_tModule.pLog;
	s_tModule.pLog = 0;
#endif
}

//-----------------------------------------------------------------------------

void e_InitDB_InitLog(void)
{
#if E_INITDB_DEVELOPMENT
	e_InitDB_DeinitLog();
	if (s_tModule.bDumpToFile)
	{
		OS_PATH_CHAR FileName[MAX_PATH];
		time_t curtime;
		time(&curtime);
		struct tm curtimeex;
		localtime_s(&curtimeex, &curtime);
		PStrPrintf(
			FileName,
			_countof(FileName),
			OS_PATH_TEXT("%sinitdb_audit_%04d%02d%02d_%02d%02d%02d.txt"),
			FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA),
			curtimeex.tm_year + 1900,
			curtimeex.tm_mon + 1,
			curtimeex.tm_mday,
			curtimeex.tm_hour,
			curtimeex.tm_min,
			curtimeex.tm_sec
		);
		FileTextLogA * pLog = new FileTextLogA;
		if (!pLog->Open(FileName))
		{
			delete pLog;
			return;
		}
		s_tModule.pLog = pLog;
	}
	else
	{
		s_tModule.pLog = new DebugTextLogA;
	}
#endif
}

//-----------------------------------------------------------------------------

void e_InitDB_Audit(const FeatureLineMap * pMap)
{
#if E_INITDB_DEVELOPMENT
	e_InitDB_Audit("\nFinal settings:\n");
	e_InitDB_Audit("Feature  Init  Min.  Max.\n");
	e_InitDB_Audit("-------  ----  ----  ----\n");

	for (unsigned i = 0; /**/; ++i)
	{
		DWORD nLineName = 0;
		if (!e_FeatureLine_EnumerateLines(i, nLineName))
		{
			break;
		}

		DWORD nStopName = 0;
		V_CONTINUE(pMap->GetSelectedStopName(nLineName, nStopName));

		DWORD nMinStopName = 0;
		DWORD nMaxStopName = 0;
		V_CONTINUE(pMap->GetMinimumAndMaximumStops(nLineName, nMinStopName, nMaxStopName));

		e_InitDB_Audit("%s ... %s  %s  %s\n", _FCCS(nLineName), _FCCS(nStopName), _FCCS(nMinStopName), _FCCS(nMaxStopName));
	}
#else
	REF(pMap);
#endif
}

//-----------------------------------------------------------------------------

};	// end of anonymous namespace

//-----------------------------------------------------------------------------

PRESULT e_InitDB_Init(void)
{
	if ( e_IsNoRender() )
		return S_FALSE;

	// Get a state object.
	CComPtr<OptionState> pState;
	V_RETURN(e_OptionState_CloneActive(&pState));
	ASSERT_RETFAIL(pState != 0);

	// Get a feature mapping.
	CComPtr<FeatureLineMap> pMap;
	V_RETURN(e_FeatureLine_OpenMap(pState, &pMap));
	ASSERT_RETFAIL(pMap != 0);

	// Init the audit log.
	e_InitDB_InitLog();
	e_InitDB_Audit("*** BEGIN INITDB AUDIT ***\n");

	// Apply the database constraints.
	e_InitDB_Apply(pMap, pState);

	//
	e_InitDB_Audit(pMap);

	// Deinit the audit log.
	e_InitDB_Audit("\n*** END INITDB AUDIT ***\n");
	e_InitDB_DeinitLog();

	// Commit the new settings.
	PRESULT nReturn = S_OK;
	PRESULT nResult;
	V_DO_FAILED(nResult = e_FeatureLine_CommitToActive(pMap))
	{
		nReturn = nResult;
	}
	V_DO_FAILED(nResult = e_OptionState_CommitToActive(pState))
	{
		nReturn = nResult;
	}

	// All done.
	return nReturn;
}

//-----------------------------------------------------------------------------

#if E_INITDB_DEVELOPMENT
void e_InitDB_SetDumpToFile(bool bFlag)
{
	s_tModule.bDumpToFile = bFlag;
}
#endif

//-----------------------------------------------------------------------------

#if E_INITDB_DEVELOPMENT
void e_InitDB_SetIgnoreDB(bool bFlag)
{
	s_tModule.bIgnoreDB = bFlag;
}
#endif
