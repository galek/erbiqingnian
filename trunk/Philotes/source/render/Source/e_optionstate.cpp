//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#include "e_pch.h"
#include "e_optionstate.h"
#include "e_settings.h"		// CHB !!! - for e_SettingsCommitActionFlags()
#include "settingsexchange.h"
#include "safe_vector.h"
#undef min	// some things really shouldn't be macros
#undef max	// some things really shouldn't be macros
#include <limits>

//-----------------------------------------------------------------------------

#ifdef _DEBUG
#define DEBUG_ASSERT(e) ASSERT(e)
#else
#define DEBUG_ASSERT(e) ((void)0)
#endif

//-----------------------------------------------------------------------------

enum ParmType	// was hoping to avoid this, but dynamic interaction with initdb pretty much forces it
{
	// The order of these must match TypeInfo[]
	OPTIONSTATE_TYPE_bool,
	OPTIONSTATE_TYPE_float,
	OPTIONSTATE_TYPE_int,
	OPTIONSTATE_TYPE_unsigned,
	OPTIONSTATE_TYPE_COUNT,
};

/*
union Types
{
#define DEF_PARM3(type, line) type v_##type##line;
#define DEF_PARM2(type, line) DEF_PARM3(type, line)
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t) DEF_PARM2(type, __LINE__)
#include "e_optionstate_def.h"
#undef DEF_PARM3
#undef DEF_PARM2
#undef DEF_PARMT
};
*/

union TypeUnion
{
	bool b;
	float f;
	int i;
	unsigned u;
};

struct TypeElem
{
	const TypeUnion (*pToUnion)(const void * pVal);
	float (*pToFloat)(const void * pVal);
	const TypeUnion (*pFromFloat)(float fVal);
	bool (*pIsLessThan)(const void * pLHS, const void * pRHS);
	void (*pGetMinMax)(TypeUnion & Min, TypeUnion & Max);
};

extern const TypeElem TypeInfo[OPTIONSTATE_TYPE_COUNT];

//-----------------------------------------------------------------------------

struct OptionState::State
{
	ParmData Current;
	ParmData Deferred;
	ParmData Min;
	ParmData Max;
};

struct OptionState::ParmElem
{
	unsigned short nOffset;
	unsigned char nSize;
	unsigned char nType;
	const char * pName;
};

const OptionState::ParmElem OptionState::ParmInfo[] =
{
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t) { offsetof(OptionState::ParmData, name), sizeof(type), OPTIONSTATE_TYPE_##type, #name },
#include "e_optionstate_def.h"
#undef DEF_PARMT
};

//-----------------------------------------------------------------------------

class OptionState_Module
{
	public:
		CComPtr<OptionState> pActive;
		int nCommitSuspendCount;

		typedef safe_vector<OptionState_EventHandler *> EventHandlerVector;
		EventHandlerVector Handlers;

		static PRESULT Clone(const OptionState * pState, OptionState * * ppStateOut);
		static PRESULT CopyDeferrables(const OptionState * pSourceState, OptionState * pDestinationState);
		void Update(OptionState * pState) const;
		unsigned QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState) const;
		PRESULT CommitToActive(const OptionState * pNewState);
		static void _DoUpdate(OptionState * pState);

		void Reset(void);
		OptionState_Module(void);
		~OptionState_Module(void);
};

static OptionState_Module * s_pModule = 0;

//-----------------------------------------------------------------------------

void OptionState::_DoUpdate(void)
{
	//
	ASSERT(!bUpdating);
	bUpdating = true;
	ASSERT(!bDeferredUpdate);

	//
	bDeferredUpdate = true;
	BOUNDED_WHILE(bDeferredUpdate, 100)
	{
		bDeferredUpdate = false;
		s_pModule->Update(this);
	}

	//
	bDeferredUpdate = false;
	bUpdating = false;
}

void OptionState::Update(void)
{
	if ((nSuspendCount > 0) || bUpdating)
	{
		bDeferredUpdate = true;
	}
	else
	{
		ASSERT(nSuspendCount == 0);
		_DoUpdate();
	}
}

void OptionState::SuspendUpdate(void)
{
	++nSuspendCount;
}

void OptionState::ResumeUpdate(void)
{
	ASSERT_RETURN(nSuspendCount > 0);
	if (--nSuspendCount == 0)
	{
		if (bDeferredUpdate)
		{
			bDeferredUpdate = false;
			_DoUpdate();
		}
	}
}

//-----------------------------------------------------------------------------

const OptionState::ParmData & OptionState::GetCurrent(void) const
{
	return pData->Current;
}

//-----------------------------------------------------------------------------

static
bool sIsLessThan(unsigned char nType, const void * pLHS, const void * pRHS)
{
	DEBUG_ASSERT((0 <= nType) && (nType < OPTIONSTATE_TYPE_COUNT));
	return (*TypeInfo[nType].pIsLessThan)(pLHS, pRHS);
}

static
const TypeUnion sToUnion(unsigned char nType, const void * pVal)
{
	DEBUG_ASSERT((0 <= nType) && (nType < OPTIONSTATE_TYPE_COUNT));
	return (*TypeInfo[nType].pToUnion)(pVal);
}

static
float sToFloat(unsigned char nType, const void * pVal)
{
	DEBUG_ASSERT((0 <= nType) && (nType < OPTIONSTATE_TYPE_COUNT));
	return (*TypeInfo[nType].pToFloat)(pVal);
}

static
const TypeUnion sFromFloat(unsigned char nType, float fVal)
{
	DEBUG_ASSERT((0 <= nType) && (nType < OPTIONSTATE_TYPE_COUNT));
	return (*TypeInfo[nType].pFromFloat)(fVal);
}

static
void sGetMinMax(unsigned char nType, TypeUnion & Min, TypeUnion & Max)
{
	DEBUG_ASSERT((0 <= nType) && (nType < OPTIONSTATE_TYPE_COUNT));
	return (*TypeInfo[nType].pGetMinMax)(Min, Max);
}

//-----------------------------------------------------------------------------

#define _INDEX(p, n, c) (static_cast<c char *>(static_cast<c void *>(p)) + (n))

// Returns true if the value changed.
bool OptionState::_Set(const ParmElem * pInfo, ParmData & Data, const void * pVal)
{
	DEBUG_ASSERT(pInfo->nOffset < sizeof(Data));
	DEBUG_ASSERT((pInfo->nOffset + pInfo->nSize) <= sizeof(Data));
	void * pTarg = _INDEX(&Data, pInfo->nOffset, /**/);
	if (memcmp(pTarg, pVal, pInfo->nSize) != 0)
	{
		memcpy(pTarg, pVal, pInfo->nSize);
		return true;
	}
	return false;
}

const void * OptionState::_Get(const ParmElem * pInfo, ParmData & Data) const
{
	DEBUG_ASSERT(pInfo->nOffset < sizeof(Data));
	DEBUG_ASSERT((pInfo->nOffset + pInfo->nSize) <= sizeof(Data));
	return _INDEX(&Data, pInfo->nOffset, const);
}

#undef _INDEX

void OptionState::_set(const ParmElem * pInfo, const void * pVal, bool bDefer)
{
	// Check min & max.
	TypeUnion uVal = sToUnion(pInfo->nType, pVal);
	if (sIsLessThan(pInfo->nType, &uVal, _Get(pInfo, pData->Min)))
	{
		uVal = sToUnion(pInfo->nType, _Get(pInfo, pData->Min));
	}
	if (sIsLessThan(pInfo->nType, _Get(pInfo, pData->Max), &uVal))
	{
		uVal = sToUnion(pInfo->nType, _Get(pInfo, pData->Max));
	}

	// Don't permit deferral during initialization phase.
	bDefer = bDefer && (s_pModule->nCommitSuspendCount < 1);	// we'll key off s_pModule->nCommitSuspendCount for now

	// Always update deferred value.
	// And be sure to update it first, because the
	// Update() below can change things.
	_Set(pInfo, pData->Deferred, &uVal);

	// Update the main value unless deferred.
	if (!bDefer)
	{
		if (_Set(pInfo, pData->Current, &uVal))
		{
			Update();
		}
	}
}

void OptionState::_set(ParmId nItem, const void * pVal, bool bDefer)
{
	DEBUG_ASSERT((0 <= nItem) && (nItem < OPTIONSTATE_PARM_COUNT));
	return _set(&ParmInfo[nItem], pVal, bDefer);
}

//-----------------------------------------------------------------------------

const OptionState::ParmElem * OptionState::GetInfo(const char * pName)
{
	// O(n), but n is small and we only use this for the init DB
	for (int i = 0; i < _countof(ParmInfo); ++i)
	{
		if (strcmp(pName, ParmInfo[i].pName) == 0)
		{
			// Match.
			return &ParmInfo[i];
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
		
bool OptionState::HasValueNamed(const char * pName)
{
	return GetInfo(pName) != 0;
}

//-----------------------------------------------------------------------------

void OptionState::_SetAuto(const ParmElem * pInfo, ParmData & Data, float fValue)
{
	DEBUG_ASSERT(pInfo != 0);

	TypeUnion uVal = sFromFloat(pInfo->nType, fValue);

	if (&Data == &pData->Current)
	{
		_set(pInfo, &uVal, false);
	}
	else
	{
		_Set(pInfo, Data, &uVal);
	}
}

//-----------------------------------------------------------------------------

bool OptionState::SetByNameAuto(const char * pName, float fValue)
{
	const ParmElem * pInfo = GetInfo(pName);
	ASSERT_RETFALSE(pInfo != 0);
	_SetAuto(pInfo, pData->Current, fValue);
	return true;
}

//-----------------------------------------------------------------------------

bool OptionState::_GetSetMinMaxByNameAuto(const char * pName, float & fValue, bool bSet, bool bMax)
{
	const ParmElem * pInfo = GetInfo(pName);
	ASSERT_RETFALSE(pInfo != 0);
	ParmData & Data = bMax ? pData->Max : pData->Min;
	if (bSet)
	{
		_SetAuto(pInfo, Data, fValue);
	}
	else
	{
		fValue = sToFloat(pInfo->nType, _Get(pInfo, Data));
	}
	return true;
}

bool OptionState::GetMinByNameAuto(const char * pName, float & fValue) const
{
	return const_cast<OptionState *>(this)->_GetSetMinMaxByNameAuto(pName, fValue, false, false);
}

bool OptionState::GetMaxByNameAuto(const char * pName, float & fValue) const
{
	return const_cast<OptionState *>(this)->_GetSetMinMaxByNameAuto(pName, fValue, false, true);
}

bool OptionState::SetMinByNameAuto(const char * pName, float fMin)
{
	// Don't permit min > max
	float fMax = 0.0f;
	ASSERT_RETFALSE(GetMaxByNameAuto(pName, fMax));
	ASSERT_RETFALSE(fMin <= fMax);
	return _GetSetMinMaxByNameAuto(pName, fMin, true, false);
	// Should also adjust value to [min, max] here.
}

bool OptionState::SetMaxByNameAuto(const char * pName, float fMax)
{
	// Don't permit min > max
	float fMin = 0.0f;
	ASSERT_RETFALSE(GetMinByNameAuto(pName, fMin));
	ASSERT_RETFALSE(fMin <= fMax);
	return _GetSetMinMaxByNameAuto(pName, fMax, true, true);
	// Should also adjust value to [min, max] here.
}

//-----------------------------------------------------------------------------

bool OptionState::HasDeferrals(void) const
{
	return memcmp(&pData->Current, &pData->Deferred, sizeof(pData->Current)) != 0;
}

//-----------------------------------------------------------------------------

void OptionState::Exchange(SettingsExchange & se)
{
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t)\
	if (pers)\
	{\
		SettingsExchange_Do(se, pData->Deferred.name, ParmInfo[OPTIONSTATE_PARM_##name].pName);\
		if (se.IsLoading())\
		{\
			pData->Current.name = pData->Deferred.name;\
		}\
	}
#include "e_optionstate_def.h"
#undef DEF_PARMT
}

void OptionState::_Init(void)
{
	pData = new State;
	nSuspendCount = 0;
	bDeferredUpdate = false;
	bUpdating = false;
}

OptionState::OptionState(void)
{
	_Init();
	::ZeroMemory(pData, sizeof(*pData));

	// Init min and max.
	for (int i = 0; i < _countof(ParmInfo); ++i)
	{
		const ParmElem * const pInfo = &ParmInfo[i];
		TypeUnion Min, Max;
		sGetMinMax(pInfo->nType, Min, Max);
		_Set(pInfo, pData->Min, &Min);
		_Set(pInfo, pData->Max, &Max);
	}
}

OptionState::OptionState(const OptionState & r)
{
	_Init();
	*pData = *r.pData;
}

OptionState & OptionState::operator=(const OptionState & r)
{
	*pData = *r.pData;
	return *this;
}

OptionState::~OptionState(void)
{
	delete pData;
}

//-----------------------------------------------------------------------------

void e_OptionState_SuspendCommit(void)
{
	++s_pModule->nCommitSuspendCount;
}

void e_OptionState_ResumeCommit(void)
{
	DEBUG_ASSERT(s_pModule->nCommitSuspendCount);
	--s_pModule->nCommitSuspendCount;
}

unsigned e_OptionState_QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState)
{
	return s_pModule->QueryActionRequired(pOldState, pNewState);
}

PRESULT e_OptionState_CommitToActive(const OptionState * pNewState)
{
	return s_pModule->CommitToActive(pNewState);
}

const OptionState * e_OptionState_GetActive(void)
{
	return s_pModule->pActive;
}

PRESULT e_OptionState_Clone(const OptionState * pSourceState, OptionState * * ppStateOut)
{
	return s_pModule->Clone(pSourceState, ppStateOut);
}

PRESULT e_OptionState_CloneActive(OptionState * * ppStateOut)
{
	return e_OptionState_Clone(e_OptionState_GetActive(), ppStateOut);
}

PRESULT e_OptionState_RegisterEventHandler(OptionState_EventHandler * pEventHandler)
{
	s_pModule->Handlers.push_back(pEventHandler);
	return S_OK;
}

static	// currently private
PRESULT e_OptionState_CopyDeferrables(const OptionState * pSourceState, OptionState * pState)
{
	return s_pModule->CopyDeferrables(pSourceState, pState);
}

PRESULT e_OptionState_CopyDeferrablesFromActive(OptionState * pState)
{
	return e_OptionState_CopyDeferrables(e_OptionState_GetActive(), pState);
}

//-----------------------------------------------------------------------------

static
void e_OptionState_Exchange(SettingsExchange & se, void * pContext)
{
	OptionState * pState = static_cast<OptionState *>(pContext);
	pState->Exchange(se);
}

PRESULT e_OptionState_LoadFromFile(const OS_PATH_CHAR * pFileName, const char * pName, OptionState * pState)
{
	V_RETURN(SettingsExchange_LoadFromFile(pFileName, pName, &e_OptionState_Exchange, pState));
	s_pModule->_DoUpdate(pState);
	return S_OK;
}

PRESULT e_OptionState_SaveToFile(const OS_PATH_CHAR * pFileName, const char * pName, const OptionState * pState)
{
	V_RETURN(SettingsExchange_SaveToFile(pFileName, pName, &e_OptionState_Exchange, const_cast<OptionState *>(pState)));
	return S_OK;
}

//-----------------------------------------------------------------------------

void OptionState_Module::_DoUpdate(OptionState * pState)
{
	pState->_DoUpdate();
}

unsigned OptionState_Module::QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState) const
{
	// Determine actions needed.
	unsigned nActionFlagsCumulative = 0;
	for (EventHandlerVector::const_iterator i = Handlers.begin(); i != Handlers.end(); ++i)
	{
		unsigned nActionFlags = 0;
		(*i)->QueryActionRequired(pActive, pNewState, nActionFlags);
		nActionFlagsCumulative |= nActionFlags;
	}
	return nActionFlagsCumulative;
}

PRESULT OptionState_Module::CommitToActive(const OptionState * pNewState)
{
	// New state must be updated and not suspended.
	ASSERT_RETVAL(pNewState->nSuspendCount == 0, E_FAIL);
//	pNewState->_DoUpdate();

	if (s_pModule->nCommitSuspendCount == 0)
	{
		// Determine actions needed.
		unsigned nActionFlagsCumulative = QueryActionRequired(pActive, pNewState);

		// Application exit should be detected and handled beforehand
		// by the application.  It doesn't make sense to commit a
		// state that requires the application to have exited.
		ASSERT((nActionFlagsCumulative & SETTING_NEEDS_APP_EXIT) == 0);

		// Send commit event.
		for (EventHandlerVector::const_iterator i = Handlers.begin(); i != Handlers.end(); ++i)
		{
			PRESULT nResult = (*i)->Commit(pActive, pNewState);
			V_RETURN(nResult);
		}

		// Handle action flags.
		*pActive = *pNewState;	// CHB !!! - this is a temporary workaround because currently committing action flags depends on the use of 'live' state info
		V_RETURN(e_SettingsCommitActionFlags(nActionFlagsCumulative, true));	// CHB !!!
	}

	// Save to active state.
	*pActive = *pNewState;

	// Return.
	return S_OK;
}

PRESULT OptionState_Module::Clone(const OptionState * pState, OptionState * * ppStateOut)
{
	CComPtr<OptionState> pNewState = new OptionState(*pState);
	*ppStateOut = pNewState;
	pNewState.p->AddRef();
	return S_OK;
}

PRESULT OptionState_Module::CopyDeferrables(const OptionState * pSourceState, OptionState * pDestinationState)
{
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t)\
	if (defer)\
	{\
		pDestinationState->pData->Current.name = pSourceState->pData->Current.name;\
	}
#include "e_optionstate_def.h"
#undef DEF_PARMT
	return S_OK;
}

void OptionState_Module::Update(OptionState * pState) const
{
	for (EventHandlerVector::const_iterator i = Handlers.begin(); i != Handlers.end(); ++i)
	{
		(*i)->Update(pState);
	}
}

void OptionState_Module::Reset(void)
{
	for (EventHandlerVector::iterator i = Handlers.begin(); i != Handlers.end(); ++i)
	{
		delete *i;
		*i = 0;
	}
	Handlers.clear();
}

OptionState_Module::OptionState_Module(void)
{
	pActive = new OptionState;
	nCommitSuspendCount = 1;
}

OptionState_Module::~OptionState_Module(void)
{
	Reset();
}

//-----------------------------------------------------------------------------

BOOL e_OptionState_IsInitialized(void)
{
	return s_pModule != NULL;
};

void e_OptionState_InitModule(void)
{
	delete s_pModule;
	s_pModule = new OptionState_Module;
}

PRESULT e_OptionState_Deinit(void)
{
	delete s_pModule;
	s_pModule = 0;
	return S_OK;
}

//-----------------------------------------------------------------------------

void OptionState_EventHandler::Update(OptionState * /*pState*/)
{
}

PRESULT OptionState_EventHandler::Commit(const OptionState * /*pOldState*/, const OptionState * /*pNewState*/)
{
	return S_FALSE;
}

void OptionState_EventHandler::QueryActionRequired(const OptionState * /*pOldState*/, const OptionState * /*pNewState*/, unsigned & /*nActionFlagsOut*/)
{
}

//-----------------------------------------------------------------------------

template<class T>
const TypeUnion _ToUnion(const T & nVal)
{
	C_ASSERT(sizeof(T) <= sizeof(TypeUnion));
	TypeUnion tu;
	*static_cast<T *>(static_cast<void *>(&tu)) = nVal;
	return tu;
}

template<class T>
const TypeUnion _ToUnion(const void * pVal)
{
	return _ToUnion(*static_cast<const T *>(pVal));
}

template<class T>
float _ToFloat(const void * pVal)
{
	return static_cast<float>(*static_cast<const T *>(pVal));
}

template<class T>
const TypeUnion _FromFloat(float fVal)
{
	return _ToUnion<T>(static_cast<T>(fVal));
}

template<>
const TypeUnion _FromFloat<bool>(float fVal)	// specialization
{
	TypeUnion tu;
	tu.b = (fVal != 0.0f);
	return tu;
}

template<class T>
bool _IsLessThan(const void * pLHS, const void * pRHS)
{
	return *static_cast<const T *>(pLHS) < *static_cast<const T *>(pRHS);
}

template<class T>
void _GetMinMax(TypeUnion & Min, TypeUnion & Max)
{
	Min = _ToUnion(std::numeric_limits<T>::min());
	Max = _ToUnion(std::numeric_limits<T>::max());
}

template<>
void _GetMinMax<float>(TypeUnion & Min, TypeUnion & Max)	// specialization
{
	Max.f = std::numeric_limits<float>::max();
	Min.f = -Max.f;
}

//-----------------------------------------------------------------------------

#define _TYPE(t) { &_ToUnion<t>, &_ToFloat<t>, &_FromFloat<t>, &_IsLessThan<t>, &_GetMinMax<t> },
const TypeElem TypeInfo[OPTIONSTATE_TYPE_COUNT] =
{
	// The order of these must match enum ParmType
	_TYPE(bool)
	_TYPE(float)
	_TYPE(int)
	_TYPE(unsigned)
};
