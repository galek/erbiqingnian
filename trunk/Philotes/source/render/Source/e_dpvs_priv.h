//----------------------------------------------------------------------------
// e_dpvs_priv.h
//
// - Header for engine-only dPVS interface code
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_DPVS_PRIV__
#define __E_DPVS_PRIV__

#ifdef ENABLE_DPVS

#include "dpvs.hpp"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//#define DPVS_ENABLE_TRACE

//#define DPVS_DEBUG_OCCLUDER

void  e_DPVS_Trace					( const char * szStr, ... );
#if ISVERSION(DEVELOPMENT)
#	ifdef DPVS_ENABLE_TRACE
#		define DPVSTRACE( str, ... )	e_DPVS_Trace( str, __VA_ARGS__ )
#	else
#		define DPVSTRACE( str, ... )
#	endif
#else
#	define DPVSTRACE( str, ... )
#endif


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

class DPVSCommander : public DPVS::Commander
{
public:
	DPVSCommander()										{}
	virtual			~DPVSCommander()					{}
	virtual void	command(Command);
private:

	DPVSCommander				(const DPVSCommander&);
	DPVSCommander& operator=	(const DPVSCommander&);
};

class DPVSServices : public DPVS::LibraryDefs::Services
{
public:
	DPVSServices  (void)								{}
	virtual         ~DPVSServices	(void)				{}
	virtual void    error			(const char* message)
	{
		if (message)                        // display message and die
		{
			trace( "dPVS FATAL: %s", message);
		}
		assert(false);              
	}

private:
	DPVSServices  (const DPVSServices&);
	DPVSServices& operator=       (const DPVSServices&);
};

//----------------------------------------------------------------------------
// DEBUG-ONLY commander

class DPVSDebugCommander : public DPVS::Commander
{
public:
	DPVSDebugCommander()								{}
	virtual			~DPVSDebugCommander()				{}
	virtual void	command(Command);
private:

	DPVSDebugCommander				(const DPVSDebugCommander&);
	DPVSDebugCommander& operator=	(const DPVSDebugCommander&);
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern DPVSCommander*			gpDPVSCommander;
extern DPVSServices*			gpDPVSServices;
extern DPVSDebugCommander *		gpDPVSDebugCommander;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void 		e_DPVS_ModelSetCell				( int nModelID, DPVS::Cell * pCell );
void 		e_DPVS_ModelSetObjToCellMatrix	( int nModelID, MATRIX & tMatrix );
void 		e_DPVS_ModelSetTestModel		( int nModelID, DPVS::Model * pCullModel );
void 		e_DPVS_ModelSetWriteModel		( int nModelID, DPVS::Model * pWriteModel );
void 		e_DPVS_ModelSetVisibilityParent	( int nChildModelID, int nParentModelID );

void		e_DPVS_DebugRestoreTexture		();
PRESULT		e_DPVS_DebugUpdateTexture		( const unsigned char * pbyData, int nWidth, int nHeight );

#endif // ENABLE_DPVS

#endif // __E_DPVS_PRIV__
