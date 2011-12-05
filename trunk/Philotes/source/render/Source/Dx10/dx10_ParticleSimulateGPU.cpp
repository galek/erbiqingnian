//----------------------------------------------------------------------------
// dx10_ParticleSimulateGPU.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "e_pch.h"
#include "dxC_model.h"
#include "dxC_buffer.h"
#include "dxC_target.h"
#include "dxC_particle.h"
#include "dxC_state.h"
#include "dxC_VertexDeclarations.h"
#include "e_particles_priv.h"
#include "dx10_ParticleSimulateGPU.h"
#include "dx10_fluid.h"
#include "excel.h"

#include "e_main.h"
#include "definition.h"

#include "c_particles.h"
#include "dxC_particle.h"

#include "e_optionstate.h"	// CHB 2007.09.12

static bool sgbInitialized = FALSE; 
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Sarah, this is the function that you need to call
//
//extern PFN_PARTICLES_GET_NEARBY_UNITS gpfn_ParticlesGetNearbyUnits;
//
//typedef int (* PFN_PARTICLES_GET_NEARBY_UNITS)(
//	int nMaxUnits, 
//	const VECTOR & vCenter, 
//	float fRadius, 
//	VECTOR * pvPositions, 
//	VECTOR * pvHeightsRadiiSpeeds, 
//	VECTOR * pvDirections );



#define MAX_PARTICLE_BUFFERS	2

//hash for saving the effect and the streamOutVB
struct PARTICLE_SYSTEM_GPU
{
	int							nId;
	PARTICLE_SYSTEM_GPU*		pNext;

	CFluid*                      pFluid;

	unsigned char currentBuffer;
	int nParticleBufferIDs[ MAX_PARTICLE_BUFFERS ];
	bool newFrame;
	bool hasBeenUpdatedThisFrame;

	D3D_EFFECT* pEffect;

	float runningTime;
	GPUPS_PARAMETERS tParams;

};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define MAX_GPU_PARTICLES 10240*6
#define RANDOM_TEXTURE_SIZE 1024
SPD3DCTEXTURE1D g_pRandomTexture = 0;
SPD3DCSHADERRESOURCEVIEW g_pRandomView = 0;

CHash<PARTICLE_SYSTEM_GPU> sgtGPUParticleSystems;

PRESULT dx10_ParticleSystemsGPUInit()
{
	HashInit(sgtGPUParticleSystems, NULL, 10);

	////
	//create a random data buffer used for controlling spawn rate
	BYTE* source[ RANDOM_TEXTURE_SIZE ];

	//fill with random data
	RAND tRand;
	RandInit( tRand );
	for( unsigned int x = 0; x < RANDOM_TEXTURE_SIZE; x++ )
		((BYTE*)source)[x] = (BYTE) RandGetNum( tRand, 255 );

	V_RETURN( dx10_Create1DTexture( RANDOM_TEXTURE_SIZE, 1, D3DC_USAGE_1DTEX, D3DFMT_L8, 
		&g_pRandomTexture, source, 0 ) );

	V_RETURN( CreateSRVFromTex1D( g_pRandomTexture, &g_pRandomView, 1 ) );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sFreeParticleSystemGPU(
	PARTICLE_SYSTEM_GPU * pGPUParticleSystem )
{
	if ( pGPUParticleSystem->pFluid )
	{
		FREE_DELETE( NULL, pGPUParticleSystem->pFluid, CFluid );
		return S_OK;
	}

	for ( UINT i = 0; i < MAX_PARTICLE_BUFFERS; i++ )
	{
		V( dxC_VertexBufferDestroy( pGPUParticleSystem->nParticleBufferIDs[ i ] ) );
		pGPUParticleSystem->nParticleBufferIDs[ i ] = INVALID_ID;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_ParticleSystemsFree()
{
	PARTICLE_SYSTEM_GPU * pGPUParticleSystem = (PARTICLE_SYSTEM_GPU*) HashGetFirst(sgtGPUParticleSystems);
	while(pGPUParticleSystem)
	{
		V( sFreeParticleSystemGPU( pGPUParticleSystem ) );
		
		pGPUParticleSystem = (PARTICLE_SYSTEM_GPU*) HashGetNext(sgtGPUParticleSystems,pGPUParticleSystem);
	}

	HashFree(sgtGPUParticleSystems);

	g_pRandomTexture = NULL;
	g_pRandomView = NULL;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_ParticleSystemGPUDestroy(int nId)
{
	PARTICLE_SYSTEM_GPU* pGPUParticleSystem = (PARTICLE_SYSTEM_GPU* )HashGet(sgtGPUParticleSystems,nId);
	if ( pGPUParticleSystem )
	{
		V( sFreeParticleSystemGPU( pGPUParticleSystem ) );
		HashRemove( sgtGPUParticleSystems, nId );
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PRESULT sFluidSystemInit( 
	PARTICLE_SYSTEM* pParticleSystem,
	PARTICLE_SYSTEM_GPU* pGPUParticleSystem)
{  
	if ( pGPUParticleSystem->pFluid )
		return S_OK;

	if ( ! e_OptionState_GetActive()->get_bFluidEffects() )
		return S_FALSE;

	PARTICLE_SYSTEM_DEFINITION * pDefinition = pParticleSystem->pDefinition;
	ASSERT_RETFAIL( pDefinition );
	if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_FLUID ) == 0 )
		return S_OK;

	D3D_EFFECT * pRenderEffect = e_ParticleSystemGetEffect( pDefinition );
	if ( !pRenderEffect || !TESTBIT_DWORD( pRenderEffect->dwFlags, EFFECT_FLAG_LOADED ) )
		return S_FALSE;

	EFFECT_TECHNIQUE * pRenderTechnique = e_ParticleSystemGetTechnique( pDefinition, pRenderEffect );
	if ( ! pRenderTechnique )
		return S_FALSE;
	ASSERT_RETFAIL( pRenderTechnique->hHandle );

	V( dxC_SetTechnique( pRenderEffect, pRenderTechnique, NULL ) ); 
	//initialize fluid
	pGPUParticleSystem->pFluid = MALLOC_NEW( NULL, CFluid );
	pGPUParticleSystem->pFluid->Initialize( pDefinition->nGridWidth, 
		pDefinition->nGridHeight, 
		pDefinition->nGridDepth,
		pDefinition->nDensityTextureId,
		pDefinition->nVelocityTextureId,
		pDefinition->nObstructorTextureId,
		pDefinition->nGridDensityTextureIndex,
		pDefinition->nGridVelocityTextureIndex,
		pDefinition->nGridObstructorTextureIndex,
		pDefinition->fRenderScale,
		//pParticleSystem->vPosition,
		(D3DXMATRIX*)&(pParticleSystem->mWorld),
		pRenderEffect, pRenderTechnique );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_ParticleSystemGPUInitialize( PARTICLE_SYSTEM* pParticleSystem )
{  
	ASSERT( NULL == HashGet( sgtGPUParticleSystems, pParticleSystem->nId ) );

	PARTICLE_SYSTEM_DEFINITION * pDefinition = pParticleSystem->pDefinition;
	SHADER_TYPE_DEFINITION * pShaderTypeDef = (SHADER_TYPE_DEFINITION*)ExcelGetData( EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, pDefinition->nShaderType );
	ASSERTV_RETFAIL( pShaderTypeDef, "Failed to get shadertypedef for index %d for particle system def %s!", pDefinition->nShaderType, pDefinition->tHeader.pszName );

	if ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_FLUID )
	{
		PARTICLE_SYSTEM_GPU* pGPUParticleSystem = HashAdd(sgtGPUParticleSystems, pParticleSystem->nId);
	    if( !pGPUParticleSystem )
		    return E_FAIL;

		sFluidSystemInit( pParticleSystem, pGPUParticleSystem );

		for ( UINT i = 0; i < MAX_PARTICLE_BUFFERS; i++ )
		{
			pGPUParticleSystem->nParticleBufferIDs[ i ] = INVALID_ID;
		}
	}
	else
	{
		PARTICLE_SYSTEM_GPU* pGPUParticleSystem = HashAdd(sgtGPUParticleSystems, pParticleSystem->nId);
		if( !pGPUParticleSystem )
			return E_FAIL;

		//create two buffers for particle system
		D3D_VERTEX_BUFFER_DEFINITION tVBDef;
		dxC_VertexBufferDefinitionInit( tVBDef );
		tVBDef.tUsage = D3D10_USAGE_BUFFER_STREAMOUT;
		tVBDef.eVertexType = VERTEX_DECL_PARTICLE_SIMULATION;
		tVBDef.nVertexCount = MAX_GPU_PARTICLES;
		tVBDef.nBufferSize[ 0 ] = tVBDef.nVertexCount * dxC_GetVertexSize( 0, &tVBDef );

		for ( UINT i = 0; i < MAX_PARTICLE_BUFFERS; i++ )
		{
			tVBDef.nD3DBufferID[ 0 ] = INVALID_ID; // Reset, so that we can create a second buffer.
			V_RETURN( dxC_CreateVertexBuffer( 0, tVBDef, NULL) );
			pGPUParticleSystem->nParticleBufferIDs[ i ] = tVBDef.nD3DBufferID[ 0 ];
		}

		pGPUParticleSystem->currentBuffer = 0;
		pGPUParticleSystem->newFrame = TRUE;
	}

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT dx10_ParticleSystemPrepareEmitter(D3D_EFFECT* pEffect, int nParticleInstanceId)
{
	PARTICLE_SYSTEM_GPU* pGPUParticleSystem = HashGet(sgtGPUParticleSystems,nParticleInstanceId);

	if( !pGPUParticleSystem )
		return S_FALSE;

	PARTICLE_SYSTEM* pParticleSystem = e_ParticleSystemGet( pGPUParticleSystem->nId );
	if ( !pParticleSystem )
	{
		V_RETURN( dx10_ParticleSystemGPUDestroy( pGPUParticleSystem->nId ) );
		return S_FALSE;
	}
	PARTICLE_SYSTEM_DEFINITION* pDefinition = pParticleSystem->pDefinition;

	if( pGPUParticleSystem->newFrame )
	{
		const int unsigned offsets = 0;
		
		D3D_VERTEX_BUFFER* pBuffer = dxC_VertexBufferGet( 
			pGPUParticleSystem->nParticleBufferIDs[ pGPUParticleSystem->currentBuffer ] );
		LPD3DCVB pD3DBuffer = pBuffer->pD3DVertices;

		dxC_GetDevice()->SOSetTargets(1, &pD3DBuffer, &offsets);

		pGPUParticleSystem->newFrame = FALSE;
	}

	//temporary until effects are split into 3 seperate effects. spawn/advance/draw
	pGPUParticleSystem->pEffect = pEffect;

	//bind random texture
	EFFECT_TECHNIQUE* pTechnique = &pEffect->ptTechniques[ pEffect->pnTechniquesIndex[ 0 ] ]; // Phase 1
	V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_GPUPS_TEXRANDOM, g_pRandomView) );

	float fPathTime = pParticleSystem->fPathTime;
	GPUPS_PARAMETERS& tParams = pGPUParticleSystem->tParams;
	tParams.fDeltaTime = e_GetElapsedTime();
	tParams.fRunningTime = fPathTime;

	CFloatPair tValPair = pDefinition->tParticleDurationPath.GetValue(fPathTime);
	tParams.duration.fMax = tValPair.y;
	tParams.duration.fMin = tValPair.x;

	tValPair = pDefinition->tLaunchSpeed.GetValue(fPathTime);
	tParams.launchSpeed.fMax = tValPair.y;
	tParams.launchSpeed.fMin = tValPair.x;

	tValPair = pDefinition->tParticlesPerMeterPerSecond.GetValue(fPathTime);
	tParams.ratePPMS.fMax = tValPair.y;
	tParams.ratePPMS.fMin = tValPair.x;

	tValPair = pDefinition->tLaunchOffsetZ.GetValue(fPathTime);
	tParams.launchOffsetZ.fMax = tValPair.y;
	tParams.launchOffsetZ.fMin = tValPair.x;

	tValPair = pDefinition->tLaunchScale.GetValue(fPathTime);
	tParams.launchScale.fMax = tValPair.y;
	tParams.launchScale.fMin = tValPair.x;

	tValPair = pDefinition->tLaunchSphereRadius.GetValue(fPathTime);
	tParams.launchSphereRadius.fMax = tValPair.y;
	tParams.launchSphereRadius.fMin = tValPair.x;

	V( dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_GPUPS_PARAMS, (float*)&tParams, 0, sizeof( tParams ) ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT dx10_ParticleSystemGPUAdvance(
	int nParticleInstanceId )
{	
	PARTICLE_SYSTEM_GPU* pGPUParticleSystem = 
		(PARTICLE_SYSTEM_GPU*)HashGet( sgtGPUParticleSystems, nParticleInstanceId );

	if( !pGPUParticleSystem ) 
		return S_FALSE;

	PARTICLE_SYSTEM* pParticleSystem = e_ParticleSystemGet( pGPUParticleSystem->nId );
	if ( !pParticleSystem )
	{
		V_RETURN( dx10_ParticleSystemGPUDestroy( pGPUParticleSystem->nId ) );
		return S_FALSE;
	}

	D3D_EFFECT* pEffect = pGPUParticleSystem->pEffect;
	EFFECT_TECHNIQUE* pTechnique = &pEffect->ptTechniques[ pEffect->pnTechniquesIndex[ 1 ] ]; // Phase 2
	V_RETURN( dxC_SetVertexDeclaration( VERTEX_DECL_PARTICLE_SIMULATION, pEffect ) );

	float fPathTime = pParticleSystem->fPathTime;

	GPUPS_PARAMETERS& tParams = pGPUParticleSystem->tParams;
	tParams.fDeltaTime = e_GetElapsedTime();
	tParams.fRunningTime = fPathTime;

	PARTICLE_SYSTEM_DEFINITION* pDefinition = pParticleSystem->pDefinition;
	CFloatPair tValPair = pDefinition->tParticleWorldAccelerationZ.GetValue( fPathTime );
	tParams.zAcceleration.fMax = tValPair.y;
	tParams.zAcceleration.fMin = tValPair.x;
	V( dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_GPUPS_PARAMS, (float*)&tParams, 0, sizeof( tParams ) ) );

	unsigned char nextBuffer = 
		( pGPUParticleSystem->currentBuffer + 1 ) % MAX_PARTICLE_BUFFERS;
	D3D_VERTEX_BUFFER* pBuffer = dxC_VertexBufferGet( 
		pGPUParticleSystem->nParticleBufferIDs[ nextBuffer ] );

	D3D_VERTEX_BUFFER_DEFINITION tVBDef;
	tVBDef.eVertexType = VERTEX_DECL_PARTICLE_SIMULATION;
	unsigned int stride = dxC_GetVertexSize( 0, &tVBDef, NULL );
	V_RETURN( dxC_SetStreamSource( 0, pBuffer->pD3DVertices, 0, stride ) );
	dxC_GetDevice()->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );

	V_RETURN( dxC_SetTechnique( pEffect, pTechnique, NULL ) ); 
	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );
	V_RETURN( gpStateManager->ApplyBeforeRendering( pEffect ) );

	dxC_GetDevice()->DrawAuto();

	return S_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_FillParticleInterpolationValues( const CInterpolationPath<CFloatPair>& tParticleField, 
										   GPUPS_FLOAT4& tParamTime, GPUPS_FLOAT4& tParamValue )
{	
	int nDataPointCount = tParticleField.GetPointCount();
	ASSERT_RETINVALIDARG( nDataPointCount <= MAX_GPUPS_INTERP_VALUES );
	for( int i = 0; i < MAX_GPUPS_INTERP_VALUES; i++ )
	{
		const CInterpolationPath<CFloatPair>::CInterpolationPoint* pPoint =
							tParticleField.GetPoint( MIN( i, nDataPointCount - 1 ) );
		
		float time = 0.0f; 
		float value = 0.0f;
		if ( pPoint )
		{
			time = pPoint->m_fTime;
			CFloatPair tValPair = pPoint->m_tData;
			value = tValPair.x;
		}
		switch( i )
		{
		case 0:
			tParamTime.fP0 = time;
			tParamValue.fP0 = value;
			break;
		case 1:
			tParamTime.fP1 = time;
			tParamValue.fP1 = value;
			break;
		case 2:
			tParamTime.fP2 = time;
			tParamValue.fP2 = value;
			break;
		case 3:
			tParamTime.fP3 = time;
			tParamValue.fP3 = value;
			break;
		}			
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_FillParticleInterpolationValues( const CInterpolationPath<CFloatPair>& tParticleField, 
											  GPUPS_FLOAT4& tParamTime, GPUPS_FLOAT4& tParamValueMin, 
											  GPUPS_FLOAT4& tParamValueMax )
{	
	int nDataPointCount = tParticleField.GetPointCount();
	ASSERT_RETINVALIDARG( nDataPointCount <= MAX_GPUPS_INTERP_VALUES );
	for( int i = 0; i < MAX_GPUPS_INTERP_VALUES; i++ )
	{
		const CInterpolationPath<CFloatPair>::CInterpolationPoint* pPoint =
							tParticleField.GetPoint( MIN( i, nDataPointCount - 1 ) );
		
		float time = 0.0f; 
		float min = 0.0f;
		float max = 0.0f;
		if ( pPoint )
		{
			time = pPoint->m_fTime;
			CFloatPair tValPair = pPoint->m_tData;
			min = tValPair.x;
			max = tValPair.y;
		}
		switch( i )
		{
		case 0:
			tParamTime.fP0 = time;
			tParamValueMin.fP0 = min;
			tParamValueMax.fP0 = max;
			break;
		case 1:
			tParamTime.fP1 = time;
			tParamValueMin.fP1 = min;
			tParamValueMax.fP1 = max;
			break;
		case 2:
			tParamTime.fP2 = time;
			tParamValueMin.fP2 = min;
			tParamValueMax.fP2 = max;
			break;
		case 3:
			tParamTime.fP3 = time;
			tParamValueMin.fP3 = min;
			tParamValueMax.fP3 = max;
			break;
		}			
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_FillParticleInterpolationValues( const CInterpolationPath<CFloatTriple>& tParticleField, 
											  GPUPS_FLOAT4& tParam, int nDataPoint )
{	
	int nDataPointCount = tParticleField.GetPointCount();
	//ASSERT_RETINVALIDARG( nDataPointCount <= MAX_GPUPS_INTERP_VALUES );	
	const CInterpolationPath<CFloatTriple>::CInterpolationPoint* pPoint =
						tParticleField.GetPoint( MIN( nDataPoint, nDataPointCount - 1 ) );

	float time = 0.0f;
	CFloatTriple tValTriple(0.0f);

	if ( pPoint )
	{
		time = pPoint->m_fTime;
		tValTriple = pPoint->m_tData;		
	}

	tParam.fP0 = tValTriple.x;
	tParam.fP1 = tValTriple.y;
	tParam.fP2 = tValTriple.z;
	tParam.fP3 = time;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_ParticleSystemGPURender(int nParticleInstanceId, PARTICLE_DRAW_INFO& drawInfo, float color[], float windInfluence, VECTOR vOffsetPosition, float velocityMultiplier, float velocityClamp, float sizeMultiplier, float SmokeRenderThickness)
{
	PARTICLE_SYSTEM_GPU* pGPUParticleSystem = HashGet(sgtGPUParticleSystems,nParticleInstanceId);
	if( ! pGPUParticleSystem ) 
		return S_FALSE;

	PARTICLE_SYSTEM * pParticleSystem = e_ParticleSystemGet( pGPUParticleSystem->nId );
	if ( ! pParticleSystem )
	{
		V_RETURN( dx10_ParticleSystemGPUDestroy( pGPUParticleSystem->nId ) );
		return S_FALSE;
	}

	PARTICLE_SYSTEM_DEFINITION * pDefinition = pParticleSystem->pDefinition;
	if (! pDefinition )
		return S_FALSE;

	if (
			( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_FLUID ) != 0 )
				&&
			e_OptionState_GetActive()->get_bFluidEffects()	// CHB 2007.09.12
		)
	{
		if ( sFluidSystemInit( pParticleSystem, pGPUParticleSystem ) == S_FALSE )
			return S_FALSE;

		//if nvidia fluid sim, update and render then return....
		if(pGPUParticleSystem->pFluid)
		{
			bool useBFECC;
			if ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_FLUID_USE_BFECC )
                useBFECC = true;
			else
				useBFECC = false;

			
			VECTOR monsterPositions[MAX_INTERACTING_UNITS];
			VECTOR monsterHeightAndRadiusAndSpeed[MAX_INTERACTING_UNITS];
			VECTOR monsterDirection[MAX_INTERACTING_UNITS];
			VECTOR center = VECTOR(0,0,0); 
			float radius = 0;

			for(int i=0;i<MAX_INTERACTING_UNITS;i++)
                monsterHeightAndRadiusAndSpeed[i].z = 0;     
			
            pGPUParticleSystem->pFluid->GetCenterAndRadius(center,radius);    

            //distance from center to eye:
            VECTOR vEye;
            e_GetViewPosition( &vEye );
			VECTOR vecCenterEye =  vEye - center ;
			float fVectorCELengthSquared  = vecCenterEye.x*vecCenterEye.x + vecCenterEye.y*vecCenterEye.y + vecCenterEye.z*vecCenterEye.z;
            float fLOD = e_GetWorldDistanceBiasedScreenSizeByVertFOV(SQRT_SAFE(fVectorCELengthSquared),radius); //[0,1]

			bool updateBasedOnLOD = true;
			bool renderBasedOnLOD = true;

			float transitionUpperLimit = 0.38f; //0.56f;
			float transitionLowerLimit = 0.33f; //0.26f;
			float transitionUpperLimitAlpha = 0.4f; //0.7f;
			float UpperAlphaMinusLower      = transitionUpperLimitAlpha - transitionLowerLimit; 

			if( fLOD < transitionUpperLimit )
                updateBasedOnLOD = false;
			if( fLOD < transitionLowerLimit )
                renderBasedOnLOD = false;

            if(updateBasedOnLOD)
			{
				(*gpfn_ParticlesGetNearbyUnits)(MAX_INTERACTING_UNITS, center, radius, monsterPositions, monsterHeightAndRadiusAndSpeed, monsterDirection );
			    pGPUParticleSystem->pFluid->update((D3D_EFFECT*)drawInfo.pEffect, (EFFECT_TECHNIQUE*)drawInfo.pTechnique, drawInfo.tFocusUnit.vPosition, drawInfo.tFocusUnit.fRadius, drawInfo.tFocusUnit.fHeight, drawInfo.vWindDirection, drawInfo.fWindMagnitude, windInfluence, 2.0, velocityMultiplier*3, velocityClamp, sizeMultiplier, useBFECC, monsterPositions, monsterHeightAndRadiusAndSpeed, monsterDirection);
			}
			if(renderBasedOnLOD)
			{  

            	D3D_EFFECT * pEffect = e_ParticleSystemGetEffect( pDefinition );
         	    EFFECT_TECHNIQUE * pTechnique = e_ParticleSystemGetTechnique( pDefinition, pEffect );

				float alpha = (fLOD - transitionLowerLimit)/UpperAlphaMinusLower;
				if(alpha>1) alpha=1;
				V( dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_RENDER_THICKNESS,SmokeRenderThickness*alpha) );
				pGPUParticleSystem->pFluid->render3D(fluidVelocity, (D3D_EFFECT*)drawInfo.pEffect, (EFFECT_TECHNIQUE*)drawInfo.pTechnique, (D3DXMATRIX*)&(drawInfo.matView), (D3DXMATRIX*)&(drawInfo.matProjection), (D3DXMATRIX*)&(pParticleSystem->mWorld),vOffsetPosition, color);//drawInfo.pParticleSystemCurr2->vPosition);
			}
			//pGPUParticleSystem->pFluid->draw(fluidVelocity,(D3D_EFFECT*)drawInfo.pEffect,(EFFECT_TECHNIQUE*)drawInfo.pTechnique);

			return S_OK;
		}
	}
	//======================================
	//else if general gpu system...

	//TODO: sort out the difference between a particle system which can be updated at any time
	// vs. a particle system spawned from mesh which needs immediate update.


	//TEMPORARY FIX TO PREVENT HAMMER SPAWNED SYSTEM FROM DRAWING
	if(!pGPUParticleSystem->pEffect)
	{
		return S_FALSE;
	}

	D3D_EFFECT* pEffect = pGPUParticleSystem->pEffect;

	V_RETURN( dxC_SetVertexDeclaration( VERTEX_DECL_PARTICLE_SIMULATION, pEffect ) );
	//dxC_GetDevice()->SOSetTargets(0,0,0);

	D3D_VERTEX_BUFFER_DEFINITION tVBDef;
	tVBDef.eVertexType = VERTEX_DECL_PARTICLE_SIMULATION;
	unsigned int stride = dxC_GetVertexSize(0, &tVBDef, NULL);

	D3D_VERTEX_BUFFER* pBuffer = dxC_VertexBufferGet( 
		pGPUParticleSystem->nParticleBufferIDs[ pGPUParticleSystem->currentBuffer ] );

	V_RETURN( dxC_SetStreamSource( 0, pBuffer->pD3DVertices, 0, stride ) );
	dxC_GetDevice()->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );

	EFFECT_TECHNIQUE * pTechnique = &pEffect->ptTechniques[ pEffect->pnTechniquesIndex[ 2 ] ]; // Phase 3

	V_RETURN( dxC_SetTechnique( pEffect, pTechnique, NULL ) );
	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );
	V_RETURN( gpStateManager->ApplyBeforeRendering( pEffect ) );

	V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_VIEWMATRIX, (D3DXMATRIXA16*)&drawInfo.matView ) );
	V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_PROJECTIONMATRIX, (D3DXMATRIXA16*)&drawInfo.matViewProjection ) );

	GPUPS_PARAMETERS& tParams = pGPUParticleSystem->tParams;
	
	float fPathTime = pParticleSystem->fPathTime;
	CFloatPair tValPair = pDefinition->tLaunchScale.GetValue(fPathTime);

	tParams.launchScale.fMax = tValPair.y;
	tParams.launchScale.fMin = tValPair.x;

	V( dx10_FillParticleInterpolationValues( pDefinition->tParticleScale, tParams.scaleTime, tParams.scaleMin, tParams.scaleMax ) );
	V( dx10_FillParticleInterpolationValues( pDefinition->tParticleAlpha, tParams.alphaTime, tParams.alpha ) );
	V( dx10_FillParticleInterpolationValues( pDefinition->tParticleColor, tParams.color0, 0 ) );
	V( dx10_FillParticleInterpolationValues( pDefinition->tParticleColor, tParams.color1, 1 ) );
	V( dx10_FillParticleInterpolationValues( pDefinition->tParticleColor, tParams.color2, 2 ) );
	V( dx10_FillParticleInterpolationValues( pDefinition->tParticleColor, tParams.color3, 3 ) );

	V( dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_GPUPS_PARAMS, (float*)&tParams, 0, sizeof( tParams ) ) );

	CFloatTriple tColor = pDefinition->tParticleColor.GetValue(fPathTime);
	D3DXVECTOR4 vColor( tColor.x, tColor.y, tColor.z, 1.0f );
	V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_PARTICLE_GLOBAL_COLOR, &vColor ) );

#ifdef D3D10_GPUPS_QUERY
	D3D10_QUERY_DESC queryDesc;
	queryDesc.Query = D3D10_QUERY_SO_STATISTICS;
	queryDesc.MiscFlags = 0;
	ID3D10Query* pQuery;
	V( dxC_GetDevice()->CreateQuery(&queryDesc, &pQuery) );
	V( pQuery->Begin() );
#endif //#ifdef D3D10_GPUPS_QUERY

	dxC_GetDevice()->DrawAuto();
	
#ifdef D3D10_GPUPS_QUERY
	V( pQuery->End() );
	D3D10_QUERY_DATA_SO_STATISTICS tQueryData;
	while( S_OK != pQuery->GetData( &tQueryData, sizeof( tQueryData ), 0) )
	{
		trace( "Number of particles rendered: %ll\n", tQueryData.NumPrimitivesWritten );		
	}
#endif //#ifdef D3D10_GPUPS_QUERY

	//FLIP BUFFERS
	pGPUParticleSystem->currentBuffer = 
		( pGPUParticleSystem->currentBuffer + 1 ) % MAX_PARTICLE_BUFFERS;
	pGPUParticleSystem->newFrame = TRUE;

	return S_OK;
}




