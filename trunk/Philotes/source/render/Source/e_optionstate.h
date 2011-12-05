//*****************************************************************************
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//*****************************************************************************

#ifndef __E_OPTIONSTATE_H__
#define __E_OPTIONSTATE_H__

/*
	Discussion
	----------

	Imagine the user is tweaking graphics options in the game's
	UI.  Some of the settings are interdependent -- for example,
	an overall lighting quality setting may require a reduced-
	feature shader, which doesn't support shadows.  Given that,
	it is desired to allow the user to tweak the settings and
	receive instant feedback in the UI.  This system attempts to
	address this requirement.

	The "OptionState" refers to the embodiment of whatever
	parameters are necessary for engine configuration into a
	state object.  Implementation as an independent state object
	addresses the following design goals:

	*	A central location from which to store and retrieve
		engine configuration parameters.

	*	The ability to "play with" configuration parameters in a
		"sandbox" fashion, and receive relevant feedback regarding
		configuration limitations, during user option editing
		sessions, without affecting the parameters currently in
		use by the engine.

	*	The ability to commit or discard parameter edits as desired.

	The OptionState does not necessarily correlate directly to user
	editable features; those would be handled by EngineSettings and
	FeatureLine.  Instead, the OptionState reflects the engine-side
	parameters necessary to implement the features.  For example,
	the FeatureLine might include an overall lighting quality
	setting that the user can tweak.  The FeatureLine system then
	sets the OptionState accordingly, perhaps setting the effective
	shader model.

	OptionState also performs whatever validation logic is necessary
	on the parameters; in effect, implementing the parameter inter-
	dependency.  For example, if the effective shader model is
	changed, the shadow capability may need to change as well.

	The OptionState system also includes the concept of the "active"
	OptionState, which is the one currently in active use by the
	engine.  It can only be changed by committing a new state.
*/

#include "e_irefcount.h"

//
// The OptionState object is the embodiment of all user-tweakable
// (and other related and interdependent) engine and graphics options.
//

class OptionState_Module;
class SettingsExchange;

class OptionState : public IRefCount
{
	private:
		enum ParmId
		{
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t) OPTIONSTATE_PARM_##name,
#include "e_optionstate_def.h"
#undef DEF_PARMT
			OPTIONSTATE_PARM_COUNT
		};

		struct ParmData
		{
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t) type name;
#include "e_optionstate_def.h"
#undef DEF_PARMT
		};

	// Accessors.
	public:
#define DEF_PARMT1(type, name, pers, defer, t)\
		inline t get_##name(void) const { return static_cast<t>(GetCurrent().name); }\
		inline void set_##name(const t v) { _set(OPTIONSTATE_PARM_##name, &v, defer); }\
		typedef t type_##name;
#if defined(ENGINE_TARGET_DX9)
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t) DEF_PARMT1(type, name, pers, defer, dx9t)
#elif defined(ENGINE_TARGET_DX10)
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t) DEF_PARMT1(type, name, pers, defer, dx10t)
#else
#define DEF_PARMT(type, name, pers, defer, dx9t, dx10t) DEF_PARMT1(type, name, pers, defer, type)
#endif
#include "e_optionstate_def.h"
#undef DEF_PARMT1
#undef DEF_PARMT

		static bool HasValueNamed(const char * pName);
		bool SetByNameAuto(const char * pName, float fValue);	// automatically converts float value to appropriate type

		bool GetMinByNameAuto(const char * pName, float & fValue) const;
		bool GetMaxByNameAuto(const char * pName, float & fValue) const;
		bool SetMinByNameAuto(const char * pName, float fValue);
		bool SetMaxByNameAuto(const char * pName, float fValue);

	//
	public:
		//
		bool HasDeferrals(void) const;

		// Use to defer updating, for example when setting multiple
		// related parameters.  (Set frame buffer width and height.)
		void SuspendUpdate(void);
		void ResumeUpdate(void);

		//
		void Exchange(SettingsExchange & se);

	private:
		OptionState(void);
		OptionState(const OptionState & r);
		OptionState & operator=(const OptionState & r);

	protected:
		virtual ~OptionState(void);

	private:
		void _Init(void);
		void _DoUpdate(void);
		void Update(void);
		const ParmData & GetCurrent(void) const;

		struct State;
		State * pData;
		int nSuspendCount;
		bool bDeferredUpdate;
		bool bUpdating;

		struct ParmElem;
		static const ParmElem ParmInfo[];

		static const ParmElem * GetInfo(const char * pName);
		void _set(const ParmElem * pInfo, const void * pVal, bool bDefer);
		void _set(ParmId nItem, const void * pVal, bool bDefer);
		const void * _Get(const ParmElem * pInfo, ParmData & Data) const;
		bool _Set(const ParmElem * pInfo, ParmData & Data, const void * pVal);
		void _SetAuto(const ParmElem * pInfo, ParmData & Data, float fValue);
		bool _GetSetMinMaxByNameAuto(const char * pName, float & fValue, bool bSet, bool bMax);

	friend class OptionState_Module;
};

// Get the active state.
// Returning a const object is intentional.
// It should NEVER be necessary to use non-const operations on the active state.
// To make changes, clone the active state, make the changes, then commit the new state.
const OptionState * e_OptionState_GetActive(void);

// Clone the active state to a new state object.
// It's the caller's responsibility to call Release() on the new state object when finished.
PRESULT e_OptionState_CloneActive(OptionState * * ppStateOut);

// Returns the actions required (a la SETTING_NEEDS_*) to transition
// from one state to another.
unsigned e_OptionState_QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState);

// Commit the state to become the new active state.
// Note, if this functions fails, the active state may be out of
// sync with the actual engine state.  In this case, a new state
// should be committed until the function succeeds.  (You could
// re-commit the active state.)
PRESULT e_OptionState_CommitToActive(const OptionState * pNewState);

// Load & save from persistent storage.
PRESULT e_OptionState_LoadFromFile(const OS_PATH_CHAR * pFileName, const char * pName, OptionState * pState);
PRESULT e_OptionState_SaveToFile(const OS_PATH_CHAR * pFileName, const char * pName, const OptionState * pState);

// Initialize.
BOOL e_OptionState_IsInitialized(void);
PRESULT e_OptionState_Init(void);
PRESULT e_OptionState_InitCaps(void);
PRESULT e_OptionState_Deinit(void);

// These are used to suspend and resume committing actions.
// (The active state object is updated, however.)
// This is used during initialization, when the configuration
// needs to be accessed and updated before the relevant engine
// systems have been initialized.  There should be no need to
// use these otherwise.
void e_OptionState_SuspendCommit(void);
void e_OptionState_ResumeCommit(void);

// Clone a state to a new state object.
// It's the caller's responsibility to call Release() on the new state object when finished.
PRESULT e_OptionState_Clone(const OptionState * pSourceState, OptionState * * ppStateOut);

//
PRESULT e_OptionState_CopyDeferrablesFromActive(OptionState * pState);

//
// The event handler allows other engine systems to receive
// notification of and implement OptionState-systemwide events.
//

class OptionState_EventHandler
{
	public:
		// Obligatory virtual destructor.
		virtual ~OptionState_EventHandler(void) throw() {}

		// The purpose of Update() is to validate the state, and enforce
		// internal state consistency.  Update() is called each time a
		// state variable is modified, giving all interested systems a
		// chance to ensure the the new setting correctly fits in with
		// the state as a whole.  This implies it should never be possible
		// for the state to be invalid.  Commit() should be able to use
		// the state without any further validation or modification.
		// Indeed, Commit() is prohibited from modifying the state.
		virtual void Update(OptionState * pState);

		// Commit callbacks are used to implement committing of options.
		// For example, the frame buffer module would register a callback
		// that compared the screen dimensions of the old and new states.
		// If they differed, it would perform the work of actually adjusting
		// the frame buffer size.  Any action flags returned are accumulated
		// and performed following the successful conclusion of all commit
		// callbacks.
		// On success, return S_FALSE if no action was taken, or S_OK otherwise.
		virtual PRESULT Commit(const OptionState * pOldState, const OptionState * pNewState);

		// Performs no action other than returning what action flags would
		// be required to switch from the old state to the new state.  This
		// can be used ahead of time to determine what would be required of
		// a hypothetical state change, and is also used during the Commit()
		// process for actually performing the actions after committing.
		virtual void QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut);
};

PRESULT e_OptionState_RegisterEventHandler(OptionState_EventHandler * pEventHandler);

#endif
