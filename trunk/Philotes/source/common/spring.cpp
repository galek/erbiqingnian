//----------------------------------------------------------------------------
// spring.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------



#include "spring.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define SPRING_UPDATES_PER_SEC		100

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

const float cgfSpringUpdateInterval = 1.f / SPRING_UPDATES_PER_SEC;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Symmetrical spring
//
//----------------------------------------------------------------------------

static inline void sSpringUpdate( SPRING * s, float fSecs, float fForce )
{
	// this should operate on a more-or-less fixed rate, so the *= for dampen is close enough

	// compute instantaneous force
	float f;
	f			= fForce;
	if ( fabs(f) < s->nul )
		f		= 0.f;
	f			*= s->k;
	s->vel		+= f * fSecs;
	s->vel		*= s->d;
	s->cur		+= s->vel;
}

void SpringUpdate( SPRING * pSpring, float fSecs, const float * pfForce )
{
	float fInterval = cgfSpringUpdateInterval;
	BOUNDED_WHILE( fSecs > fInterval, INT_MAX )
	{
		sSpringUpdate( pSpring, fInterval, pfForce ? *pfForce : ( pSpring->tgt - pSpring->cur ) );
		fSecs -= fInterval;
	}

	sSpringUpdate( pSpring, fSecs, pfForce ? *pfForce : ( pSpring->tgt - pSpring->cur ) );
}

//----------------------------------------------------------------------------
//
// Asymmetrical spring
//
//----------------------------------------------------------------------------

static inline void sSpringUpdate( ASYMMETRIC_SPRING * s, float fSecs, float fForce )
{
	// this should operate on a more-or-less fixed rate, so the *= for dampen is close enough

	// compute instantaneous force
	float f;
	f			= fForce;
	if ( fabs(f) < s->nul )
		f		= 0.f;
	SPRING_DIRECTION eDir = ( f < 0.f ) ? SPRING_DIR_NEG : SPRING_DIR_POS;
	f			*= s->k[ eDir ];
	s->vel		+= f * fSecs;
	eDir		= ( s->vel < 0.f ) ? SPRING_DIR_NEG : SPRING_DIR_POS;
	s->vel		*= s->d[ eDir ];
	s->cur		+= s->vel;
}

void SpringUpdate( ASYMMETRIC_SPRING * pSpring, float fSecs, const float * pfForce )
{
	float fInterval = cgfSpringUpdateInterval;
	BOUNDED_WHILE( fSecs > fInterval, INT_MAX )
	{
		sSpringUpdate( pSpring, fInterval, pfForce ? *pfForce : ( pSpring->tgt - pSpring->cur ) );
		fSecs -= fInterval;
	}

	sSpringUpdate( pSpring, fSecs, pfForce ? *pfForce : ( pSpring->tgt - pSpring->cur ) );
}

//----------------------------------------------------------------------------
//
// Simple lerp-towards-target spring
//
//----------------------------------------------------------------------------

static inline void sSpringUpdate( LERP_SPRING * s, float fSecs )
{
	// this should operate on a more-or-less fixed rate

	float f;
	f			= s->k * fSecs;
	f			= min( f, 1.f );
	f			= max( f, 0.f );
	if ( fabs(f) < s->nul )
		f		= 0.f;
	s->cur		= LERP( s->cur, s->tgt, f );
}

void SpringUpdate( LERP_SPRING * pSpring, float fSecs )
{
	sSpringUpdate( pSpring, fSecs );
}
