//----------------------------------------------------------------------------
// FILE: effect.cpp
// DESC: One shot effects
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "e_model.h"
#include "effect.h"
#include "filepaths.h"
#include "s_message.h"
#include "unit_priv.h" // also includes units.h

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_EffectAtUnit(
	UNIT *pUnit,
	EFFECT_TYPE eEffectType)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// setup message
	MSG_SCMD_EFFECT_AT_UNIT tMessage;
	tMessage.idUnit = UnitGetId( pUnit );
	tMessage.eEffectType = eEffectType;
	
	// send message to all with room
	s_SendUnitMessage( pUnit, SCMD_EFFECT_AT_UNIT, &tMessage );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_EffectAtLocation(
	GAME *pGame,
	ROOM *pRoom,
	const VECTOR *pvLocation,
	EFFECT_TYPE eEffectType)
{
	ASSERTX_RETURN( pvLocation, "Expected location vector" );
	
	// setup message
	MSG_SCMD_EFFECT_AT_LOCATION tMessage;
	tMessage.vPosition = *pvLocation;
	tMessage.eEffectType = eEffectType;
	
	// send to all with room
	s_SendMessageToAllWithRoom( pGame, pRoom, SCMD_EFFECT_AT_LOCATION, &tMessage );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDoParticleAtUnit( 
	UNIT *pUnit, 
	int idParticleDef)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( idParticleDef != INVALID_ID, "Invalid particle definition id" );

	// get model
	int idModel = c_UnitGetModelId( pUnit );
	
	// setup attachment definition
	ATTACHMENT_DEFINITION tAttachDef;
	tAttachDef.eType = ATTACHMENT_PARTICLE;

	// set attachment
	tAttachDef.nAttachedDefId = idParticleDef;
//	tAttachDef.pszAttached[ MAX_XML_STRING_LENGTH ];
	
	// set position to be center of model for now
	int nModelDefinition = e_ModelGetDefinition( idModel );
	if ( nModelDefinition != INVALID_ID )
	{
		const BOUNDING_BOX * pBoundingBox = e_ModelDefinitionGetBoundingBox( nModelDefinition, LOD_ANY );
		if (pBoundingBox)
		{
			tAttachDef.vPosition = BoundingBoxGetCenter( pBoundingBox );
		}
		
	}

	// do it!
	c_ModelAttachmentAdd( 
		idModel, 
		tAttachDef, 
		FILE_PATH_DATA, 
		FALSE, 
		ATTACHMENT_OWNER_NONE, 
		INVALID_LINK, 
		INVALID_ID);
		
}

//----------------------------------------------------------------------------
struct EFFECT_SPEC
{	
	UNIT *pUnit;
	VECTOR vLocation;
	EFFECT_TYPE eEffectType;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDoEffect(
	EFFECT_SPEC *pSpec)
{
	ASSERTX_RETURN( pSpec, "Expected effect spec" );
	UNIT *pUnit = pSpec->pUnit;
	const UNIT_DATA *pUnitData = NULL;
	
	// get unit data (if unit is present)
	if (pSpec->pUnit)
	{
		pUnitData = UnitGetData( pUnit );
		ASSERTX_RETURN( pUnitData, "Can't find unit data" );
	}
		
	switch (pSpec->eEffectType)
	{
	
		//----------------------------------------------------------------------------	
		case ET_RESTORE_VITALS:
		{
			ASSERTX_BREAK( pUnitData, "Can't find unit data" );
			int idParticleDef = pUnitData->nRestoreVitalsParticleId;
			c_sDoParticleAtUnit( pUnit, idParticleDef );
			break;
		}
		
		//----------------------------------------------------------------------------
		default:
		{
			break;
		}
		
	}
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_EffectAtUnit(
	UNIT *pUnit,
	EFFECT_TYPE eEffectType)
{
	
	// setup request
	EFFECT_SPEC tSpec;
	tSpec.pUnit = pUnit;
	tSpec.vLocation = UnitGetPosition( pUnit );
	tSpec.eEffectType = eEffectType;
	
	// do effect
	c_sDoEffect( &tSpec );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_EffectAtLocation(
	const VECTOR *pvLocation,
	EFFECT_TYPE eEffectType)
{

	// setup request
	EFFECT_SPEC tSpec;
	tSpec.pUnit = NULL;
	tSpec.vLocation = *pvLocation;
	tSpec.eEffectType = eEffectType;
	
	// do effect
	c_sDoEffect( &tSpec );

}
	
