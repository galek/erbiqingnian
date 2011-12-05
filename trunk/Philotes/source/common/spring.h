//----------------------------------------------------------------------------
// spring.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _SPRING_H_
#define _SPRING_H_

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum SPRING_DIRECTION
{
	SPRING_DIR_BOTH = -1,
	SPRING_DIR_NEG = 0,
	SPRING_DIR_POS,
	// count
	NUM_SPRING_DIRS // duh, always 2
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS/CLASSES
//----------------------------------------------------------------------------

// symmetrical Hooke spring
struct SPRING
{
	// attributes
	float k;		// strength
	float d;		// dampen
	float tgt;		// target value
	float nul;		// null zone

	// status
	float cur;		// current value
	float vel;		// current velocity (per second)
};

// asymmetrical Hooke spring
struct ASYMMETRIC_SPRING
{
	static const int scnDirs = NUM_SPRING_DIRS;

	// attributes
	float k[ scnDirs ];		// strength
	float d[ scnDirs ];		// dampen
	float tgt;				// target value
	float nul;				// null zone

	// status
	float cur;				// current value
	float vel;				// current velocity (per second)
};

// simple lerp-towards-target spring
struct LERP_SPRING
{
	// attributes
	float k;		// strength
	float tgt;		// target value
	float nul;		// null zone

	// status
	float cur;		// current value
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Templatized, general spring funcs
//
//----------------------------------------------------------------------------

template <class T>
inline void SpringInit( T * pSpring )
{
	ZeroMemory( pSpring, sizeof(T) );
}

template <class T>
inline void SpringSetTarget( T * pSpring, float fTarget )
{
	pSpring->tgt = fTarget;
}

template <class T>
inline void SpringSetCurrent( T * pSpring, float fCurrent )
{
	pSpring->cur = fCurrent;
}

template <class T>
inline void SpringSetNullZone( T * pSpring, float fNullZone )
{
	pSpring->nul = fNullZone;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Simple, symmetrical spring
//
//----------------------------------------------------------------------------

template void SpringInit		<SPRING>	( SPRING * pSpring );
template void SpringSetTarget	<SPRING>	( SPRING * pSpring, float fTarget );
template void SpringSetCurrent	<SPRING>	( SPRING * pSpring, float fCurrent );
template void SpringSetNullZone	<SPRING>	( SPRING * pSpring, float fNullZone );

inline void SpringSetStrength( SPRING * pSpring, float fStrength )
{
	pSpring->k = fStrength;
}

inline void SpringSetDampen( SPRING * pSpring, float fDampen )
{
	pSpring->d = fDampen;
}

void SpringUpdate( SPRING * pSpring, float fSecs, const float * pfForce = NULL );

//----------------------------------------------------------------------------
//
// Asymmetrical spring
//
//----------------------------------------------------------------------------

template void SpringInit		<ASYMMETRIC_SPRING>		( ASYMMETRIC_SPRING * pSpring );
template void SpringSetTarget	<ASYMMETRIC_SPRING>		( ASYMMETRIC_SPRING * pSpring, float fTarget );
template void SpringSetCurrent	<ASYMMETRIC_SPRING>		( ASYMMETRIC_SPRING * pSpring, float fCurrent );
template void SpringSetNullZone	<ASYMMETRIC_SPRING>		( ASYMMETRIC_SPRING * pSpring, float fNullZone );

inline void SpringSetStrength( ASYMMETRIC_SPRING * pSpring, float fStrength, SPRING_DIRECTION eDir = SPRING_DIR_BOTH )
{
	if ( eDir == SPRING_DIR_BOTH )
	{
		for ( int i=0; i < pSpring->scnDirs; i++ )
			pSpring->k[ i ] = fStrength;
	}
	else
	{
		ASSERT_RETURN ( (int)eDir < pSpring->scnDirs && (int)eDir >= 0 );
		pSpring->k[ eDir ] = fStrength;
	}
}

inline void SpringSetDampen( ASYMMETRIC_SPRING * pSpring, float fDampen, SPRING_DIRECTION eDir = SPRING_DIR_BOTH )
{
	if ( eDir == SPRING_DIR_BOTH )
	{
		for ( int i=0; i < pSpring->scnDirs; i++ )
			pSpring->d[ i ] = fDampen;
	}
	else
	{
		ASSERT_RETURN ( (int)eDir < pSpring->scnDirs && (int)eDir >= 0 );
		pSpring->d[ eDir ] = fDampen;
	}
}


void SpringUpdate( ASYMMETRIC_SPRING * pSpring, float fSecs, const float * pfForce = NULL );

//----------------------------------------------------------------------------
//
// Simple, lerp-towards-target spring
//
//----------------------------------------------------------------------------

template void SpringInit		<LERP_SPRING>		( LERP_SPRING * pSpring );
template void SpringSetTarget	<LERP_SPRING>		( LERP_SPRING * pSpring, float fTarget );
template void SpringSetCurrent	<LERP_SPRING>		( LERP_SPRING * pSpring, float fCurrent );
template void SpringSetNullZone	<LERP_SPRING>		( LERP_SPRING * pSpring, float fNullZone );

inline void SpringSetStrength( LERP_SPRING * pSpring, float fStrength )
{
	pSpring->k = fStrength;
}

inline void SpringSetDampen( LERP_SPRING * pSpring, float fDampen, SPRING_DIRECTION eDir )
{
	UNREFERENCED_PARAMETER(pSpring);
	UNREFERENCED_PARAMETER(fDampen);
	UNREFERENCED_PARAMETER(eDir);
}

void SpringUpdate( LERP_SPRING * pSpring, float fSecs );



#endif // _SPRING_H_
