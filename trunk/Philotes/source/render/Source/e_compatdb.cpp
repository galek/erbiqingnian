//*****************************************************************************
//
// Hardware compatibility database interface.
// Facilitates enforcement of compatibility/performance policies
// based on video hardware and driver.
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//*****************************************************************************

/*
	Database rule precedence determination was designed to not rely
	on the order of rules.  This is important because modifying the
	database may change the ordering of the rules, and because
	context-dependent rules can lead to subtle, hard-to-diagnose
	issues.

	Furthermore, the database must not contain ambiguous rules.
	If any pair of rules both affect the same action, and can both
	apply for any given platform, then the pair is ambiguous.
	Here are some examples of ambiguous rules:

		*	Set cap A to 0 if driver version >= 1.
			Set cap A to 1 if driver version <= 3.
			(Equal specificity when driver version = 2.)
			(Overlapping areas in opposite directions.)

	Here are some examples of unambiguous rules:

		*	Set cap A to 0 if driver version <= 1.
			Set cap A to 1 if driver version <= 3.
			(The former is more specific when driver version <= 1.)

		*	Set cap A to 0 if driver version == 1.
			Set cap A to 1 if driver version <= 3.
			(The former is more specific when driver version == 1.)

	Can an ambiguity be introduced by the removal of a rule?
*/

#include "e_pch.h"
#include "e_compatdb.h"

#if defined( ENGINE_TARGET_DX9 ) || defined( ENGINE_TARGET_DX10 )

#include "pakfile.h"
#include "prime.h"
#include "filepaths.h"
#include "dxC_caps.h"
#include "e_optionstate.h"
#include "e_featureline.h"
#include <string>
#include "safe_vector.h"
#include <algorithm>

extern VideoDeviceInfo gtAdapterID;	// defined in dxC_caps.cpp

namespace
{

//-----------------------------------------------------------------------------

typedef std::string string_type;
typedef string_type path_string;
typedef safe_vector<string_type> StringVector;

const unsigned DB_FORMAT_VERSION = 2;
const char sAnyText[] = "any";

enum COMPAREOP
{
	COMPAREOP_ALWAYS,
	COMPAREOP_NEVER,
	COMPAREOP_EQUAL,
	COMPAREOP_NOTEQUAL,
	COMPAREOP_GREATER,
	COMPAREOP_LESS,
	COMPAREOP_GREATER_OR_EQUAL,
	COMPAREOP_LESS_OR_EQUAL,

	COMPAREOP_COUNT		// not a valid item
};

enum ACTION
{
	ACTION_EMPTY,		// action is empty -- not a valid rule
	ACTION_NONE,		// no action (NOP) -- rule is valid
	ACTION_CAPS,
	ACTION_FEATURE,

	ACTION_COUNT		// not a valid item
};

struct DBROW
{
	// Matching.
	COMPAREOP nTargetOp;
	DWORD nTargetId;			// engine target code
	COMPAREOP nVendorOp;
	DWORD nVendorId;			// video hardware vendor code
	COMPAREOP nDeviceOp1;		// comparison operator for device code 1
	DWORD nDeviceId1;			// video hardware device code 1
	COMPAREOP nDeviceOp2;		// comparison operator for device code 2
	DWORD nDeviceId2;			// video hardware device code 2
	COMPAREOP nSubsystemOp;
	DWORD nSubsystemId;			// video hardware device subsystem code
	COMPAREOP nRevisionOp;
	DWORD nRevisionId;			// video hardware device revision code
	COMPAREOP nDriverVersionOp;	// comparison operaton for driver version
	ULONGLONG nDriverVersion;

	// Action.
	ACTION nAction;
	unsigned nValueId;
	int nValueMin;
	int nValueMax;

	// Other.
	bool bEnabled;
	unsigned nLineNumber;		// line number of file; not persistent
	string_type Comment;

	DBROW(void);
	bool AreCriteriaFieldsEqual(const DBROW & rhs) const;
	bool ArePersistentFieldsEqual(const DBROW & rhs) const;
	bool DoesApplyToAdapter(const VideoDeviceInfo & Adapter) const;
	bool DoesApplyToCurrentAdapter(void) const;
	bool DoActionsConflict(const DBROW & Other) const;

	bool operator<(const DBROW & rhs) const;
};

class File;

class COMPATDB : public safe_vector<DBROW>
{
	public:
		bool AddRow(const DBROW & ToAdd);
		bool DeleteRow(const DBROW & ToAdd);
		bool GetApplicableRows(COMPATDB & DBOut, const VideoDeviceInfo & Adapter) const;
		bool GetApplicableRows(COMPATDB & DBOut) const;
		const_iterator FindConflictingRow(const DBROW & Row) const;
		bool HasConflictingRow(const DBROW & Row) const;
		bool HasRowWithSameAction(const DBROW & Row) const;
		void SortBySpecificity(void);
		void SeparateBySpecificity(COMPATDB & SpecificDatabase, COMPATDB & VagueDatabase, COMPATDB & DisabledDatabase) const;
		bool SaveToFile(const path_string & FileName) const;
		bool WriteToFile(File & f) const;
	private:
		typedef safe_vector<DBROW> base_type;
		using base_type::push_back;
};

//-----------------------------------------------------------------------------

class File
{
	public:
		File(void);
		~File(void);

		typedef string_type string_type;
		typedef string_type::value_type char_type;

		void Close(void);
		bool OpenForReadingText(const path_string & FileName);
		bool OpenForWritingText(const path_string & FileName);

		bool ReadLine(string_type & StrOut);

		bool WriteNewLine(void);
		bool Write(const char_type * pStr);
		bool Write(const string_type & Str);
		bool WriteLine(const char_type * pStr);
		bool WriteLine(const string_type & Str);

	private:
		FILE * pFile;
};

//-----------------------------------------------------------------------------

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define __OVERRIDE override
#else
#define __OVERRIDE
#endif

struct TextReader
{
	virtual ~TextReader(void) throw() {}
	virtual bool ReadLine(string_type & StrOut) = 0;
};

class MemoryTextReader : public TextReader
{
	public:
		MemoryTextReader(const void * pSrc, unsigned nBytes);
		~MemoryTextReader(void);
		virtual bool ReadLine(string_type & StrOut) __OVERRIDE;
	private:
		const string_type::value_type * pPtr;
		unsigned nChars;
};

class FileTextReader : public TextReader
{
	public:
		FileTextReader(File & f);
		~FileTextReader(void);
		virtual bool ReadLine(string_type & StrOut) __OVERRIDE;
	private:
		File & f;
};

//-----------------------------------------------------------------------------

struct PerformState
{
	CComPtr<OptionState> pState;
	CComPtr<FeatureLineMap> pMap;
};

//-----------------------------------------------------------------------------

const string_type e_CompatDB_ToLower(const string_type & Str)
{
	string_type::size_type nLen = Str.length();
	std::auto_ptr<string_type::value_type> pStr(new string_type::value_type[nLen + 1]);
	if (strcpy_s(pStr.get(), nLen + 1, Str.c_str()) != 0)
	{
		return Str;
	}
	if (_strlwr_s(pStr.get(), nLen + 1) != 0)
	{
		return Str;
	}
	return string_type(pStr.get());
}

const char * e_CompatDB_CompareOpString(COMPAREOP nOp)
{
	switch (nOp)
	{
		case COMPAREOP_ALWAYS:				return "al";
		default:
		case COMPAREOP_NEVER:				return "nv";
		case COMPAREOP_EQUAL:				return "eq";
		case COMPAREOP_NOTEQUAL:			return "ne";
		case COMPAREOP_GREATER:				return "gt";
		case COMPAREOP_LESS:				return "lt";
		case COMPAREOP_GREATER_OR_EQUAL:	return "ge";
		case COMPAREOP_LESS_OR_EQUAL:		return "le";
	}
}

bool e_CompatDB_ComparatorFromString(COMPAREOP & nOpOut, const string_type & Str)
{
	string_type s = e_CompatDB_ToLower(Str);
	for (int i = 0; i < COMPAREOP_COUNT; ++i)
	{
		COMPAREOP t = static_cast<COMPAREOP>(i);
		if (s == e_CompatDB_CompareOpString(t))
		{
			nOpOut = t;
			return true;
		}
	}
	return false;
}

template<class T>
bool e_CompatDB_Compare(COMPAREOP nOp, const T & LHS, const T & RHS)
{
	switch (nOp)
	{
		case COMPAREOP_ALWAYS:
			return true;
		case COMPAREOP_NEVER:
		default:
			return false;
		case COMPAREOP_EQUAL:
			return LHS == RHS;
		case COMPAREOP_NOTEQUAL:
			return LHS != RHS;
		case COMPAREOP_GREATER:
			return LHS > RHS;
		case COMPAREOP_LESS:
			return LHS < RHS;
		case COMPAREOP_GREATER_OR_EQUAL:
			return LHS >= RHS;
		case COMPAREOP_LESS_OR_EQUAL:
			return LHS <= RHS;
	}
}

const string_type e_CompatDB_IntToString(unsigned n)
{
	char buf[16];
	_itoa_s(n, buf, sizeof(buf), 10);
	return string_type(buf);
}

const string_type e_CompatDB_CriteriaIntToString(COMPAREOP nOp, DWORD nValue, const string_type & AnyStr)
{
	ASSERT((nOp == COMPAREOP_ALWAYS) || (nOp == COMPAREOP_EQUAL));
	if (nOp == COMPAREOP_ALWAYS)
	{
		return AnyStr;
	}
	else
	{
		return e_CompatDB_IntToString(nValue);
	}
}

const string_type e_CompatDB_VersionToString(ULONGLONG nVersion)
{
	return
		e_CompatDB_IntToString(static_cast<unsigned>((nVersion >> 48) & 0xffff)) + "." +
		e_CompatDB_IntToString(static_cast<unsigned>((nVersion >> 32) & 0xffff)) + "." +
		e_CompatDB_IntToString(static_cast<unsigned>((nVersion >> 16) & 0xffff)) + "." +
		e_CompatDB_IntToString(static_cast<unsigned>((nVersion >>  0) & 0xffff));
}

const string_type e_CompatDB_SafeString(const string_type & Str)
{
	// Commas and quotes not allowed.
	string_type Out = Str;
	string_type::size_type nLen = Out.length();
	for (string_type::size_type i = 0; i < nLen; ++i)
	{
		switch (Str.at(i))
		{
			case ',':
				Out.at(i) = ';';
				break;
			case '\"':
				Out.at(i) = '\'';
				break;
			default:
				break;
		}
	}
	return Out;
}

bool e_CompatDB_IsAny(const string_type & s)
{
	if (s.empty())
	{
		return true;
	}
	if (s == sAnyText)
	{
		return true;
	}
	return false;
}

template<class T>
bool e_CompatDB_IntFromString(T & nValOut, const string_type & ValStr)
{
	if (ValStr.empty())
	{
		// Empty string.
		return false;
	}

	string_type::size_type nPos = 0;
	unsigned nRadix = 10;
	if ((ValStr.length() >= 3) && (ValStr.at(0) == '0') && (ValStr.at(1) == 'x'))
	{
		nRadix = 16;
		nPos = 2;
	}

	T nValue = 0;
	string_type::size_type nLength = ValStr.length();
	for (/**/; nPos < nLength; ++nPos)
	{
		T nNewValue = nValue * nRadix;
		if (nNewValue < nValue)
		{
			// Overflow.
			return false;
		}
		nValue = nNewValue;

		T nDigit;
		char c = ValStr.at(nPos);
		if (('0' <= c) && (c <= '9'))
		{
			nDigit = c - '0';
		}
		else if (('a' <= c) && (c <= 'z'))
		{
			nDigit = c - 'a' + 10;
		}
		else if (('A' <= c) && (c <= 'Z'))
		{
			nDigit = c - 'A' + 10;
		}
		else
		{
			// Bad digit.
			return false;
		}

		if (static_cast<unsigned>(nDigit) >= nRadix)
		{
			// Digit out of range.
			return false;
		}

		nValue += nDigit;
	}

	nValOut = nValue;
	return true;
}

bool e_CompatDB_VersionFromString(ULONGLONG & nVersionOut, const string_type & Str)
{
	ULONGLONG nVer = 0;
	const string_type s = Str + ".";
	string_type::size_type nPos = 0;

	for (unsigned i = 0; i < 4; ++i)
	{
		string_type::size_type nPer = s.find('.', nPos);
		if (nPer == string_type::npos)
		{
			return false;
		}
		unsigned n;
		if (!e_CompatDB_IntFromString(n, s.substr(nPos, nPer - nPos)))
		{
			return false;
		}
		if (n > 0xffff)
		{
			return false;
		}
		nVer |= static_cast<ULONGLONG>(n) << ((3 - i) * 16);
		
		nPos = nPer + 1;

		if (nPos >= s.length())
		{
			break;
		}
	}

	nVersionOut = nVer;
	return true;
}

//-----------------------------------------------------------------------------
//
// Actions
//
//-----------------------------------------------------------------------------

const char * e_CompatDB_ActionToString(ACTION nAct)
{
	switch (nAct)
	{
		default:
		case ACTION_EMPTY:		return "";
		case ACTION_NONE:		return "none";
		case ACTION_CAPS:		return "caps";
		case ACTION_FEATURE:	return "feat";
	}
}

bool e_CompatDB_ActionFromString(ACTION & nActOut, const string_type & Str)
{
	string_type s = e_CompatDB_ToLower(Str);
	for (int i = 0; i < ACTION_COUNT; ++i)
	{
		ACTION t = static_cast<ACTION>(i);
		if (s == e_CompatDB_ActionToString(t))
		{
			nActOut = t;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

bool e_CompatDB_ParseActionMinMaxInt(DBROW & RowOut, StringVector::const_iterator iBegin, StringVector::const_iterator iEnd, string_type & ErrorOut)
{
	// Minimum.
	if (iBegin == iEnd)
	{
		ErrorOut = "missing minimum value";
		return false;
	}
	if (!e_CompatDB_IntFromString(RowOut.nValueMin, *iBegin))
	{
		ErrorOut = "invalid minimum value";
		return false;
	}
	
	// Maximum.
	++iBegin;
	if ((iBegin == iEnd) || iBegin->empty())
	{
		RowOut.nValueMax = RowOut.nValueMin;
		return true;
	}
	if (!e_CompatDB_IntFromString(RowOut.nValueMax, *iBegin))
	{
		ErrorOut = "invalid maximum value";
		return false;
	}

	// Done.
	return true;
}

bool e_CompatDB_ParseAction_FEATURE(DBROW & RowOut, StringVector::const_iterator iBegin, StringVector::const_iterator iEnd, string_type & ErrorOut)
{
	if (iBegin == iEnd)
	{
		ErrorOut = "missing feature line name";
		return false;
	}

	// Find feature line name.
	if (iBegin->length() != 4)
	{
		ErrorOut = "invalid feature line name";
		return false;
	}
	DWORD nLineName = E_FEATURELINE_FOURCC(iBegin->c_str());
	DWORD nStopName;
	if (FAILED(e_FeatureLine_EnumerateStopNames(nLineName, 0, nStopName)))
	{
		ErrorOut = "invalid feature line name";
		return false;
	}
	RowOut.nValueId = nLineName;

	// Minimum.
	++iBegin;
	if (iBegin == iEnd)
	{
		ErrorOut = "missing minimum value";
		return false;
	}
	// It's legal to have no minimum value.
	// In that case the minimum will not be affected by the rule.
	DWORD nMinStopName = 0;
	RowOut.nValueMin = 0;
	if (!iBegin->empty())
	{
		if (iBegin->length() != 4)
		{
			ErrorOut = "invalid minimum value";
			return false;
		}
		nMinStopName = E_FEATURELINE_FOURCC(iBegin->c_str());

		// Verify existence of stop name.
		for (unsigned i = 0; SUCCEEDED(e_FeatureLine_EnumerateStopNames(nLineName, i, nStopName)); ++i)
		{
			if (nStopName == nMinStopName)
			{
				RowOut.nValueMin = nMinStopName;
				break;
			}
		}
		if (RowOut.nValueMin == 0)
		{
			ErrorOut = "minimum value: invalid feature line stop name";
			return false;
		}
	}
	
	// Maximum.
	++iBegin;
	RowOut.nValueMax = 0;
	if ((iBegin == iEnd) || iBegin->empty())
	{
		// It's legal to have no maximum value.
		// In that case the maximum will not be affected by the rule.
		return true;
	}
	if (iBegin->length() != 4)
	{
		ErrorOut = "invalid maximum value";
		return false;
	}
	DWORD nMaxStopName = E_FEATURELINE_FOURCC(iBegin->c_str());

	// Verify existence of stop name.
	bool bSawMin = false;
	for (unsigned i = 0; SUCCEEDED(e_FeatureLine_EnumerateStopNames(nLineName, i, nStopName)); ++i)
	{
		if (nStopName == nMinStopName)
		{
			bSawMin = true;
		}
		if (nStopName == nMaxStopName)
		{
			RowOut.nValueMax = nMaxStopName;
			break;
		}
	}
	if (RowOut.nValueMax == 0)
	{
		ErrorOut = "maximum value: invalid feature line stop name";
		return false;
	}
	if ((!bSawMin) && (nMinStopName != 0))
	{
		ErrorOut = "maximum feature line stop name is less than minimum";
		return false;
	}
	
	// Done.
	return true;
}

const string_type e_CompatDB_GetCapName(int nFlag)
{
	PLATFORM_CAP nCap = static_cast<PLATFORM_CAP>(nFlag);
	string_type::value_type Buf[8];
	dx9_CapGetFourCCString(nCap, Buf, _countof(Buf));
	return e_CompatDB_ToLower(Buf);
}

bool e_CompatDB_ParseAction_CAPS(DBROW & RowOut, StringVector::const_iterator iBegin, StringVector::const_iterator iEnd, string_type & ErrorOut)
{
	//
	if (iBegin == iEnd)
	{
		ErrorOut = "missing cap flag name";
		return false;
	}

	//
	string_type FlagStr = e_CompatDB_ToLower(*iBegin);

	// Find cap flag name.
	int nFlag;
	for (nFlag = DX9CAP_DISABLE + 1; nFlag < NUM_PLATFORM_CAPS; ++nFlag)
	{
		if (FlagStr == e_CompatDB_GetCapName(nFlag))
		{
			break;
		}
	}
	if (nFlag >= NUM_PLATFORM_CAPS)
	{
		ErrorOut = "invalid cap flag name";
		return false;
	}
	RowOut.nValueId = nFlag;

	// Do some translation.
	switch (RowOut.nValueId)
	{
		case DX9CAP_MAX_VS_VERSION:
		case DX9CAP_MAX_PS_VERSION:
		{
			++iBegin;
			if (iBegin == iEnd)
			{
				ErrorOut = "missing minimum value";
				return false;
			}
			ULONGLONG nMin, nMax;
			if (!e_CompatDB_VersionFromString(nMin, *iBegin))
			{
				ErrorOut = "invalid minimum value";
				return false;
			}
			++iBegin;
			if ((iBegin == iEnd) || iBegin->empty())
			{
				nMax = nMin;
			}
			else if (!e_CompatDB_VersionFromString(nMax, *iBegin))
			{
				ErrorOut = "invalid minimum value";
				return false;
			}
			unsigned nMinMajor = static_cast<unsigned>((nMin >> 48) & 0xff);
			unsigned nMinMinor = static_cast<unsigned>((nMin >> 32) & 0xff);
			unsigned nMaxMajor = static_cast<unsigned>((nMax >> 48) & 0xff);
			unsigned nMaxMinor = static_cast<unsigned>((nMax >> 32) & 0xff);
			if (RowOut.nValueId == DX9CAP_MAX_VS_VERSION)
			{
				RowOut.nValueMin = D3DVS_VERSION(nMinMajor, nMinMinor);
				RowOut.nValueMax = D3DVS_VERSION(nMaxMajor, nMaxMinor);
			}
			else
			{
				RowOut.nValueMin = D3DPS_VERSION(nMinMajor, nMinMinor);
				RowOut.nValueMax = D3DPS_VERSION(nMaxMajor, nMaxMinor);
			}
			return true;
		}
		default:
			break;
	}

	return e_CompatDB_ParseActionMinMaxInt(RowOut, iBegin + 1, iEnd, ErrorOut);
}

bool e_CompatDB_ParseAction(DBROW & RowOut, ACTION nAction, StringVector::const_iterator iBegin, StringVector::const_iterator iEnd, string_type & ErrorOut)
{
	switch (nAction)
	{
		case ACTION_FEATURE:
			return e_CompatDB_ParseAction_FEATURE(RowOut, iBegin, iEnd, ErrorOut);
		case ACTION_CAPS:
			return e_CompatDB_ParseAction_CAPS(RowOut, iBegin, iEnd, ErrorOut);
		default:
			ErrorOut = "unrecognized action";
			return false;
	}
}

//-----------------------------------------------------------------------------

const string_type e_CompatDB_ComposeActionMinMaxInt(const DBROW & Row)
{
	string_type s = e_CompatDB_IntToString(Row.nValueMin);
	if (Row.nValueMin != Row.nValueMax)
	{
		s = s + "," + e_CompatDB_IntToString(Row.nValueMax);
	}
	return s;
}

const string_type e_CompatDB_ComposeAction_FEATURE(const DBROW & Row)
{
	char buf[5];
	DWORD nName = Row.nValueId;
	string_type s = FOURCCTOSTR(nName, buf);
	nName = Row.nValueMin;
	s = s + "," + FOURCCTOSTR(nName, buf);
	if (Row.nValueMin != Row.nValueMax)
	{
		nName = Row.nValueMax;
		s = s + "," + FOURCCTOSTR(nName, buf);
	}
	return s;
}

const string_type e_CompatDB_ComposeAction_CAPS(const DBROW & Row)
{
	string_type s = e_CompatDB_GetCapName(Row.nValueId);
	
	// Do some translation.
	switch (Row.nValueId)
	{
		case DX9CAP_MAX_VS_VERSION:
		case DX9CAP_MAX_PS_VERSION:
		{
			ULONGLONG nMin = (static_cast<ULONGLONG>((Row.nValueMin >> 8) & 0xff) << 48) | (static_cast<ULONGLONG>((Row.nValueMin >> 0) & 0xff) << 32);
			ULONGLONG nMax = (static_cast<ULONGLONG>((Row.nValueMax >> 8) & 0xff) << 48) | (static_cast<ULONGLONG>((Row.nValueMax >> 0) & 0xff) << 32);
			string_type MinStr = e_CompatDB_VersionToString(nMin);
			string_type MaxStr = e_CompatDB_VersionToString(nMax);
			MinStr.resize(MinStr.length() - 4);
			MaxStr.resize(MaxStr.length() - 4);
			s = s + "," + MinStr;
			if (Row.nValueMax != Row.nValueMin)
			{
				s = s + "," + MaxStr;
			}
			return s;
		}
		default:
			break;
	}

	return s + "," + e_CompatDB_ComposeActionMinMaxInt(Row);
}

const string_type e_CompatDB_ComposeAction(const DBROW & Row)
{
	string_type s = e_CompatDB_ActionToString(Row.nAction);
	switch (Row.nAction)
	{
		case ACTION_FEATURE:
			return s + "," + e_CompatDB_ComposeAction_FEATURE(Row);
		case ACTION_CAPS:
			return s + "," + e_CompatDB_ComposeAction_CAPS(Row);
		default:
			ASSERT(false);
			return "";
	}
}

//-----------------------------------------------------------------------------

bool e_CompatDB_PerformAction_FEATURE(const DBROW & Row, PerformState & State)
{
	PRESULT nResult1 = (Row.nValueMin != 0) ? State.pMap->AdjustMinimumStop(Row.nValueId, Row.nValueMin) : S_OK;
	PRESULT nResult2 = (Row.nValueMax != 0) ? State.pMap->AdjustMaximumStop(Row.nValueId, Row.nValueMax) : S_OK;
	return SUCCEEDED(nResult1) && SUCCEEDED(nResult2);
}

bool e_CompatDB_PerformAction_CAPS(const DBROW & Row, PerformState & /*State*/)
{
	if ((DX9CAP_DISABLE < Row.nValueId) && (Row.nValueId < NUM_PLATFORM_CAPS))
	{
		PLATFORM_CAP nFlag = static_cast<PLATFORM_CAP>(Row.nValueId);
		DWORD nMin = Row.nValueMin;
		DWORD nMax = Row.nValueMax;
		V( dx9_PlatformCapSetMinMax(nFlag, nMin, nMax) );
		return true;
	}
	return false;
}

bool e_CompatDB_PerformAction(const DBROW & Row, PerformState & State)
{
	ASSERT(Row.DoesApplyToCurrentAdapter());
	switch (Row.nAction)
	{
		case ACTION_FEATURE:
			return e_CompatDB_PerformAction_FEATURE(Row, State);
		case ACTION_CAPS:
			return e_CompatDB_PerformAction_CAPS(Row, State);
		default:
			return false;
	}
}

//-----------------------------------------------------------------------------

bool e_CompatDB_AreSameAction(const DBROW & RowA, const DBROW & RowB)
{
	if (RowA.nAction != RowB.nAction)
	{
		return false;
	}
	switch (RowA.nAction)
	{
		case ACTION_FEATURE:
		case ACTION_CAPS:
			return RowA.nValueId == RowB.nValueId;
		default:
			return true;
	}
}

//-----------------------------------------------------------------------------
//
// Criteria
//
//-----------------------------------------------------------------------------

const string_type e_CompatDB_ComposeCriteria(const DBROW & Row, const string_type & AnyStr)
{
	string_type Str;

	Str += Row.bEnabled ? "Y," : "N,";
	Str += e_CompatDB_CriteriaIntToString(Row.nTargetOp, Row.nTargetId, AnyStr);
	Str += "," + e_CompatDB_CriteriaIntToString(Row.nVendorOp, Row.nVendorId, AnyStr);

	if (Row.nDeviceOp1 == COMPAREOP_ALWAYS)
	{
		Str += "," + AnyStr + "," + AnyStr;
	}
	else
	{
		Str += ",";
		Str += e_CompatDB_CompareOpString(Row.nDeviceOp1);
		Str += ",";
		Str += e_CompatDB_IntToString(Row.nDeviceId1);
	}

	if (Row.nDeviceOp2 == COMPAREOP_ALWAYS)
	{
		Str += "," + AnyStr + "," + AnyStr;
	}
	else
	{
		Str += ",";
		Str += e_CompatDB_CompareOpString(Row.nDeviceOp2);
		Str += ",";
		Str += e_CompatDB_IntToString(Row.nDeviceId2);
	}

	Str += "," + e_CompatDB_CriteriaIntToString(Row.nSubsystemOp, Row.nSubsystemId, AnyStr);
	Str += "," + e_CompatDB_CriteriaIntToString(Row.nRevisionOp, Row.nRevisionId, AnyStr);

	if (Row.nDriverVersionOp == COMPAREOP_ALWAYS)
	{
		Str += "," + AnyStr + "," + AnyStr;
	}
	else
	{
		Str += ",";
		Str += e_CompatDB_CompareOpString(Row.nDriverVersionOp);
		Str += ",";
		Str += e_CompatDB_VersionToString(Row.nDriverVersion);
	}

	string_type Com = "\"" + e_CompatDB_SafeString(Row.Comment) + "\"";
	Str += "," + Com;

	return Str;
}

const string_type e_CompatDB_ComposeCSVLine(const DBROW & Row, const string_type & AnyStr)
{
	string_type Str = e_CompatDB_ComposeCriteria(Row, AnyStr);
	Str += ",";
	Str += e_CompatDB_ComposeAction(Row);
	return Str;
}

bool e_CompatDB_ParseValue(COMPAREOP & nOpOut, DWORD & nValOut, const string_type & ValStr)
{
	if (e_CompatDB_IsAny(ValStr))
	{
		nOpOut = COMPAREOP_ALWAYS;
		nValOut = 0;
		return true;
	}
	else if (e_CompatDB_IntFromString(nValOut, ValStr))
	{
		nOpOut = COMPAREOP_EQUAL;
		return true;
	}
	return false;
}

bool e_CompatDB_ParseValue4CC(COMPAREOP & nOpOut, DWORD & nValOut, const string_type & ValStr)
{
	if (e_CompatDB_IsAny(ValStr))
	{
		nOpOut = COMPAREOP_ALWAYS;
		nValOut = 0;
		return true;
	}
	else
	{
		nValOut = MAKEFOURCCSTR(ValStr.c_str());
		nOpOut = COMPAREOP_EQUAL;
		return true;
	}
}

const string_type e_CompatDB_BuildString(const string_type::value_type * pMsg, va_list args)
{
	char buf[1024];
	vsprintf_s(buf, sizeof(buf), pMsg, args);
	return string_type(buf);
}

const string_type e_CompatDB_BuildString(const string_type::value_type * pMsg, ...)
{
	va_list args;
	va_start(args, pMsg);
	return e_CompatDB_BuildString(pMsg, args);
}

HWND e_CompatDB_GetHWND(void)
{
	return AppCommonGetHWnd();
}

#ifdef E_COMPATDB_ENABLE_EDIT

int e_CompatDB_MessageBox(const string_type::value_type * pMsg, const string_type::value_type * pCap, unsigned nFlags)
{
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( AppCommonGetSilentAssert() || ( pGlobals && pGlobals->dwFlags & GLOBAL_FLAG_NO_POPUPS ) )
		return IDOK;
	return ::MessageBoxA(e_CompatDB_GetHWND(), pMsg, pCap, nFlags | MB_SYSTEMMODAL);	// CHB 2007.08.01 - String audit: development
}

void e_CompatDB_NotifyUserOfWarning(const string_type::value_type * pMsg, ...)
{
	va_list args;
	va_start(args, pMsg);
	string_type s = e_CompatDB_BuildString(pMsg, args);
	e_CompatDB_MessageBox(s.c_str(), "CompatDB Warning", MB_ICONWARNING | MB_OK);	// CHB 2007.08.01 - String audit: development
}

void e_CompatDB_NotifyUserOfError(const string_type::value_type * pMsg, ...)
{
	va_list args;
	va_start(args, pMsg);
	string_type s = e_CompatDB_BuildString(pMsg, args);
	e_CompatDB_MessageBox(s.c_str(), "CompatDB Error", MB_ICONERROR | MB_OK);	// CHB 2007.08.01 - String audit: development
}

#endif

bool e_CompatDB_ParseRow(DBROW & RowOut, const string_type & Line, string_type & ErrorOut)
{
	StringVector Columns;
	Columns.reserve(15);	// anticipated max columns

	// Parse line into columns.
	for (string_type::size_type nPos = 0; nPos < Line.length(); /**/)
	{
		// Get column.
		string_type::size_type nCommaPos = Line.find(',', nPos);
		if (nCommaPos == string_type::npos)
		{
			Columns.push_back(Line.substr(nPos));
			nPos = Line.length();
		}
		else
		{
			Columns.push_back(Line.substr(nPos, nCommaPos - nPos));
			nPos = nCommaPos + 1;
		}
	}

	// Handle individual columns.

	// Enable/disable.
	StringVector::const_iterator iCol = Columns.begin();
	if ((iCol == Columns.end()) || iCol->empty())
	{
		ErrorOut = "missing enable/disable value";
		return false;
	}
	RowOut.bEnabled = (iCol->at(0) == 'y') || (iCol->at(0) == 'Y') || (iCol->at(0) == '1');

	// Target.
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing target id";
		return false;
	}
	if (!e_CompatDB_ParseValue4CC(RowOut.nTargetOp, RowOut.nTargetId, *iCol))
	{
		ErrorOut = "invalid target id";
		return false;
	}

	// Vendor.
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing vendor id";
		return false;
	}
	if (!e_CompatDB_ParseValue(RowOut.nVendorOp, RowOut.nVendorId, *iCol))
	{
		ErrorOut = "invalid vendor id";
		return false;
	}

	// Device comparator (first).
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing device id 1 comparator";
		return false;
	}
	if (e_CompatDB_IsAny(*iCol))
	{
		RowOut.nDeviceOp1 = COMPAREOP_ALWAYS;
	}
	else if (!e_CompatDB_ComparatorFromString(RowOut.nDeviceOp1, *iCol))
	{
		ErrorOut = "invalid device id 1 comparator";
		return false;
	}

	// Device ID (first).
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing device id 1";
		return false;
	}
	if (RowOut.nDeviceOp1 != COMPAREOP_ALWAYS)
	{
		if (e_CompatDB_IsAny(*iCol))
		{
			RowOut.nDeviceOp1 = COMPAREOP_ALWAYS;
		}
		else if (!e_CompatDB_IntFromString(RowOut.nDeviceId1, *iCol))
		{
			ErrorOut = "invalid device id 1";
			return false;
		}
	}

	// Device comparator (second).
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing device id 2 comparator";
		return false;
	}
	if (e_CompatDB_IsAny(*iCol))
	{
		RowOut.nDeviceOp2 = COMPAREOP_ALWAYS;
	}
	else if (!e_CompatDB_ComparatorFromString(RowOut.nDeviceOp2, *iCol))
	{
		ErrorOut = "invalid device id 2 comparator";
		return false;
	}

	// Device ID (second).
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing device id 2";
		return false;
	}
	if (RowOut.nDeviceOp2 != COMPAREOP_ALWAYS)
	{
		if (e_CompatDB_IsAny(*iCol))
		{
			RowOut.nDeviceOp2 = COMPAREOP_ALWAYS;
		}
		else if (!e_CompatDB_IntFromString(RowOut.nDeviceId2, *iCol))
		{
			ErrorOut = "invalid device id 2";
			return false;
		}
	}

	// Subsystem.
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing subsystem id";
		return false;
	}
	if (!e_CompatDB_ParseValue(RowOut.nSubsystemOp, RowOut.nSubsystemId, *iCol))
	{
		ErrorOut = "invalid subsystem id";
		return false;
	}

	// Revision.
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing revision id";
		return false;
	}
	if (!e_CompatDB_ParseValue(RowOut.nRevisionOp, RowOut.nRevisionId, *iCol))
	{
		ErrorOut = "invalid revision id";
		return false;
	}

	//
	RowOut.nDriverVersion = 0;

	// Driver version comparator.
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing driver version comparator";
		return false;
	}
	if (e_CompatDB_IsAny(*iCol))
	{
		RowOut.nDriverVersionOp = COMPAREOP_ALWAYS;
	}
	else if (!e_CompatDB_ComparatorFromString(RowOut.nDriverVersionOp, *iCol))
	{
		ErrorOut = "invalid driver version comparator";
		return false;
	}

	// Driver version.
	++iCol;
	if (iCol == Columns.end())
	{
		ErrorOut = "missing driver version";
		return false;
	}
	if (RowOut.nDriverVersionOp != COMPAREOP_ALWAYS)
	{
		if (e_CompatDB_IsAny(*iCol))
		{
			RowOut.nDriverVersionOp = COMPAREOP_ALWAYS;
		}
		else if (!e_CompatDB_VersionFromString(RowOut.nDriverVersion, *iCol))
		{
			ErrorOut = "invalid driver version";
			return false;
		}
	}

	//
	RowOut.nAction = ACTION_EMPTY;

	// Comment.
	++iCol;
	if (iCol == Columns.end())
	{
		// No comment, no action -- that's okay.
		return true;
	}
	RowOut.Comment = *iCol;
	if (RowOut.Comment.length() >= 2)
	{
		if ((RowOut.Comment.at(0) == '\"') && (RowOut.Comment.at(RowOut.Comment.length() - 1) == '\"'))
		{
			// Remove surrounding quotes, if any.
			RowOut.Comment = RowOut.Comment.substr(1, RowOut.Comment.length() - 2);
		}
	}
	RowOut.Comment = e_CompatDB_SafeString(RowOut.Comment);

	// Action.
	++iCol;
	if (iCol == Columns.end())
	{
		// No action -- that's okay.
		return true;
	}
	if (iCol->empty())
	{
		// No action -- that's okay.
		// Just make sure that the rest of the columns are empty...
		for (++iCol; iCol != Columns.end(); ++iCol)
		{
			if (!iCol->empty())
			{
				ErrorOut = "extraneous columns after empty action";
				return false;
			}
		}
		return true;
	}
	if (!e_CompatDB_ActionFromString(RowOut.nAction, *iCol))
	{
		ErrorOut = "invalid action";
		return false;
	}

	// Handle the action portion, which may differ depending on the action.
	return e_CompatDB_ParseAction(RowOut, RowOut.nAction, iCol + 1, Columns.end(), ErrorOut);
}

const path_string e_CompatDB_GetTemporaryFilePath(void)
{
	DWORD nNeeded = ::GetTempPathA(0, 0);
	for (;;)
	{
		std::auto_ptr<char> pBuffer(new char[nNeeded + 1]);
		DWORD nLength = ::GetTempPathA(nNeeded + 1, pBuffer.get());
		if (nLength <= nNeeded)
		{
			return path_string(pBuffer.get()) + "CompatDBTemp\\";
		}
		nNeeded = nLength;
	}
}

const path_string e_CompatDB_GetTemporaryFileName(void)
{
	return e_CompatDB_GetTemporaryFilePath() + "CompatDB_Edit.csv";
}

const char * e_CompatDB_GetHeaderRow(void)
{
	return "//Enabled,Vendor ID,Device ID,Subsystem ID,Revision ID,Driver Version Comparator,Driver Version,Comment,Action,Setting,Minimum,Maximum";
}

const VideoDeviceInfo & e_CompatDB_GetCurrentAdapterIdentifier(void)
{
	return ::gtAdapterID;
}

// This version return true and TimeOut = 0 if the file's not found.
bool e_CompatDB_GetFileWriteTimeOrZero(ULONGLONG & TimeOut, const path_string & FileName)
{
	WIN32_FIND_DATA FindData;
	FindFileHandle handle = ::FindFirstFileA(FileName.c_str(), &FindData);
	if ((handle == 0) || (handle == INVALID_HANDLE_VALUE))
	{
		DWORD nError = ::GetLastError();
		if (nError == ERROR_FILE_NOT_FOUND)
		{
			TimeOut = 0;
			return true;
		}
		return false;
	}
	ULARGE_INTEGER uli;
	uli.LowPart = FindData.ftLastWriteTime.dwLowDateTime;
	uli.HighPart = FindData.ftLastWriteTime.dwHighDateTime;
	TimeOut = uli.QuadPart;
	return true;
}

// This version return false if the file's not found.
bool e_CompatDB_GetFileWriteTime(ULONGLONG & TimeOut, const path_string & FileName)
{
	return e_CompatDB_GetFileWriteTimeOrZero(TimeOut, FileName) && (TimeOut != 0);
}

bool e_CompatDB_ReadCSVFile(COMPATDB & RowsOut, TextReader & Tr, unsigned nStartLineNumber, string_type & ErrorOut, bool bIgnoreParseErrors)
{
	COMPATDB Rows;
	string_type Line;
	string_type Err;
	for (unsigned nLine = nStartLineNumber; Tr.ReadLine(Line); ++nLine)
	{
		//
		if (Line.empty())
		{
			continue;
		}

		// Check for comment.
		if (Line.length() >= 2)
		{
			if ((Line.at(0) == '/') && (Line.at(1) == '/'))
			{
				continue;
			}
		}

		DBROW row;
		row.nLineNumber = nLine;
		if (!e_CompatDB_ParseRow(row, Line, Err))
		{
			if (!bIgnoreParseErrors)
			{
				ErrorOut = Err + e_CompatDB_BuildString(" in row %u", nLine);
				return false;
			}
			continue;
		}

		// Don't bother with empty actions.
		if (row.nAction == ACTION_EMPTY)
		{
			continue;
		}

		if (!Rows.AddRow(row))
		{
			if (!bIgnoreParseErrors)
			{
				COMPATDB::const_iterator i = Rows.FindConflictingRow(row);
				ASSERT(i != Rows.end());
				unsigned nLine1 = nLine;
				if (i != Rows.end())
				{
					nLine1 = i->nLineNumber;
				}
#ifdef E_COMPATDB_ENABLE_EDIT
				e_CompatDB_NotifyUserOfWarning("Row %u conflicts with row %u and will be ignored.", nLine, nLine1);	// CHB 2007.08.01 - String audit: development
#endif
			}
		}
	}

	RowsOut.swap(Rows);
	return true;
}

bool e_CompatDB_ReadCSVFile(COMPATDB & RowsOut, const path_string & FileName, string_type & ErrorOut, bool bIgnoreParseErrors)
{
	File f;
	if (!f.OpenForReadingText(FileName))
	{
		ErrorOut = "unable to open file for reading";
		return false;
	}
	FileTextReader Ftr(f);
	return e_CompatDB_ReadCSVFile(RowsOut, Ftr, 1, ErrorOut, bIgnoreParseErrors);
}

const path_string e_CompatDB_GetMasterDatabaseFileName(void)
{
	return path_string(FILE_PATH_EXCEL) + "compatdb.csv";
}

bool e_CompatDB_IsFileReadOnly(bool & bReadOnlyOut, const path_string & FileName)
{
	DWORD nResult = ::GetFileAttributesA(FileName.c_str());
	if (nResult == INVALID_FILE_ATTRIBUTES)
	{
		DWORD nError = ::GetLastError();
		if (nError != ERROR_FILE_NOT_FOUND)
		{
			return false;
		}
		nResult = 0;
	}
	bReadOnlyOut = (nResult & FILE_ATTRIBUTE_READONLY) != 0;
	return true;
}

bool e_CompatDB_ApplyDatabase(const COMPATDB & Database, PerformState & State)
{
	// First, get only those rules that apply to the current platform.
	COMPATDB ReducedDatabase;
	if (!Database.GetApplicableRows(ReducedDatabase))
	{
		return false;
	}

	// Next, separate out which specific rules apply.
	COMPATDB SpecificDatabase, VagueDatabase, DisabledDatabase;
	ReducedDatabase.SeparateBySpecificity(SpecificDatabase, VagueDatabase, DisabledDatabase);

	// Now, run through and apply them.
	for (COMPATDB::const_iterator i = SpecificDatabase.begin(); i != SpecificDatabase.end(); ++i)
	{
		e_CompatDB_PerformAction(*i, State);
	}

	return true;
}

bool e_CompatDB_ApplyDatabase(const COMPATDB & Database)
{
	PerformState State;

	if (FAILED(e_OptionState_CloneActive(&State.pState)))
	{
		return false;
	}
	if (FAILED(e_FeatureLine_OpenMap(State.pState, &State.pMap)))
	{
		return false;
	}
	if (!e_CompatDB_ApplyDatabase(Database, State))
	{
		return false;
	}
	if (FAILED(e_FeatureLine_CommitToActive(State.pMap)))
	{
		return false;
	}
	if (FAILED(e_OptionState_CommitToActive(State.pState)))
	{
		return false;
	}
	return true;
}

bool e_CompatDB_LoadMasterDatabase(COMPATDB & DBOut, string_type & ErrorOut, bool bIgnoreParseErrors)
{
	DWORD dwBytesRead = 0;
	void * pData = e_CompatDB_LoadMasterDatabaseFile( &dwBytesRead );
	if (pData == 0)
	{
		ErrorOut = "pak file load failed";
		return false;
	}

	bool bResult = false;
	for (;;)
	{
		MemoryTextReader Mtr(pData, dwBytesRead);

		string_type VersionLine;
		if (!Mtr.ReadLine(VersionLine))
		{
			ErrorOut = "failed to read database version";
			break;
		}

		// Stop at first comma
		string_type::size_type nComma = VersionLine.find(',');
		if (nComma != string_type::npos)
		{
			VersionLine = VersionLine.substr(0, nComma);
		}

		unsigned nVersion = 0;
		if (!e_CompatDB_IntFromString(nVersion, VersionLine))
		{
			ErrorOut = "invalid database version";
			break;
		}

		if (nVersion != DB_FORMAT_VERSION)
		{
			ErrorOut = "database version mismatch";
			break;
		}

		if (!e_CompatDB_ReadCSVFile(DBOut, Mtr, 2, ErrorOut, bIgnoreParseErrors))
		{
			// Error already set by called function.
			break;
		}

		bResult = true;
		break;
	}

	FREE(NULL, pData);	// free buffer
	return bResult;
}

//-----------------------------------------------------------------------------

File::File(void)
{
	pFile = 0;
}

File::~File(void)
{
	Close();
}

void File::Close(void)
{
	if (pFile != 0)
	{
		fclose(pFile);
		pFile = 0;
	}
}

bool File::OpenForReadingText(const string_type & FileName)
{
	Close();
//	return fopen_s(&pFile, FileName.c_str(), "rt") == 0;
	pFile = _fsopen(FileName.c_str(), "rt", _SH_DENYNO);
	return pFile != 0;
}

bool File::OpenForWritingText(const string_type & FileName)
{
	Close();
	return fopen_s(&pFile, FileName.c_str(), "wt") == 0;
}

bool File::WriteNewLine(void)
{
	return fputc('\n', pFile) != EOF;
}

bool File::Write(const char_type * pStr)
{
	return fputs(pStr, pFile) >= 0;
}

bool File::Write(const string_type & Str)
{
	return Write(Str.c_str());
}

bool File::WriteLine(const char_type * pStr)
{
	return Write(pStr) && WriteNewLine();
}

bool File::WriteLine(const string_type & Str)
{
	return Write(Str) && WriteNewLine();
}

bool File::ReadLine(string_type & StrOut)
{
	string_type s;
	for (;;)
	{
		char buf[256];
		if (fgets(buf, sizeof(buf), pFile) == 0)
		{
			StrOut = s;
			return !s.empty();
		}
		string_type t(buf);
		s += t;
		if ((!s.empty()) && (s.at(s.length() - 1) == '\n'))
		{
			s.resize(s.length() - 1);
			break;
		}
	}
	StrOut = s;
	return true;
}

//-----------------------------------------------------------------------------

struct E_COMPATDB_STATE
{
	E_COMPATDB_STATE(void);
	~E_COMPATDB_STATE(void);

	void CloseChangeNotification(void);

#ifdef E_COMPATDB_ENABLE_EDIT
	bool WatchFile(const path_string & FileName);
	void CheckForUpdates(void);
	void WatchFileUpdated(void);
	bool CacheMasterDatabase(void);
#endif

	const COMPATDB & GetMasterDatabase(void) const;

	COMPATDB MasterDatabase;
	ULONGLONG nMasterDatabaseWriteTime;

	COMPATDB TempDatabase;

	HANDLE hChangeNotification;
	ULONGLONG OriginalFileWriteTime;
	path_string WatchFileName;
};

E_COMPATDB_STATE * s_pState = 0;

E_COMPATDB_STATE::E_COMPATDB_STATE(void)
{
	hChangeNotification = 0;
	nMasterDatabaseWriteTime = 0;
}

E_COMPATDB_STATE::~E_COMPATDB_STATE(void)
{
	CloseChangeNotification();
}

void E_COMPATDB_STATE::CloseChangeNotification(void)
{
	if (hChangeNotification != 0)
	{
		::FindCloseChangeNotification(hChangeNotification);
		hChangeNotification = 0;
	}
}

#ifdef E_COMPATDB_ENABLE_EDIT
bool E_COMPATDB_STATE::WatchFile(const path_string & FileName)
{
	// Clear any prior notification request.
	CloseChangeNotification();

	// Get the file's current write time as a basis for comparison.
	if (!e_CompatDB_GetFileWriteTime(OriginalFileWriteTime, FileName))
	{
		e_CompatDB_NotifyUserOfError("Unable to find file to watch.");	// CHB 2007.08.01 - String audit: development
		return false;
	}
	WatchFileName = FileName;

	// Request system notification of changes to the directory.
	HANDLE hCN = ::FindFirstChangeNotificationA(
		e_CompatDB_GetTemporaryFilePath().c_str(),
		false,
		FILE_NOTIFY_CHANGE_LAST_WRITE
	);
	if ((hCN == 0) || (hCN == INVALID_HANDLE_VALUE))
	{
		e_CompatDB_NotifyUserOfError("Unable to obtain notification of directory changes.");	// CHB 2007.08.01 - String audit: development
		return false;
	}
	hChangeNotification = hCN;

	// Okay.
	return true;
}
#endif

#ifdef E_COMPATDB_ENABLE_EDIT
void E_COMPATDB_STATE::CheckForUpdates(void)
{
	if (hChangeNotification == 0)
	{
		return;
	}

	// Check for notification of change.
	DWORD nResult = ::WaitForSingleObject(hChangeNotification, 0);
	if (nResult != WAIT_OBJECT_0)
	{
		return;
	}

	// Continue watching after triggered.
	if (!::FindNextChangeNotification(hChangeNotification))
	{
		//
	}

	// Get the file's current time stamp.
	ULONGLONG CurrentTime;
	if (!e_CompatDB_GetFileWriteTime(CurrentTime, WatchFileName))
	{
		return;
	}

	// Check the time stamp.
	if (CurrentTime <= OriginalFileWriteTime)
	{
		// Not newer.
		return;
	}
	OriginalFileWriteTime = CurrentTime;

	// File has been updated!
	WatchFileUpdated();
}
#endif

#ifdef E_COMPATDB_ENABLE_EDIT
void E_COMPATDB_STATE::WatchFileUpdated(void)
{
	COMPATDB AllNewRows;
	AllNewRows.reserve(TempDatabase.size());
	string_type ErrorMsg;
	if (!e_CompatDB_ReadCSVFile(AllNewRows, WatchFileName, ErrorMsg, false))
	{
		ErrorMsg = "Error: " + ErrorMsg + ". Changes ignored.";
		e_CompatDB_NotifyUserOfError(ErrorMsg.c_str());	// CHB 2007.08.01 - String audit: development
		return;
	}

	// Don't allow user to enter rules that don't pertain to the current platform.
	for (COMPATDB::size_type i = 0; i < AllNewRows.size(); /**/)
	{
		if (!AllNewRows[i].DoesApplyToCurrentAdapter())
		{
			string_type s = e_CompatDB_BuildString(
				"The rule in row %u does not apply to current adapter. Do you want to keep this rule?"
				"\n\nIf you're sure you want to keep this rule for a different adapter, choose \"Yes.\""
				"\n(To avoid seeing this warning again, close the file then re-edit it.)"
				"\n\nTo ignore this rule, choose \"No.\"",
				AllNewRows[i].nLineNumber
			);
			int nResult = e_CompatDB_MessageBox(s.c_str(), "CompatDB Question", MB_ICONQUESTION | MB_YESNO);	// CHB 2007.08.01 - String audit: development
			if (nResult != IDYES)
			{
				AllNewRows.erase(AllNewRows.begin() + i);
				continue;
			}
		}
		++i;
	}

	//
	COMPATDB NewRows = AllNewRows;
	COMPATDB OldRows = TempDatabase;

	// Compare the original with the new rows.
	// First find any equivalents and delete them; these have not been changed.
	for (COMPATDB::size_type i = 0; i < OldRows.size(); /**/)
	{
		if (NewRows.DeleteRow(OldRows[i]))
		{
			OldRows.erase(OldRows.begin() + i);
			continue;
		}
		++i;
	}

	// Check for nothing to do.
	if (OldRows.empty() && NewRows.empty())
	{
		return;
	}

	// Make a copy of the master database to work from.
	COMPATDB WorkDatabase = MasterDatabase;

	// Everything remaining in OldRows needs to be deleted from the master database.
	for (COMPATDB::const_iterator i = OldRows.begin(); i != OldRows.end(); ++i)
	{
		if (!WorkDatabase.DeleteRow(*i))
		{
			// Internal consistency issue if this assert trips.
			ASSERT(false);
		}
	}

	// Everything remaining in NewRows needs to be added to the master database.
	for (COMPATDB::const_iterator i = NewRows.begin(); i != NewRows.end(); ++i)
	{
		if (!WorkDatabase.AddRow(*i))
		{
			// Row add failed.
			// Notify the user.
			// Do not proceed.
			e_CompatDB_NotifyUserOfError("The rule in row %u couldn't be added to the master database due to an apparent conflict. Changes will not be saved to disk.", i->nLineNumber);	// CHB 2007.08.01 - String audit: development
			return;
		}
	}

	// Commit changes to master database.
	MasterDatabase.swap(WorkDatabase);
	TempDatabase.swap(AllNewRows);

	// The cached master database is now newer than the file on disk.
	FILETIME NowTime;
	::GetSystemTimeAsFileTime(&NowTime);
	LARGE_INTEGER li;
	li.LowPart = NowTime.dwLowDateTime;
	li.HighPart = NowTime.dwHighDateTime;
	nMasterDatabaseWriteTime = li.QuadPart;

	// Write changes to master database.
	const path_string OutFileName = e_CompatDB_GetMasterDatabaseFileName();
	bool bReadOnly = true;
	if (!e_CompatDB_IsFileReadOnly(bReadOnly, OutFileName))
	{
		e_CompatDB_NotifyUserOfError("Failed to get read-only status of database file \"%s\". Changes will not be saved to disk.", OutFileName.c_str());	// CHB 2007.08.01 - String audit: development
	}
	else if (bReadOnly)
	{
		// Read-only. Warn user we won't be saving.
		e_CompatDB_NotifyUserOfError("The database file \"%s\" is read only. Changes will not be saved to disk. (You may need to check the file out in Perforce.)", OutFileName.c_str());	// CHB 2007.08.01 - String audit: development
	}
	else
	{
		// Not read only...  go for it!
		bool bSuccess = false;
		ULONGLONG nNewTime = 0;
		for (;;)
		{
			const path_string OutFileNameNew = OutFileName + ".new";
			const path_string OutFileNameTmp = OutFileName + ".tmp";
			if (!MasterDatabase.SaveToFile(OutFileNameNew))
			{
				// Failed to save the file.
				break;
			}

			// Get time stamp of new file.
			if (!e_CompatDB_GetFileWriteTime(nNewTime, OutFileNameNew))
			{
				break;
			}

			// Delete previous temp file.
			if (!::DeleteFileA(OutFileNameTmp.c_str()))
			{
				// But if the file doesn't exist, that's okay.
				DWORD nError = ::GetLastError();
				if (nError != ERROR_FILE_NOT_FOUND)
				{
					break;
				}
			}

			// Rename original to temp.
			if (!::MoveFileA(OutFileName.c_str(), OutFileNameTmp.c_str()))
			{
				// But if the file doesn't exist, that's okay.
				DWORD nError = ::GetLastError();
				if (nError != ERROR_FILE_NOT_FOUND)
				{
					break;
				}
			}

			// Rename new to original.
			if (!::MoveFileA(OutFileNameNew.c_str(), OutFileName.c_str()))
			{
				// Undo -- move master DB back.
				::MoveFileA(OutFileNameTmp.c_str(), OutFileName.c_str());
				break;
			}

			// Delete temp copy.
			::DeleteFileA(OutFileNameTmp.c_str());

			bSuccess = true;
			break;
		}

		if (!bSuccess)
		{
			e_CompatDB_NotifyUserOfError("Error writing to master database file. Changes have not be saved to disk.");	// CHB 2007.08.01 - String audit: development
		}

		// Update the cache time stamp.
		nMasterDatabaseWriteTime = nNewTime;
	}

	// Process new master database.
	e_CompatDB_ApplyDatabase(MasterDatabase);
}
#endif

#ifdef E_COMPATDB_ENABLE_EDIT
bool E_COMPATDB_STATE::CacheMasterDatabase(void)
{
	// Get master database file name.
	const path_string FileName = e_CompatDB_GetMasterDatabaseFileName();

	// Get the file's current time stamp.
	ULONGLONG nCurrentTime;
	if (!e_CompatDB_GetFileWriteTimeOrZero(nCurrentTime, FileName))
	{
		e_CompatDB_NotifyUserOfError("Failed to get time stamp of master database file.");	// CHB 2007.08.01 - String audit: development
		return false;
	}

	// Check time stamp.
	if (nCurrentTime <= nMasterDatabaseWriteTime)
	{
		// File has not been updated, so our cache is current.
		return true;
	}

	// Read the database.
	COMPATDB LoadedDatabase;
	string_type ErrorMsg;
	if (!e_CompatDB_LoadMasterDatabase(LoadedDatabase, ErrorMsg, false))
	{
		ErrorMsg = "Reading master database file failed due to the error: " + ErrorMsg + ".";
		e_CompatDB_NotifyUserOfError(ErrorMsg.c_str());	// CHB 2007.08.01 - String audit: development
		return false;
	}

	// Cache it.
	MasterDatabase.swap(LoadedDatabase);
	nMasterDatabaseWriteTime = nCurrentTime;
	return true;
}
#endif

const COMPATDB & E_COMPATDB_STATE::GetMasterDatabase(void) const
{
	return MasterDatabase;
}

//-----------------------------------------------------------------------------

DBROW::DBROW(void)
{
	nTargetOp = COMPAREOP_NEVER;
	nTargetId = 0;
	nVendorOp = COMPAREOP_NEVER;
	nVendorId = 0;
	nDeviceOp1 = COMPAREOP_NEVER;
	nDeviceId1 = 0;
	nDeviceOp2 = COMPAREOP_NEVER;
	nDeviceId2 = 0;
	nSubsystemOp = COMPAREOP_NEVER;
	nSubsystemId = 0;
	nRevisionOp = COMPAREOP_NEVER;
	nRevisionId = 0;
	nDriverVersionOp = COMPAREOP_NEVER;
	nDriverVersion = 0;
	nAction = ACTION_EMPTY;
	//nAction = ACTION_NONE;
	nValueId = 0;
	nValueMin = 0;
	nValueMax = 0;
	nLineNumber = 0;
	bEnabled = true;
}

bool DBROW::AreCriteriaFieldsEqual(const DBROW & rhs) const
{
	return
		(nTargetOp == rhs.nTargetOp) &&
		(nTargetId == rhs.nTargetId) &&
		(nVendorOp == rhs.nVendorOp) &&
		(nVendorId == rhs.nVendorId) &&
		(nDeviceOp1 == rhs.nDeviceOp1) &&
		(nDeviceId1 == rhs.nDeviceId1) &&
		(nDeviceOp2 == rhs.nDeviceOp2) &&
		(nDeviceId2 == rhs.nDeviceId2) &&
		(nSubsystemOp == rhs.nSubsystemOp) &&
		(nSubsystemId == rhs.nSubsystemId) &&
		(nRevisionOp == rhs.nRevisionOp) &&
		(nRevisionId == rhs.nRevisionId) &&
		(nDriverVersionOp == rhs.nDriverVersionOp) &&
		(nDriverVersion == rhs.nDriverVersion);
}

bool DBROW::ArePersistentFieldsEqual(const DBROW & rhs) const
{
	return
		AreCriteriaFieldsEqual(rhs) &&
		(nAction == rhs.nAction) &&
		(nValueId == rhs.nValueId) &&
		(nValueMin == rhs.nValueMin) &&
		(nValueMax == rhs.nValueMax) &&
		(bEnabled == rhs.bEnabled) &&		// CHB 2007.01.05 - Is this a criteria field?
		(Comment == rhs.Comment);
}

bool DBROW::DoesApplyToAdapter(const VideoDeviceInfo & Adapter) const
{
	DWORD nCurrentTarget = e_GetEngineTarget4CC();

	if (!e_CompatDB_Compare(nTargetOp, nCurrentTarget, nTargetId))
	{
		return false;
	}
	if (!e_CompatDB_Compare(nVendorOp, Adapter.VendorID, nVendorId))
	{
		return false;
	}
	if (!e_CompatDB_Compare(nDeviceOp1, Adapter.DeviceID, nDeviceId1))
	{
		return false;
	}
	if (!e_CompatDB_Compare(nDeviceOp2, Adapter.DeviceID, nDeviceId2))
	{
		return false;
	}
	if (!e_CompatDB_Compare(nSubsystemOp, Adapter.SubSysID, nSubsystemId))
	{
		return false;
	}
	if (!e_CompatDB_Compare(nRevisionOp, Adapter.Revision, nRevisionId))
	{
		return false;
	}
	if (!e_CompatDB_Compare(nDriverVersionOp, static_cast<ULONGLONG>(Adapter.DriverVersion.QuadPart), nDriverVersion))
	{
		return false;
	}
	return true;
}

bool DBROW::DoesApplyToCurrentAdapter(void) const
{
	return DoesApplyToAdapter(e_CompatDB_GetCurrentAdapterIdentifier());
}

bool DBROW::DoActionsConflict(const DBROW & Other) const
{
	return e_CompatDB_AreSameAction(*this, Other);
}

// This would be a problem with signed types, but we're not using
// this with any right now.
template<class T>
T _Matches(COMPAREOP nOp, const T & nVal, const T & nMinVal, const T & nMaxVal)
{
	switch (nOp)
	{
		case COMPAREOP_ALWAYS:
		case COMPAREOP_NEVER:
		case COMPAREOP_NOTEQUAL:
		default:
			// Shouldn't get here.
			return nMaxVal - nMinVal;
		case COMPAREOP_EQUAL:
			return 1;
		case COMPAREOP_GREATER:
			return nMaxVal - nVal;
		case COMPAREOP_LESS:
			return nVal - nMinVal;
		case COMPAREOP_GREATER_OR_EQUAL:
			return nMaxVal - nVal + 1;
		case COMPAREOP_LESS_OR_EQUAL:
			return nVal - nMinVal + 1;
	}
}

// Basically we want to sort by the "precision" of the rule.
// The fewer the number of possible matches, the more precise the rule, the higher the precedence.
// The higher the number of possible matches, the less precise the rule, the lower the precedence.
template<class T>
bool _CompareLT(bool & bResultOut, COMPAREOP nOpL, COMPAREOP nOpR, const T & nValL, const T & nValR, const T & nMinVal, const T & nMaxVal)
{
	// Although "never" has 0 possible matches, this is considered imprecise.
	// Anything with "never" should drop to the bottom.
	if ((nOpL == COMPAREOP_NEVER) && (nOpR != COMPAREOP_NEVER))
	{
		bResultOut = true;		// lhs less than rhs
		return true;
	}
	if ((nOpL != COMPAREOP_NEVER) && (nOpR == COMPAREOP_NEVER))
	{
		bResultOut = false;		// lhs not less than rhs
		return true;
	}

	// Next is "always" which has infinite possible matches.
	if ((nOpL != COMPAREOP_ALWAYS) && (nOpR == COMPAREOP_ALWAYS))
	{
		bResultOut = true;		// lhs less than rhs
		return true;
	}
	if ((nOpL == COMPAREOP_ALWAYS) && (nOpR != COMPAREOP_ALWAYS))
	{
		bResultOut = false;		// lhs not less than rhs
		return true;
	}

	// Next is "not equal" which has infinity less one possible matches.
	if ((nOpL != COMPAREOP_NOTEQUAL) && (nOpR == COMPAREOP_NOTEQUAL))
	{
		bResultOut = true;		// lhs less than rhs
		return true;
	}
	if ((nOpL == COMPAREOP_NOTEQUAL) && (nOpR != COMPAREOP_NOTEQUAL))
	{
		bResultOut = false;		// lhs not less than rhs
		return true;
	}

	// Next are inequalities and equality, which have a finite number of possible matches.
	const T nMatchesL = _Matches(nOpL, nValL, nMinVal, nMaxVal);
	const T nMatchesR = _Matches(nOpR, nValR, nMinVal, nMaxVal);
	ASSERT((nMatchesL > 0) && (nMatchesR > 0));
	if (nMatchesL < nMatchesR)
	{
		bResultOut = true;		// lhs less than rhs
		return true;
	}
	if (nMatchesL > nMatchesR)
	{
		bResultOut = false;		// lhs not less than rhs
		return true;
	}

	// Huh, same number of matches. Let's sort on absolute value.
	if (nValL < nValR)
	{
		bResultOut = true;		// lhs less than rhs
		return true;
	}
	if (nValL > nValR)
	{
		bResultOut = false;		// lhs not less than rhs
		return true;
	}

	// Must be equal!
	return false;
}

const DWORD DWORD_MIN = static_cast<DWORD>( 0);
const DWORD DWORD_MAX = static_cast<DWORD>(-1);
const ULONGLONG ULONGLONG_MIN = static_cast<ULONGLONG>( 0);
const ULONGLONG ULONGLONG_MAX = static_cast<ULONGLONG>(-1);

bool DBROW::operator<(const DBROW & rhs) const
{
	bool bResult;
	if (_CompareLT(bResult, nTargetOp, rhs.nTargetOp, nTargetId, rhs.nTargetId, DWORD_MIN, DWORD_MAX))
	{
		return bResult;
	}
	if (_CompareLT(bResult, nVendorOp, rhs.nVendorOp, nVendorId, rhs.nVendorId, DWORD_MIN, DWORD_MAX))
	{
		return bResult;
	}
	if (_CompareLT(bResult, nDeviceOp1, rhs.nDeviceOp1, nDeviceId1, rhs.nDeviceId1, DWORD_MIN, DWORD_MAX))
	{
		return bResult;
	}
	if (_CompareLT(bResult, nDeviceOp2, rhs.nDeviceOp2, nDeviceId2, rhs.nDeviceId2, DWORD_MIN, DWORD_MAX))
	{
		return bResult;
	}
	if (_CompareLT(bResult, nSubsystemOp, rhs.nSubsystemOp, nSubsystemId, rhs.nSubsystemId, DWORD_MIN, DWORD_MAX))
	{
		return bResult;
	}
	if (_CompareLT(bResult, nRevisionOp, rhs.nRevisionOp, nRevisionId, rhs.nRevisionId, DWORD_MIN, DWORD_MAX))
	{
		return bResult;
	}
	if (_CompareLT(bResult, nDriverVersionOp, rhs.nDriverVersionOp, nDriverVersion, rhs.nDriverVersion, ULONGLONG_MIN, ULONGLONG_MAX))
	{
		return bResult;
	}

	// If we make it this far, they're considered equal.
	// It is imperative that a given set of rules in arbitrary
	// order be sorted to yield an identical sorted order each
	// time.  In order to do this, it is necessary that every
	// possible pair of rules is comparable, with the comparison
	// yielding an identical deterministic relative ordering
	// for every comparison of the pair.  That is to say, every
	// pair of rules must have a known fixed relative ordering.
	// The only exception is when the two rules are identical.
	// Thus, the only time the ordering comparator (this member
	// function) should find the two to be equal is when they are
	// in fact identical.  The code below verifies this in order
	// to ensure correctness of the comparator.
	ASSERT(AreCriteriaFieldsEqual(rhs));
	return false;
}

//-----------------------------------------------------------------------------

bool COMPATDB::AddRow(const DBROW & ToAdd)
{
	// Check to make sure row does not conflict with any existing rows.
	if (HasConflictingRow(ToAdd))
	{
		return false;
	}
	push_back(ToAdd);
	return true;
}

bool COMPATDB::DeleteRow(const DBROW & ToDelete)
{
	for (iterator i = begin(); i != end(); ++i)
	{
		if (i->ArePersistentFieldsEqual(ToDelete))
		{
			erase(i);
			return true;
		}
	}
	return false;
}

bool COMPATDB::GetApplicableRows(COMPATDB & DBOut, const VideoDeviceInfo & Adapter) const
{
	COMPATDB t;
	t.reserve(size());
	for (const_iterator i = begin(); i != end(); ++i)
	{
		if (i->DoesApplyToAdapter(Adapter))
		{
			t.push_back(*i);
		}
	}
	DBOut.swap(t);
	return true;
}

bool COMPATDB::GetApplicableRows(COMPATDB & DBOut) const
{
	return GetApplicableRows(DBOut, e_CompatDB_GetCurrentAdapterIdentifier());
}

bool COMPATDB::WriteToFile(File & f) const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		string_type Line = e_CompatDB_ComposeCSVLine(*i, sAnyText);
		if (!f.WriteLine(Line))
		{
			return false;
		}
	}
	return true;
}

bool COMPATDB::SaveToFile(const path_string & FileName) const
{
	File f;
	if (!f.OpenForWritingText(FileName))
	{
		return false;
	}
	if (!f.WriteLine(e_CompatDB_IntToString(DB_FORMAT_VERSION)))
	{
		return false;
	}
	if (!f.WriteLine(e_CompatDB_GetHeaderRow()))
	{
		return false;
	}
	return WriteToFile(f);
}

void COMPATDB::SortBySpecificity(void)
{
	std::sort(begin(), end());

	// Verify the sorted ordering.
#if ISVERSION(DEVELOPMENT)
	if (size() > 1)
	{
		for (const_iterator i = begin(); i + 1 != end(); ++i)
		{
			ASSERT((*i < *(i + 1)) || i->AreCriteriaFieldsEqual(*(i + 1)));
		}
	}
#endif
}

// "Conflicting" is defined as same criteria and same action.
COMPATDB::const_iterator COMPATDB::FindConflictingRow(const DBROW & Row) const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		if (i->DoActionsConflict(Row) && i->AreCriteriaFieldsEqual(Row))
		{
			return i;
		}
	}
	return end();
}

bool COMPATDB::HasConflictingRow(const DBROW & Row) const
{
	return FindConflictingRow(Row) != end();
}

bool COMPATDB::HasRowWithSameAction(const DBROW & Row) const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		if (i->DoActionsConflict(Row))
		{
			return true;
		}
	}
	return false;
}

void COMPATDB::SeparateBySpecificity(COMPATDB & SpecificDatabase, COMPATDB & VagueDatabase, COMPATDB & DisabledDatabase) const
{
	COMPATDB Copy = *this;
	Copy.SortBySpecificity();

	COMPATDB Specific, Vague, Disabled;
	for (COMPATDB::const_iterator i = Copy.begin(); i != Copy.end(); ++i)
	{
		if (!i->bEnabled)
		{
			Disabled.push_back(*i);
		}
		else if (Specific.HasRowWithSameAction(*i))
		{
			Vague.push_back(*i);
		}
		else
		{
			Specific.push_back(*i);
		}
	}
	SpecificDatabase.swap(Specific);
	VagueDatabase.swap(Vague);
	DisabledDatabase.swap(Disabled);
}

//-----------------------------------------------------------------------------

MemoryTextReader::MemoryTextReader(const void * pSrcIn, unsigned nBytesIn)
{
	pPtr = static_cast<const string_type::value_type *>(pSrcIn);
	nChars = nBytesIn / sizeof(string_type::value_type);
}

MemoryTextReader::~MemoryTextReader(void)
{
}

bool MemoryTextReader::ReadLine(string_type & StrOut)
{
	if (nChars == 0)
	{
		return false;
	}

	unsigned i;
	for (i = 0; i < nChars; ++i)
	{
		if ((pPtr[i] == '\r') || (pPtr[i] == '\n'))
		{
			break;
		}
	}

	StrOut = string_type(pPtr, i);

	if (((i + 1) < nChars) && (pPtr[i] == '\r') && (pPtr[i + 1] == '\n'))
	{
		++i;
	}
	++i;

	ASSERT(nChars >= i);
	pPtr += i;
	nChars -= i;

	return true;
}

FileTextReader::FileTextReader(File & fIn)
	: f(fIn)
{
}

FileTextReader::~FileTextReader(void)
{
}

bool FileTextReader::ReadLine(string_type & StrOut)
{
	return f.ReadLine(StrOut);
}

//-----------------------------------------------------------------------------

};

//-----------------------------------------------------------------------------
//
// Public Functions
//
//-----------------------------------------------------------------------------

void e_CompatDB_Init(void)
{
	delete s_pState;
	s_pState = new E_COMPATDB_STATE;

	// Read the database.
	COMPATDB LoadedDatabase;
	string_type ErrorMsg;
	if (!e_CompatDB_LoadMasterDatabase(LoadedDatabase, ErrorMsg, true))
	{
		#ifdef E_COMPATDB_ENABLE_EDIT
		ErrorMsg = "Reading master database file failed due to the error: " + ErrorMsg + ".";
		e_CompatDB_NotifyUserOfError(ErrorMsg.c_str());	// CHB 2007.08.01 - String audit: development
		#endif
		return;
	}

	// Apply the rules.
	e_CompatDB_ApplyDatabase(LoadedDatabase);
}

//-----------------------------------------------------------------------------

void e_CompatDB_Deinit(void)
{
	delete s_pState;
	s_pState = 0;
}

//-----------------------------------------------------------------------------
// CML 2007.1.19: exposed for fillpak
void * e_CompatDB_LoadMasterDatabaseFile( DWORD * pdwBytesRead /*= NULL*/ )
{
	path_string FileName = e_CompatDB_GetMasterDatabaseFileName();
	DECLARE_LOAD_SPEC(spec, FileName.c_str());
	//// don't add it to pak if we're running fillpak client - it'll get added later.
	//if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) && AppGetState() != APP_STATE_FILLPAK) {

	//} else {
	//	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	//}
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	void * pData = PakFileLoadNow(spec);
	if ( pdwBytesRead )
		*pdwBytesRead = spec.bytesread;
	return pData;
}

//-----------------------------------------------------------------------------

#ifdef E_COMPATDB_ENABLE_EDIT
void e_CompatDB_DoEdit(void)
{
	// Check to see if master database file has read-only attribute set.
	bool bReadOnly = true;
	if (!e_CompatDB_IsFileReadOnly(bReadOnly, e_CompatDB_GetMasterDatabaseFileName()))
	{
		e_CompatDB_NotifyUserOfError("Error obtaining read-only status of master database file.");	// CHB 2007.08.01 - String audit: development
	}
	else if (bReadOnly)
	{
		e_CompatDB_NotifyUserOfWarning("Master database file is read only; changes will not be saved. (Need to check the file out in Perforce?)");	// CHB 2007.08.01 - String audit: development
	}

	//
	if (!s_pState->CacheMasterDatabase())
	{
		//e_CompatDB_NotifyUserOfError("Error reading master database file. Edit operation canceled.");	// CHB 2007.08.01 - String audit: development, commented
		return;
	}

	//
	COMPATDB ApplicableRules;
	if (!s_pState->GetMasterDatabase().GetApplicableRows(ApplicableRules))
	{
		//
		return;
	}

	// Create directory.
	path_string FileName = e_CompatDB_GetTemporaryFilePath();
	::CreateDirectoryA(FileName.c_str(), 0);

	// Write file.
	FileName = e_CompatDB_GetTemporaryFileName();
	File f;
	if (f.OpenForWritingText(FileName))
	{
		//
		COMPATDB SpecificDatabase, VagueDatabase, DisabledDatabase;
		ApplicableRules.SeparateBySpecificity(SpecificDatabase, VagueDatabase, DisabledDatabase);

		//
		f.WriteLine("// These rules apply to this platform.");
		f.WriteLine(e_CompatDB_GetHeaderRow());
		SpecificDatabase.WriteToFile(f);

		f.WriteNewLine();
		f.WriteLine("// These rules would apply to this platform in the absence of the above more specific rules.");
		f.WriteLine(e_CompatDB_GetHeaderRow());
		VagueDatabase.WriteToFile(f);

		f.WriteNewLine();
		f.WriteLine("// These rules may apply to this platform if enabled.");
		f.WriteLine(e_CompatDB_GetHeaderRow());
		DisabledDatabase.WriteToFile(f);

		f.WriteNewLine();
		f.WriteLine("// Add new rules for this platform here.");
		f.WriteLine(e_CompatDB_GetHeaderRow());

		VideoDeviceInfo AdaptId = e_CompatDB_GetCurrentAdapterIdentifier();

		DBROW New;
		New.nTargetOp = COMPAREOP_EQUAL;
		New.nTargetId = e_GetEngineTarget4CC();
		New.nVendorOp = COMPAREOP_EQUAL;
		New.nVendorId = AdaptId.VendorID;
		New.nDeviceOp1 = COMPAREOP_EQUAL;
		New.nDeviceId1 = AdaptId.DeviceID;
		New.nDeviceOp2 = COMPAREOP_ALWAYS;
		New.nDeviceId2 = 0;
		New.nSubsystemOp = COMPAREOP_EQUAL;
		New.nSubsystemId = AdaptId.SubSysID;
		New.nRevisionOp = COMPAREOP_EQUAL;
		New.nRevisionId = AdaptId.Revision;
		New.nDriverVersionOp = COMPAREOP_EQUAL;
		New.nDriverVersion = AdaptId.DriverVersion.QuadPart;
		New.Comment = AdaptId.szDeviceName;
		string_type Line = e_CompatDB_ComposeCriteria(New, sAnyText);
		f.WriteLine(Line);
		f.WriteLine(Line);
		f.WriteLine(Line);

		f.Close();

		//
		COMPATDB CombinedDatabase;
		CombinedDatabase.reserve(SpecificDatabase.size() + VagueDatabase.size() + DisabledDatabase.size());
		CombinedDatabase.insert(CombinedDatabase.end(), SpecificDatabase.begin(), SpecificDatabase.end());
		CombinedDatabase.insert(CombinedDatabase.end(), VagueDatabase.begin(), VagueDatabase.end());
		CombinedDatabase.insert(CombinedDatabase.end(), DisabledDatabase.begin(), DisabledDatabase.end());
		s_pState->TempDatabase.swap(CombinedDatabase);
	}

	// Set up monitoring.
	if (!s_pState->WatchFile(FileName))
	{
		return;
	}

	// Edit it.
	::ShellExecuteA(0, "open", FileName.c_str(), 0, 0, SW_SHOWNORMAL);
}
#endif

//-----------------------------------------------------------------------------

#ifdef E_COMPATDB_ENABLE_EDIT
void e_CompatDB_CheckForUpdates(void)
{
	s_pState->CheckForUpdates();
}
#endif

//-----------------------------------------------------------------------------

#else

//-----------------------------------------------------------------------------

void e_CompatDB_Init() { return; }
void e_CompatDB_Deinit() { return; }
void* e_CompatDB_LoadMasterDatabaseFile( DWORD * /*pdwBytesRead*/ ) { return NULL; }
#ifdef E_COMPATDB_ENABLE_EDIT
void e_CompatDB_DoEdit() { return; }
void e_CompatDB_CheckForUpdates() { return; }
#endif

//-----------------------------------------------------------------------------

#endif
