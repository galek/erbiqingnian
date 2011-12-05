//-------------------------------------------------------------------------------------------------
// Prime 
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// INCLUDE
//-------------------------------------------------------------------------------------------------
#include "e_pch.h"
#include "prime.h"
#include "boundingbox.h"

#include "game.h"
#include "havok.h"
#include "filepaths.h"
#include "e_havokfx.h"
#include "e_particle.h"
#include "dxC_havokfx.h"

#define USE_HAVOK_4 1 /// this is the last one of these, but I can't seem to get rid of it tonigh... oh well
#if USE_HAVOK_4
#ifndef USE_HAVOKFX_4
#pragma message ("Stubbing (not using) Havok FX 4")

void e_HavokFXSystemInit() 
{

}

BOOL e_HavokFXIsEnabled() 
{
	return 0; 
}

void e_HavokFXInitWorld(
						hkWorld * pWorld,
						const BOUNDING_BOX & tBoundingBox,
						BOOL bAddFloor,
						float fFloorBottom,
						int maxSystems)
{
	return;
}

void e_HavokFXCloseWorld( )
{

}

void e_HavokFXStepWorld( float )
{

}

bool e_HavokFXAddH3RigidBody(class hkRigidBody *,int,enum SampleDir)
{
	return false;
}

bool e_HavokFXAddFXRigidBody(const MATRIX&, const VECTOR&, int)
{
	return false;
}

bool e_HavokAddSystem( int, HAVOKFX_PARTICLE_TYPE )
{
	return false;
}

void e_HavokFXRemoveSystem( int )
{

}

void e_HavokFXSetSystemParamVector( int, HAVOKFX_PHYSICS_SYSTEM_PARAM, const VECTOR4& )
{

}

void e_HavokFXSetSystemParamFloat( int, HAVOKFX_PHYSICS_SYSTEM_PARAM, float )
{

}

void e_HavokFXSystemClose( )
{

}

int e_HavokFXGetSystemParticleCount( int )
{
	return 0;
}

bool e_HavokFXGetSystemVerticesToSkip( int sysid, int& )
{
	return false;
}

void e_DumpHavokFXReport( int, bool )
{

}

int dxC_HavokFXGetRigidAllTransforms(LPD3DCVB ppRigidBodyTransforms[ HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS ], LPD3DCVB * posVB, LPD3DCVB * velVB )
{
	return 0;
}

void dxC_HavokFXDeviceInit()
{

}

void dxC_HavokFXDeviceRelease()
{

}

void dxC_HavokFXDeviceLost()
{

}

void dxC_HavokFXDeviceReset()
{

}

#else
#pragma message ("Using Havok FX 4")
#error

#if defined(ENGINE_TARGET_DX9)
#include "dxC_caps.h"
#endif

#include <hkfxphysics/hkFxWorld.h>
#include <hkfxphysics/hkFxParticle.h>
#include <hkfxphysics/hkFxWorldSetup.h>
#include <hkfxphysics/hkFxHeightMapShapeRep.h>
#include <hkfxphysics/builder/hkFxShapeBuilder.h>
#include <hkfxphysics/BodySystem/Rigid/hkFxRigidBodySystem.h>
#include <hkfxphysics/BodySystem/Rigid/hkFxRigidBodySubSystem.h>
#include <hkfxphysics/BodySystem/Particle/hkFxParticleBodySystem.h>
#include <hkfxphysics/BodySystem/Particle/hkFxParticleBodySubSystem.h>

// for renderer interface (grab vertex buffers directly)
#include <hkfxphysics/BodySystem/Rigid/Gpu/hkFxRigidBodySystemGpu.h>
#include <hkfxphysics/BodySystem/Particle/Gpu/hkFxParticleBodySystemGpu.h>

static hkFxWorld* sgpFxWorld = NULL;
static hkFxWorldDefaultSystems* sgpFxSystems = NULL;
static bool sgbFxGpuSim = false;

#define HAVOKFX_TIMESTEP (1.0f/60.0f)
#define HAVOKFX_NUM_PARTICLE_SUBSYSTEMS 4 // looks like we have to preallocate these for now
static hkArray<int> sgvFxFreeSystemIndices;
static int sgnFxNumUsedSubSystems = 0;

struct FX_PARTICLE_SYSTEM
{
	int							nId;
	FX_PARTICLE_SYSTEM*			pNext;

	int							nSubSystemIndex;
	hkFxParticleEmitter*		pEmitter;
};

CHash<FX_PARTICLE_SYSTEM> sgtFxParticleSystems;

class HammerPlanarHeightFieldShape : public hkSampledHeightFieldShape
{
public:
	HammerPlanarHeightFieldShape( const hkSampledHeightFieldBaseCinfo& ci )
		: hkSampledHeightFieldShape(ci)
	{
	}

	HK_FORCE_INLINE hkReal getHeightAt( int x, int z ) const
	{
		return 0.0f;
	}

	HK_FORCE_INLINE hkBool getTriangleFlip() const
	{
		return false;
	}

	virtual void collideSpheres( const CollideSpheresInput& input, SphereCollisionOutput* outputArray) const
	{
		hkSampledHeightFieldShape_collideSpheres(*this, input, outputArray);
	}
};

void e_HavokFXSystemInit() 
{
	HashInit(sgtFxParticleSystems, NULL, HAVOKFX_NUM_PARTICLE_SUBSYSTEMS);
}

void e_HavokFXSystemClose( )
{
	e_HavokFXCloseWorld();

	for( FX_PARTICLE_SYSTEM* pFxSystem = HashGetFirst(sgtFxParticleSystems); 
		pFxSystem; 
		pFxSystem = HashGetNext(sgtFxParticleSystems, pFxSystem))
	{
		// do some system cleanup stuff here?
	}
	HashFree(sgtFxParticleSystems);
}


BOOL e_HavokFXIsEnabled() 
{
#ifndef HAVOKFX_ENABLED
	return FALSE;
#endif // HAVOKFX_ENABLED

#if defined(ENGINE_TARGET_DX9)
#ifdef _DEBUG
	GLOBAL_DEFINITION * pGlobalDef = DefinitionGetGlobal();
	if ( ( pGlobalDef->dwFlags & GLOBAL_FLAG_HAVOKFX_ENABLED ) == 0 )
		return FALSE;
#endif
	return ( dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) == DT_NVIDIA_VENDOR_ID &&
		dx9_CapGetValue( DX9CAP_MAX_VS_VERSION ) >= D3DVS_VERSION(3, 0)
		//&& dx9_CapIsDriverVersion( HAVOKFX_DRIVER_HIGH, HAVOKFX_DRIVER_LOW) 
		);
#else
	return FALSE;
#endif
}

void e_HavokFXInitWorld(
						hkWorld * pWorld,
						const BOUNDING_BOX & tBoundingBox,
						BOOL bAddFloor,
						float fFloorBottom,
						int maxSystems)
{
	if ( !e_HavokFXIsEnabled() )
		return;

	ASSERT_RETURN( pWorld );

	hkFxWorldCinfo cinfo;
	{
		cinfo.setDefaults();

		// GPU interface
		LPD3DCDEVICE d3dDev = dxC_GetDevice();
#if defined(ENGINE_TARGET_DX9)
		cinfo.m_gpuInterface = hkGpuInterfaceCreate( FILE_PATH_HAVOKFX_SHADERS, false, d3dDev );
#else
		//@@CB FX should be able to work with a D3D10 device now - like so
		cinfo.m_gpuInterface = hkGpuInterfaceCreate( FILE_PATH_HAVOKFX_SHADERS, false, NULL, d3dDev );
#endif

		// World AABB
		hkAabb tHavokBoundingBox;
		tHavokBoundingBox.m_min.set( tBoundingBox.vMin.fX, tBoundingBox.vMin.fY, tBoundingBox.vMin.fZ );
		tHavokBoundingBox.m_max.set( tBoundingBox.vMax.fX, tBoundingBox.vMax.fY, tBoundingBox.vMax.fZ );
		cinfo.m_broadPhaseCinfo.setBroadPhaseExtents( tHavokBoundingBox );
		cinfo.m_bufferMemSizeGuess = 32 * 1024 * 1024;

		// Threading
		GLOBAL_DEFINITION* pGlobalDef = DefinitionGetGlobal();
		bool bThreading = ( (pGlobalDef->dwFlags & GLOBAL_FLAG_MULTITHREADED_HAVOKFX_ENABLED) != 0 );
		cinfo.m_enableThreadingCPU = bThreading;
		cinfo.m_enableThreadingGPU = false;

		// Create world
		sgpFxWorld = new hkFxWorld( cinfo );
		sgbFxGpuSim = ( cinfo.m_gpuInterface && cinfo.m_gpuInterface->isInitialized() );

		if( cinfo.m_gpuInterface )
			cinfo.m_gpuInterface->removeReference();
	}

	ASSERT_RETURN( sgpFxWorld );

	hkFxWorldDefaultSystemSettings settings;
	{
		const int numFxPlanarShapes = 1;
		const int numFxPlanarBodies = 1;
		const int numFxCubeShapes = 100;
		const int numFxCubeBodies = 1000;

		settings.m_systemsToCreate = HKFX_DEFAULT_RBS_AND_PARTICLES_WITH_COLLISIONS & ~HKFX_DEFAULT_PARTICLE_PARTICLE_ACTION;
		settings.m_shaderPath = FILE_PATH_HAVOKFX_SHADERS;
		const hkReal fxDt = HAVOKFX_TIMESTEP * HAVOKFX_TIMESTEP;

		// FX bodies
		settings.m_gravity.set( 0.0f, 0.0f, -9.81f * fxDt );
		settings.m_maxNumPlanarShapes = numFxPlanarShapes;
		settings.m_maxNumCubeShapes = numFxCubeShapes;
		settings.m_maxNumRigidBodies = numFxPlanarBodies + numFxCubeBodies;
		settings.m_numRigidCollisionGroups = 2 + 1; // landscape = 0, inactive = 1, +1 for me
		settings.m_numRigidSubSystems = 1; // 1 subsystem
		settings.m_rigidSubSystemSize[0] = (hkUint16)settings.m_maxNumRigidBodies;

		// FX particles
		settings.m_particleGravity = settings.m_gravity;
		settings.m_maxNumParticles = HAVOKFX_NUM_PARTICLE_SUBSYSTEMS * 4096; // 1024 per subsystem
		settings.m_particleRestitution = 0.01f;
		settings.m_particleFriction = 0.1f;

		//@@CB TODO: dynamic subsystems?
		settings.m_numParticleSubSystems = HAVOKFX_NUM_PARTICLE_SUBSYSTEMS;
		for( int ss = 0; ss < settings.m_numParticleSubSystems; ss++ )
		{
			settings.m_particleSubSystemSize[ss] = (hkUint16)settings.m_maxNumParticles / settings.m_numParticleSubSystems;
			sgvFxFreeSystemIndices.pushBack( ss );
		}

		sgpFxSystems = hkFxWorldCreateDefaultSystems( sgpFxWorld, settings );
	}

	//@@CB HACK make a heightfield. FX crashes if I step it 
	// with no bodies in the world (and I'm testing in Hammer
	// anyway).
	{
		// Finally, we want our new heightfield. 
		hkSampledHeightFieldBaseCinfo ci;
		ci.m_xRes = 128;
		ci.m_zRes = 128;
		ci.m_scale.setAll( 1.0f );
		HammerPlanarHeightFieldShape* shape = new HammerPlanarHeightFieldShape( ci );

		hkTransform worldToFx;
		hkFxHeightMapShapeRep* shapeRep = 
			hkFxBuildTextureMapRepresentation( static_cast<hkSampledHeightFieldShape*>(shape), worldToFx );

		hkTransform transform;
		transform.setIdentity();
		hkReal halfExtent = 0.5f * static_cast<hkSampledHeightFieldShape*>(shape)->m_extents(0);
		transform.setRotation( hkQuaternion( hkVector4(1,0,0), hkMath::HK_REAL_PI / 2.0f) );
		transform.setTranslation( hkVector4(-halfExtent, halfExtent, 0.0f) );

		hkFxShape fxShape;
		fxShape.setShapeRep( shapeRep );
		fxShape.m_bodyData.m_invInertia.setZero4();
		shapeRep->removeReference();

		hkFxShapeHandle fxHandle = sgpFxSystems->m_rigidBodies->addShape( fxShape );

		// add the shape to the rigid body system
		hkFxRigidBody body;
		//body.m_fixed = true;
		body.m_active = false;
		body.m_shapeIndex = fxHandle;
		body.m_collisionGroup = 0;
		body.m_transform.setMulMulInverse( transform, worldToFx );
		HK_ON_DEBUG( hkFxRigidBodyHandle bodyIndex = ) 
			sgpFxSystems->m_rigidBodies->addRigidBody( sgpFxSystems->m_rigidBodies->getRigidSubSystem(0), body );
		HK_ASSERT(0x0, bodyIndex != HKFX_INVALID_HANDLE);
	}

	sgpFxWorld->initialize();

	sgvFxFreeSystemIndices.reserveExactly( HAVOKFX_NUM_PARTICLE_SUBSYSTEMS );
	sgnFxNumUsedSubSystems = 0;
}

void e_HavokFXCloseWorld( )
{
	if( sgpFxSystems )
	{
		sgpFxSystems->removeReference();
		sgpFxSystems = NULL;
	}

	if( sgpFxWorld )
	{
		sgpFxWorld->removeReference();
		sgpFxWorld = NULL;
	}

	sgbFxGpuSim = false;

	sgvFxFreeSystemIndices.clearAndDeallocate();
}

void e_HavokFXStepWorld( float fDelta )
{
	if ( !e_HavokFXIsEnabled() )
		return;

	if( sgpFxWorld )
	{
		sgpFxWorld->startStepCPU( fDelta );
		// Do some rendering..
		sgpFxWorld->finishStepCPU();

		hkFxExecuteOptions executeOptions = 
			hkFxExecuteOptions( HKFX_GPU_SIM_INTEGRATION | HKFX_GPU_SIM_NARROWPHASE | HKFX_GPU_PREPARE_RENDERING_DATA );
		sgpFxWorld->startStepGPU( executeOptions );
		// Do some CPU work..
		sgpFxWorld->finishStepGPU();
	}
}

bool e_HavokFXAddH3RigidBody(class hkRigidBody *,int,enum SampleDir)
{
	return false;
}

bool e_HavokFXAddFXRigidBody(const MATRIX&, const VECTOR&, int)
{
	return false;
}

bool e_HavokAddSystem( int nParticleSystemId, HAVOKFX_PARTICLE_TYPE particleType )
{
	ASSERT( NULL == HashGet( sgtFxParticleSystems, nParticleSystemId ) );

	// allocate a new system
	FX_PARTICLE_SYSTEM* pParticleSystem = HashAdd(sgtFxParticleSystems, nParticleSystemId);
	if( !pParticleSystem )
		return false;

	ASSERT( sgvFxFreeSystemIndices.getSize() > 0 );
	if( 0 == sgvFxFreeSystemIndices.getSize() )
	{
		HashRemove( sgtFxParticleSystems, nParticleSystemId );
		return false;
	}

	pParticleSystem->nSubSystemIndex = sgvFxFreeSystemIndices[ 0 ];
	sgvFxFreeSystemIndices.removeAt( 0 );

	hkFxParticleBodySubSystem* pSubSystem 
		= sgpFxSystems->m_particles->getParticleSubSystem( pParticleSystem->nSubSystemIndex );
	ASSERT( pSubSystem );
	if( !pSubSystem )
	{
		//@@CB although if we failed to get the subsystem, maybe putting the id back
		// into rotation isn't such a hot idea.
		//sgvFxFreeSystemIndices.pushBack( pParticleSystem->nSubSystemIndex );
		HashRemove( sgtFxParticleSystems, nParticleSystemId );
		return false;
	}

	sgnFxNumUsedSubSystems++;

	pParticleSystem->pEmitter = HK_NULL;
	{
		pParticleSystem->pEmitter = new hkFxParticleEmitter;
		ASSERT( pParticleSystem->pEmitter );
		// TODO: proper names
		pParticleSystem->pEmitter->m_shaderFile = hkString(FILE_PATH_HAVOKFX_SHADERS) + "hkFx4Test.cg";
		pParticleSystem->pEmitter->m_shaderEntryPoint = "boxEmitterColor";
		pParticleSystem->pEmitter->m_rate = 10.0f;
		// TODO: this doesn't work like I expect it does. Somewhere there
		// must be a comparison like this for each particle:
		// if( currentTime - particle.birthTime > particle.lifeTime ) killParticle
		// Set it to a useful small value. m_rate does seem to work.
		pParticleSystem->pEmitter->m_particleLifetime = 5.0f;
		pParticleSystem->pEmitter->m_active = true;

		// TODO: this isn't set anywhere, set it explicitly here
		pSubSystem->setEmitterParameter( pParticleSystem->pEmitter, "particleRadius", hkVector4(0.1f,0.0f,0.0f) );
	}

	pSubSystem->addEmitter( pParticleSystem->pEmitter );
	pParticleSystem->pEmitter->removeReference();

	return true;
}

void e_HavokFXRemoveSystem( int nParticleSystemId )
{
	// allocate a new system
	FX_PARTICLE_SYSTEM* pParticleSystem = HashGet( sgtFxParticleSystems, nParticleSystemId );
	if( !pParticleSystem )
	{
		// already removed it? Gets called twice in a row from Hammer.
		// Check num_free + num_used = num_available
		ASSERT( (sgvFxFreeSystemIndices.getSize() + sgnFxNumUsedSubSystems) == HAVOKFX_NUM_PARTICLE_SUBSYSTEMS );
		return;
	}

	hkFxParticleBodySubSystem* pSubSystem 
		= sgpFxSystems->m_particles->getParticleSubSystem( pParticleSystem->nSubSystemIndex );

	pSubSystem->removeEmitter( pParticleSystem->pEmitter );
	pParticleSystem->pEmitter = NULL;

	sgvFxFreeSystemIndices.pushBack( pParticleSystem->nSubSystemIndex );
	sgnFxNumUsedSubSystems--;

	HashRemove( sgtFxParticleSystems, nParticleSystemId );
}

void e_HavokFXSetSystemParamVector( int nParticleSystemId, HAVOKFX_PHYSICS_SYSTEM_PARAM param, const VECTOR4& vVal )
{
	FX_PARTICLE_SYSTEM* pParticleSystem = HashGet( sgtFxParticleSystems, nParticleSystemId );
	ASSERT( pParticleSystem );

	hkFxParticleBodySubSystem* pSubSystem = sgpFxSystems->m_particles->getParticleSubSystem( pParticleSystem->nSubSystemIndex );

	hkVector4 hkParm( vVal.fX, vVal.fY, vVal.fZ, vVal.fW );

	switch( param )
	{
	case HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_POS:
		pSubSystem->setEmitterParameter( pParticleSystem->pEmitter, "position", hkParm );
		break;
	case HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_SIZE:
		pSubSystem->setEmitterParameter( pParticleSystem->pEmitter, "size", hkParm );
		break;
	case HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_MIN:
		pSubSystem->setEmitterParameter( pParticleSystem->pEmitter, "velocityMin", hkParm );
		break;
	case HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_RANGE:
		pSubSystem->setEmitterParameter( pParticleSystem->pEmitter, "velocityRange", hkParm );
		break;
	case HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_RADIUS:
		pSubSystem->setEmitterParameter( pParticleSystem->pEmitter, "particleRadius", hkParm );
		break;
	default:
		break;
	}
}

void e_HavokFXSetSystemParamFloat( int nParticleSystemId, HAVOKFX_PHYSICS_SYSTEM_PARAM param, float fVal )
{
	FX_PARTICLE_SYSTEM* pParticleSystem = HashGet( sgtFxParticleSystems, nParticleSystemId );
	ASSERT( pParticleSystem );

	switch( param )
	{
	case HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_RATE:
		pParticleSystem->pEmitter->m_rate = fVal;
		break;
	case HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_LIFETIME:
		pParticleSystem->pEmitter->m_particleLifetime = fVal;
		break;
	default:
		break;
	}
}

int e_HavokFXGetSystemParticleCount( int )
{
	if( !sgpFxWorld || !sgpFxSystems )
		return 0;

	return sgpFxSystems->m_particles->getNumBodiesUsed();
}

bool e_HavokFXGetSystemVerticesToSkip( int sysid, int& )
{
	return false;
}

void e_DumpHavokFXReport( int, bool )
{

}

int dxC_HavokFXGetRigidAllTransforms(LPD3DCVB ppRigidBodyTransforms[ HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS ], LPD3DCVB* posVB, LPD3DCVB* velVB )
{
#if defined(ENGINE_TARGET_DX9)
	if( !sgpFxWorld || !sgpFxSystems )
		return FALSE;

	if( ppRigidBodyTransforms )
	{
		if( sgpFxSystems->m_rigidBodies )
		{
			hkFxRigidBodySystemGpu* gpuRigidBodies 
				= static_cast<hkFxRigidBodySystemGpu*>( sgpFxSystems->m_rigidBodies );
			LPDIRECT3DTEXTURE9 rowR2VB[ HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS ];
			for( int i = 0; i < HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS; i++ )
				gpuRigidBodies->getTransformVertexArray( i, ppRigidBodyTransforms[ i ], rowR2VB[ i ] );
		}
		else
		{
			for( int i = 0; i < HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS; i++ )
				ppRigidBodyTransforms[ i ] = NULL;
		}
	}

	if( sgpFxSystems->m_particles->getNumBodiesUsed() )
	{
		if( posVB && sgpFxSystems->m_particles->getNumBodiesUsed() )
		{
			LPDIRECT3DTEXTURE9 posR2VB;
			if( sgpFxSystems->m_particles )
			{
				hkFxParticleBodySystemGpu* gpuParticles 
					= static_cast<hkFxParticleBodySystemGpu*>( sgpFxSystems->m_particles );
				gpuParticles->getParticlePositionVertexArray( *posVB, posR2VB );
			}
		}

		if( velVB )
		{
			LPDIRECT3DTEXTURE9 velR2VB;
			if( sgpFxSystems->m_particles )
			{
				hkFxParticleBodySystemGpu* gpuParticles 
					= static_cast<hkFxParticleBodySystemGpu*>( sgpFxSystems->m_particles );
				gpuParticles->getParticleVelocityVertexArray( *velVB, velR2VB ); 
			}
		}
	}
#endif

	return TRUE;
}

void dxC_HavokFXDeviceInit()
{

}

void dxC_HavokFXDeviceRelease()
{

}

void dxC_HavokFXDeviceLost()
{

}

void dxC_HavokFXDeviceReset()
{

}

#endif // USE_HAVOKFX_4
#else

#if defined(ENGINE_TARGET_DX9)
#include "dxC_caps.h"
#endif

#include "GL\GL.h"
#include <hkbase/stream/hkOstream.h>
#include <hkfxphysics/hkFxWorld.h>
#include <hkfxphysics/gpu/hkFxWorldGpu.h>
#include <hkfxphysics/builder/hkFxShapeBuilder.h>
#include <hkfxphysics/builder/hkFxPhysicsBridge.h>
#include <hkvisualize/hkDebugDisplay.h>

#include "performance.h"
#include "array.h"
#include "dxC_havokfx.h"

//#define DEBUG_HAVOK_FX
#define HAVOKFX_TIMESTEP (1.0f/60.0f)
#define MAX_NUM_H3_SHAPES 1000
#define MAX_NUM_H3_SHAPES_PER_RAGDOLL 25
#define MAX_NUM_LANDSCAPES 200 
#define MAX_NUM_RIGIDBODIES 6400
#define MAX_NUM_COLLISIONS_PER_BODY 10
#define MAX_NUM_BATCHES 200

#define PREALLOCATED_SYSTEM_COUNT 4
#define PREALLOCATED_SYSTEM_SIZE 500

#define NUM_GPU_STEPS_NEEDED 2
#define NUM_GPU_STEPS_NEEDED_AVOID_INTERSYSTEM_COLLISIONS 0


#ifdef DEBUG_HAVOK_FX
	#include <hkfxphysics/util/hkFxDebugUtility.h>
#endif


//---------------------------------------------
//checking for problems with too many collision batches

#define CHECK_COLLISION_PROBLEMS 
#define MAX_NUM_COLLIDING_BATCHES_TOLERATED 45
#define NUM_STEPS_FOR_ADD 35
#define NUM_STEPS_FOR_REM 15
#define NUM_NORMAL_COLLIDING_BATCHES 30
#define NUM_RESERVED_SHAPES 3

typedef enum
{
	STATE_NORMAL,
	STATE_CHECKING,
} STATE_COLLISIONS;

//-------------------------------------------


struct POOL
{
	int nId;
	POOL * pNext;
};

class IDPool : public CHash<POOL>
{
public:
	IDPool( int startIndex) :
	m_Index ( startIndex )
	{
		Init(NULL, 16);
	}
	int GetID()
	{
		POOL* nCurrent = GetFirstItem();
		if(nCurrent)
		{
			int ID = nCurrent->nId;
			RemoveItem(ID);
			return ID;
		}

		return m_Index++;
	}
	void FreeID(int id)
	{
#if ISVERSION(DEVELOPMENT)	// CHB 2006.07.07 - Inconsistent usage.
		AddItemDbg(id, __FILE__, __LINE__);
#else
		AddItem(id);
#endif
	}
private:
	int m_Index;
	POOL m_Pool;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

typedef enum
{
	SYSTEM_STATE_LOST,
	SYSTEM_STATE_DIRTY,
	SYSTEM_STATE_NEW,
	SYSTEM_STATE_DORMANT,
	SYSTEM_STATE_RUNNING,
} SYSTEM_STATE;

class hkScarabAction : public hkFxRigidBodyBehavior
{
public:	
	virtual void apply(hkFxWorld* world)
	{
	}
};

class hkParticlesAction : public hkFxParticleBehavior
{
public:	

	virtual void apply(hkFxWorld* world)
	{
	}
};

struct BridgeRigidBody
{
	hkRigidBody*						pRigidBody;
	hkFxPhysicsBridge::AddBodyParams	AddBodyParams;
};

struct HKFX_SYSTEM
{
    hkFxShapeHandle shapeHandle;
	int uniformParamId;
	int gpuStepped;
	unsigned int timeCreated;


	int nRegionSize;
	int nCollisionGroup;
	int nUsedBodies;

	hkFxShape ShapeRep;
	hkFxShapeHandle hRegionStart;
	SYSTEM_STATE nState;
	HAVOKFX_PARTICLE_TYPE nParticleType;
};

struct SYSTEM_HASH
{
	int nId;
	HKFX_SYSTEM system;
	SYSTEM_HASH * pNext;
};

struct SYSTEM_PARTICLE_HASH
{
	int nId;
	SYSTEM_STATE nState;
	int gpuStepped;
	SYSTEM_PARTICLE_HASH * pNext;
};

CHash<SYSTEM_PARTICLE_HASH> gtSystemParticlesIds;

//-------------------------------------------------------------------------------------------------
// Globals
//-------------------------------------------------------------------------------------------------
hkFxHeightMapShapeRep* g_ScarabShapeRep;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
typedef enum
{
	NUM_HAVOKFX_ACTION_TYPES,
}HAVOK_ACTION_TYPE;

struct HAVOKFX_SYSTEM_PARAM_DATA
{
	BOOL bIsVector;
	char szName[ 32 ];
};

const HAVOKFX_SYSTEM_PARAM_DATA sgHavokSystemParamData[ NUM_HAVOKFX_PHYSICS_SYSTEM_PARAMS ] = 
{//		is vector	Name
	{	TRUE,		"attractorPosArray"				 },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_ATTRACTOR_POSITION_AND_FORCE,
	{	TRUE,		"attractorDestructionRadiusArray"},	//HAVOKFX_PHYSICS_SYSTEM_PARAM_ATTRACTOR_DESTRUCTION_RADIUS_MIN_MAX
	{	TRUE,		"zAccelerationArray"     		 },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_Z_ACCELERATION,
	{	TRUE,		"windForceArray"				 },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_WIND_FORCE,
    {	TRUE,		"angularLinearVelMultiplierArray"},	//HAVOKFX_PHYSICS_SYSTEM_PARAM_LINEAR_AND_ANGULAR_MULTIPLIERS,
	{	TRUE,		"crawlOrFlyArray"                },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_CRAWL_OR_FLY
	{	TRUE,		"linearVelocityCapArray"         },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_LINEAR_VEL_CAP,
	{	TRUE,		"angularVelocityCapArray"        },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_ANGULAR_VEL_CAP,
	{   TRUE,       "position"                       }, //HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_POS,
	{   TRUE,       "size"                           }, //HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_SIZE,
	{   TRUE,       "velocityMin"                    }, //HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_MIN,
	{   TRUE,       "velocityRange"                  }, //HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_RANGE,
	{   TRUE,       "particleRadius"                 }, //HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_RADIUS,
	{   FALSE,      "emRate"						 },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_RATE, 
	{   FALSE,      "emLifetime"					 },	//HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_LIFETIME,
	{   FALSE,      "emParticleLifetime"		     }, //HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_LIFETIME,
	{   TRUE,       "antiBounceAmount"           	 }, //HAVOKFX_PHYSICS_SYSTEM_PARAM_PARTICLE_BEHAVIOR_ANTIBOUNCE


};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef DEBUG_HAVOK_FX
void DebugDrawRigidBodyInFXWorld(hkFxWorld* pFXWorld, hkFxRigidBodyHandle hRigidBody)
{
}
void DebugDrawRigidBodyInBridge(hkFxWorld* pFXWorld, hkFxPhysicsBridge* pBridge, hkFxRigidBodyHandle hRigidBody)
{
		hkRigidBody* pRigidBody = pBridge->getRigidBody( hRigidBody);
		if(!pRigidBody)
			return;

		hkFxShapeHandle shapeHandle = pBridge->findShape( (hkShape*)pRigidBody->getCollidable()->getShape());

		hkFxShape shape;
		shape.setDefaults();
		pFXWorld->getShape(shapeHandle, shape);
		hkFxHeightMapShapeRep* pShapeRep  = (hkFxHeightMapShapeRep*)shape.getShapeRep();

		if(!pShapeRep)
			return;

		//hkFxDebugUtility::hkFxDebugVisualizeCubeHeightMap(*pShapeRep, pRigidBody->getTransform(), 0xFFFFFFFF);
		hkFxDebugUtility::hkFxDebugVisualizeTextureHeightMap(*pShapeRep, pRigidBody->getTransform(), 0xFFFFFFFF);
}
#endif

#define WORLD_FLAG_HAS_BEEN_INITIALIZED MAKE_MASK( 0 )

class HavokFXWorldWrapper : public hkFxWorldRegionChangeCallback
{
protected:
	hkFxWorldGpu * m_pFXWorld;
	DWORD m_dwFlags;
	hkFxPhysicsBridge* m_pBridge;

	SIMPLE_DYNAMIC_ARRAY<HKFX_SYSTEM>		m_DeactivatedSystems;
	SIMPLE_DYNAMIC_ARRAY<hkFxRigidBody>		m_PendingRigidBodies;
	CHash<SYSTEM_HASH>						mtSystemsBodiesIds;
	IDPool*									m_pCollisionGroupPool;

	int m_nActionCount;
	int m_nParticleBehaviourCount;
	bool m_bStepFirstTime;

#ifdef CHECK_COLLISION_PROBLEMS
	
//	STATE_COLLISIONS runningState;
	int stepsElapsedSinceLastAdd;
	int	stepsElapsedSinceLastRem;
//	int currentSuspectSystemId;
//	int gpuStepsSinceStartCheck;

#endif CHECK_COLLISION_PROBLEMS

public:

	SIMPLE_DYNAMIC_ARRAY<BridgeRigidBody>	m_PendingH3Bodies;
	
	//Should be protected
		int m_iMaxSystems;
		int m_nParticleEmitterCount;
	

	hkArray<bool> costantParameterAssigned;
	int m_numFXShapes;
	hkVector4 m_pvParams[ NUM_HAVOKFX_PHYSICS_SYSTEM_PARAMS ];
	
	hkFxRigidBodyBehavior ** m_pActions;
    hkFxParticleBehavior ** m_pParticleActions;
    hkFxParticleEmitter ** m_pParticleEmitters;
	
#ifdef DEBUG_HAVOK_FX
	int m_hH3DebugBody;
	int m_hFXDebugBody;
#endif

public:
	HavokFXWorldWrapper::HavokFXWorldWrapper(const hkFxWorldCinfo& cInfo, int iMaxSystems, const char* fxShaderPath, IDirect3DDevice9* d3dDevice) :
		m_dwFlags( 0 ),
		m_pFXWorld( NULL ),
		m_pBridge( NULL ),	
		m_nActionCount( 0 ),
        m_nParticleEmitterCount(0),
		m_nParticleBehaviourCount( 0 ),
		m_pActions( NULL ),
		m_pParticleActions( NULL ),
		m_pParticleEmitters( NULL ),
		m_numFXShapes( 0 ),
		m_bStepFirstTime( false )
#ifdef DEBUG_HAVOK_FX
		,m_hH3DebugBody( -1 ),
		m_hFXDebugBody( -1 )
#endif
	{
		int n = 5;
		memset( m_pvParams, 0, sizeof( m_pvParams ) );

		m_PendingH3Bodies.Init( 5 );
		m_PendingRigidBodies.Init( 500 );
		m_DeactivatedSystems.Init ( 10 );
		HashInit( mtSystemsBodiesIds, NULL, 16 );

		m_pCollisionGroupPool = new IDPool( NUM_RESERVED_SHAPES );
		
		//Create our hkFx world
		m_pFXWorld = new hkFxWorldGpu( cInfo, fxShaderPath, d3dDevice);
		ASSERT( m_pFXWorld);

		/////// Make a bridge for the fixed and key framed bodies
		m_pBridge = new hkFxPhysicsBridge( m_pFXWorld );
		ASSERT( m_pFXWorld);

		//Setting region callback for tracking different systems
		m_pFXWorld->addRegionChangeCallback(this);		

		m_iMaxSystems = iMaxSystems;

		costantParameterAssigned.reserve(m_iMaxSystems);
		for(int i=0;i<m_iMaxSystems;i++)
			costantParameterAssigned.pushBack(false);


		/////// Add an action to the world for rigid bodies
		{
			hkScarabAction* pAction = new hkScarabAction;
			pAction->m_shaderFile = hkString(FILE_PATH_HAVOKFX_SHADERS) + "hkFxScarab2.cg";
			pAction->m_shaderEntryPoint = "handleBoundaries";
			m_pFXWorld->addBehavior( pAction );
			
			AddAction( pAction );

			char szNameSystem[ 30 ];
			for(int i=0;i<m_iMaxSystems;i++)
			{
				sprintf_s(szNameSystem,"shapeIDsArray[%d]",i);
				m_pFXWorld->setBehaviorParameter( pAction, szNameSystem, hkVector4(-1,0,0));
			}
		
		}

		//////// Add action to the world for particles
		{
			hkFxParticleEmitter *emitter = new hkFxParticleEmitter;
			emitter->m_active = false; 
			emitter->m_rate = 30; //used to be 100
			emitter->m_lifetime = 1e10;
			emitter->m_particleLifetime = 400;
			emitter->m_shaderFile = hkString(FILE_PATH_HAVOKFX_SHADERS) + "hkFxScarab2.cg";
			emitter->m_shaderEntryPoint = "circleEmitter";
			m_pFXWorld->setParticleEmitterParameter(emitter, "position", hkVector4(0.0, 0.0, 5.0));//(-23.0f,30.0f,10.0f)); 
			m_pFXWorld->setParticleEmitterParameter(emitter, "size", hkVector4(10.0f,10.0f,10.0f));
			m_pFXWorld->setParticleEmitterParameter(emitter, "velocityMin", hkVector4(-0.025f, -0.025f, -0.4f));
			m_pFXWorld->setParticleEmitterParameter(emitter, "velocityRange", hkVector4(0.05f, 0.05f, 0.3f));
			m_pFXWorld->setParticleEmitterParameter(emitter, "particleRadius", hkVector4(0.01f, 0.0f, 0.0f));
			AddParticleEmitter( emitter );
			m_pFXWorld->addParticleEmitter( emitter );

			hkParticlesAction *particleBoundAction = new hkParticlesAction;
			particleBoundAction->m_shaderFile =  hkString(FILE_PATH_HAVOKFX_SHADERS) + "hkFxScarab2.cg";
			particleBoundAction->m_shaderEntryPoint = "particleAntiBounce";
			AddParticleBehavior( particleBoundAction );
			m_pFXWorld->addParticleBehavior( particleBoundAction );
			
		}

//for initialization
#ifdef CHECK_COLLISION_PROBLEMS		

	    stepsElapsedSinceLastAdd = NUM_STEPS_FOR_ADD;
		stepsElapsedSinceLastRem = NUM_STEPS_FOR_REM;
	
#endif CHECK_COLLISION_PROBLEMS


		m_pFXWorld->initialize();
		m_dwFlags |= WORLD_FLAG_HAS_BEEN_INITIALIZED;

		PreAllocateSystems( PREALLOCATED_SYSTEM_COUNT, PREALLOCATED_SYSTEM_SIZE );
	}
	//-------------------------------------------------------------------------------------------------
	hkFxWorldGpu * GetWorld()
	{
		return m_pFXWorld;
	}

	bool bCanAddFXRigidBodies()
	{
		return ((UINT)m_pFXWorld->getNumSimulatedRigidBodies() + (UINT)m_PendingRigidBodies.Count() < (UINT)m_pFXWorld->getMaxNumRigidBodies());
	}

	bool bCanAddH3RigidBodies()
	{
		return (m_pBridge->getNumUsedShapes() + (UINT)m_PendingH3Bodies.Count() < MAX_NUM_H3_SHAPES + MAX_NUM_LANDSCAPES - MAX_NUM_H3_SHAPES_PER_RAGDOLL); //HACK HACK
	}
	//-------------------------------------------------------------------------------------------------
	hkFxPhysicsBridge * GetBridge()
	{
		return m_pBridge;
	}
	//-------------------------------------------------------------------------------------------------
	void Init()
	{

	}

	void PreAllocateSystems( int sysCount, int sysSize )
	{
		for( int i = 0; i < sysCount; ++i)
		{
			AddSystem( i, HAVOKFX_RIGID_BODY );
			CreateSystems();
			
			SYSTEM_HASH* pSystemHash = HashGet( mtSystemsBodiesIds, i);

			if(pSystemHash)
			{
				m_pFXWorld->setRigidBodyBodyCollision( 0, pSystemHash->system.nCollisionGroup, false);  //Turn off collisions with landscapes
				m_pFXWorld->setRigidBodyBodyCollision( 2, pSystemHash->system.nCollisionGroup, false);  //Turn off collisions with Ragdolls
				m_pFXWorld->setRigidBodyBodyCollision( pSystemHash->system.nCollisionGroup, pSystemHash->system.nCollisionGroup, false); //Turn off collisions with self

				hkFxRigidBody body;
				body.setDefaults();


				body.m_shapeIndex = pSystemHash->system.shapeHandle;
				body.m_collisionGroup = pSystemHash->system.nCollisionGroup;

				hkRotation rot;
				rot.set(hkQuaternion(0.0f, 0.0f, 0.0f, 1.0f));

				for(int j = 0; j < sysSize; ++j)
				{
					hkTransform randTrans(rot, hkVector4((float)i, (float)j, 1.0f, 1.0f));
					body.m_transform = randTrans;
					m_pFXWorld->addRigidBody( body );
				}

				pSystemHash->system.nState = SYSTEM_STATE_DIRTY;
			}
		}
	}
	//-------------------------------------------------------------------------------------------------
	~HavokFXWorldWrapper()
	{

		if (m_pFXWorld->isSimulating())
		{
			m_pFXWorld->quitSimulation();
		}

		if( m_pBridge)
		{
			m_pBridge->removeReference();
			m_pBridge = NULL;
		}

		if ( m_pActions )
			FREE( NULL, m_pActions );
		m_nActionCount = 0;

		if ( m_pParticleActions )
			FREE( NULL, m_pParticleActions );
		m_nParticleBehaviourCount = 0;

		if(m_pParticleEmitters)
			FREE( NULL,m_pParticleEmitters);
        m_nParticleEmitterCount = 0;

		if (m_pCollisionGroupPool)
			m_pCollisionGroupPool->Free();
		m_pCollisionGroupPool = NULL;

        //remove reference on all things stored above

        if ( m_pFXWorld )
		{
			m_pFXWorld->removeRegionChangeCallback(this);
			m_pFXWorld->removeReference();
			m_pFXWorld = NULL;
		}

		HashFree( mtSystemsBodiesIds );

		m_dwFlags &= ~WORLD_FLAG_HAS_BEEN_INITIALIZED;
	}

	//-------------------------------------------------------------------------------------------------
	void Step()
	{
		if ( 0 == (m_dwFlags & WORLD_FLAG_HAS_BEEN_INITIALIZED) )
			return;

		hkFxWorldGpu * pFXWorld = GetWorld();
		if ( ! pFXWorld )
			return;

		{//Broadphase simulation block.  If threading is enabled we can do other work during broadphase simulation
			if(!m_pFXWorld->isCpuThreadingEnabled() || !m_bStepFirstTime)
			{
				GetBridge()->sync();  //Need to sync here outside of the simulation
				ChangeSimulationState();
				pFXWorld->startStepCPU();
			}

			if(pFXWorld->isDoingCpuWork())
				pFXWorld->finishStepCPU();
		}

		hkFxWorldGpu::ExecuteOptions options = 
			hkFxWorldGpu::ExecuteOptions( 
			hkFxWorldGpu::GPU_PREPARE_RENDERING_DATA |  //Avoids context switch later when reading vertices
			hkFxWorldGpu::GPU_SIM_INTEGRATION |
			hkFxWorldGpu::GPU_SIM_NARROWPHASE );

		pFXWorld->startStepGPU( options );
		// Do some CPU work..
		pFXWorld->finishStepGPU();	

        //increment all existing system's stepped gpu
		for(SYSTEM_HASH* pCurSystemHash = HashGetFirst(mtSystemsBodiesIds); pCurSystemHash; pCurSystemHash = HashGetNext(mtSystemsBodiesIds, pCurSystemHash))
            if(pCurSystemHash->system.gpuStepped < NUM_GPU_STEPS_NEEDED + NUM_GPU_STEPS_NEEDED_AVOID_INTERSYSTEM_COLLISIONS+100)  pCurSystemHash->system.gpuStepped++;
        for(SYSTEM_PARTICLE_HASH* pCurSystemHash = HashGetFirst(gtSystemParticlesIds); pCurSystemHash; pCurSystemHash = HashGetNext(gtSystemParticlesIds, pCurSystemHash))
           if(pCurSystemHash->gpuStepped < NUM_GPU_STEPS_NEEDED + NUM_GPU_STEPS_NEEDED_AVOID_INTERSYSTEM_COLLISIONS+100)  pCurSystemHash->gpuStepped++; 


#ifdef CHECK_COLLISION_PROBLEMS
			
		    if( stepsElapsedSinceLastAdd < NUM_STEPS_FOR_ADD + 100) stepsElapsedSinceLastAdd++;
			if( stepsElapsedSinceLastRem < NUM_STEPS_FOR_REM + 100) stepsElapsedSinceLastRem++;

#endif CHECK_COLLISION_PROBLEMS


		if(m_pFXWorld->isCpuThreadingEnabled())
		{
			GetBridge()->sync(); //Need to sync here outside of the simulation
			ChangeSimulationState();
			pFXWorld->startStepCPU();
		}

		m_bStepFirstTime = true;


#ifdef DEBUG_HAVOK_FX
		//pFXWorld->syncBodies(hkFxWorld::SyncDirection::GPU_TO_CPU);
		hkFxDebugUtility::hkFxDebugVisualizeWorld( *pFXWorld);
		
		static int iDebugCount;

		if( m_hH3DebugBody > -1 || m_hFXDebugBody > -1)
		{
			DebugDrawRigidBodyInBridge( m_pFXWorld, m_pBridge, m_hH3DebugBody);
			DebugDrawRigidBodyInFXWorld( m_pFXWorld, m_hFXDebugBody);

			if(++iDebugCount > 5)
			{
				iDebugCount = 0;
				m_hH3DebugBody = -1;
				m_hFXDebugBody = -1;
			}
		}
#endif
	}
	//-------------------------------------------------------------------------------------------------
	void CreateSystems()
	{
		for(SYSTEM_HASH* pCurSystemHash = HashGetFirst(mtSystemsBodiesIds); pCurSystemHash; pCurSystemHash = HashGetNext(mtSystemsBodiesIds, pCurSystemHash))
		{
			if(pCurSystemHash->system.nState == SYSTEM_STATE_NEW || pCurSystemHash->system.nState == SYSTEM_STATE_DORMANT)
			{
				if( pCurSystemHash->system.nState == SYSTEM_STATE_NEW )
				{
					pCurSystemHash->system.nState = SYSTEM_STATE_RUNNING;
					hkFxShapeHandle sHandle = m_pFXWorld->addShape(pCurSystemHash->system.ShapeRep);
					ASSERT( sHandle != 0 );
 					m_numFXShapes ++;

					//tell the system its shape id
					pCurSystemHash->system.shapeHandle = sHandle;
					//tell the shader the shape id of the system
					SetParam("shapeIDsArray",float(sHandle),pCurSystemHash->nId);
					static unsigned int time = 0;
					time++;
					pCurSystemHash->system.timeCreated = time;
				}

				m_pFXWorld->setRigidBodyBodyCollision( 0, pCurSystemHash->system.nCollisionGroup, true);  //Set up our system to collide with landscapes
				m_pFXWorld->setRigidBodyBodyCollision( 2, pCurSystemHash->system.nCollisionGroup, true);  //Set up our system to collide with Ragdolls
				m_pFXWorld->setRigidBodyBodyCollision( pCurSystemHash->system.nCollisionGroup, pCurSystemHash->system.nCollisionGroup, true); //Turn on collisions with self

				pCurSystemHash->system.nState = SYSTEM_STATE_RUNNING;

#ifdef CHECK_COLLISION_PROBLEMS
			   stepsElapsedSinceLastAdd = 0;
#endif CHECK_COLLISION_PROBLEMS

			}
		}
		for(SYSTEM_PARTICLE_HASH* pCurParticleHash = HashGetFirst(gtSystemParticlesIds); pCurParticleHash; pCurParticleHash = HashGetNext(gtSystemParticlesIds, pCurParticleHash))
		{
			if(pCurParticleHash->nState == SYSTEM_STATE_NEW)
			{
					pCurParticleHash->nState = SYSTEM_STATE_RUNNING;
					m_pParticleEmitters[0]->m_active = true;
			}
		}
	}

	//-------------------------------------------------------------------------------------------------
	void RemoveSystems()
	{
		for(SYSTEM_HASH* pCurSystemHash = HashGetFirst(mtSystemsBodiesIds); pCurSystemHash; pCurSystemHash = HashGetNext(mtSystemsBodiesIds, pCurSystemHash))
		{
			int tempCount = HashGetCount(mtSystemsBodiesIds);

			if(pCurSystemHash->system.nState == SYSTEM_STATE_DIRTY)
			{
				ASSERT( pCurSystemHash->system.shapeHandle != 0 );  //Don't want to pack away any systems that are using our landscape index
				m_pFXWorld->setRigidBodyBodyCollision( 0, pCurSystemHash->system.nCollisionGroup, false);  //Turn off collisions with landscapes
				m_pFXWorld->setRigidBodyBodyCollision( 2, pCurSystemHash->system.nCollisionGroup, false);  //Turn off collisions with Ragdolls
				m_pFXWorld->setRigidBodyBodyCollision( pCurSystemHash->system.nCollisionGroup, pCurSystemHash->system.nCollisionGroup, false); //Turn off collisions with self

				m_DeactivatedSystems.Add( pCurSystemHash->system );

				HashRemove(mtSystemsBodiesIds,pCurSystemHash->nId);
				pCurSystemHash = HashGetFirst(mtSystemsBodiesIds);  //Removed node so we need to start at the tops

				//for remove system
#ifdef CHECK_COLLISION_PROBLEMS
	          stepsElapsedSinceLastRem = 0;
#endif CHECK_COLLISION_PROBLEMS

			}
			else if(pCurSystemHash->system.nState == SYSTEM_STATE_LOST)  //Actually kill the system 
			{
				//remove any left over bodies
				for(int j = 0; j < pCurSystemHash->system.nUsedBodies && pCurSystemHash->system.hRegionStart != HKFX_INVALID_HANDLE; ++j)
					m_pFXWorld->removeRigidBody( pCurSystemHash->system.hRegionStart );  //Remove bodies until we reach nCount or run out of bodies

				//remove shape from world
				if( pCurSystemHash->system.shapeHandle >= NUM_RESERVED_SHAPES )
				{
					m_pFXWorld->removeShape(pCurSystemHash->system.shapeHandle);
					m_numFXShapes--;
				}

				
				//remove shape id from shader
				SetParam("shapeIDsArray",-1,pCurSystemHash->nId);
				//unassign shader constant
				costantParameterAssigned[pCurSystemHash->system.uniformParamId] = false;
				m_pCollisionGroupPool->FreeID(pCurSystemHash->system.nCollisionGroup);

				HashRemove(mtSystemsBodiesIds, pCurSystemHash->nId);
				pCurSystemHash = HashGetFirst(mtSystemsBodiesIds);  //Removed node so we need to start at the top

				//for remove system
#ifdef CHECK_COLLISION_PROBLEMS
	          stepsElapsedSinceLastRem = 0;
#endif CHECK_COLLISION_PROBLEMS


			}
		}


		for(SYSTEM_PARTICLE_HASH* pCurParticleHash = HashGetFirst(gtSystemParticlesIds); pCurParticleHash; pCurParticleHash = HashGetNext(gtSystemParticlesIds, pCurParticleHash))
		{
			if(pCurParticleHash->nState == SYSTEM_STATE_DIRTY)
			{
				//trace( "\nRemoving system %d\n\n", sysid );

				//we are implicitly assuming that there is only one emitter right now
				ASSERT_RETURN(m_nParticleEmitterCount==1);

				//Hack, we only want to disable emitter when there are no more systems
				if(HashGetCount(gtSystemParticlesIds) == 0)
					m_pParticleEmitters[0]->m_active = false;

				HashRemove(gtSystemParticlesIds, pCurParticleHash->nId);
				pCurParticleHash = HashGetFirst(gtSystemParticlesIds);  //Removed node so we need to start at the top
			}
		}
	}

	//-------------------------------------------------------------------------------------------------
	void AddPendingFXRigidBodies()
	{
		for(UINT i = 0; i < m_PendingRigidBodies.Count(); i++)
		{
			hkFxRigidBody body = m_PendingRigidBodies[i];

			int sysid = body.m_shapeIndex;   //Using shape index to store system ID
			SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
			if ( pSystemHash && pSystemHash->system.nState == SYSTEM_STATE_RUNNING && body.m_shapeIndex != 0)  //World may remove system if it is badly behaved
			{
				//ASSERT( body.m_shapeIndex != 0 );
				body.m_shapeIndex = pSystemHash->system.shapeHandle;

				hkFxRigidBodyHandle handle = pSystemHash->system.hRegionStart + pSystemHash->system.nUsedBodies;

				if( handle < pSystemHash->system.hRegionStart + pSystemHash->system.nRegionSize )
					m_pFXWorld->updateRigidBody ( pSystemHash->system.hRegionStart + pSystemHash->system.nUsedBodies, body );
				else
					m_pFXWorld->addRigidBody( body );

				++ pSystemHash->system.nUsedBodies;
			}
		}
		m_PendingRigidBodies.Clear();
	}

	//-------------------------------------------------------------------------------------------------
	void AddPendingH3RigidBodies()
	{
		for(UINT i = 0; i < m_PendingH3Bodies.Count(); i++)
		{
			if ( ! bCanAddH3RigidBodies() )
				break;
	
			m_pBridge->addRigidBody( m_PendingH3Bodies[i].pRigidBody, m_PendingH3Bodies[i].AddBodyParams);
		}
		m_PendingH3Bodies.Clear();
	}

	bool AddFXRigidBody(hkFxRigidBody& body, int sysid)
	{
		if( !bCanAddFXRigidBodies() )
			return false;

		SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
	
		if( !pSystemHash )
			return false;

		if( pSystemHash->system.nState == SYSTEM_STATE_DIRTY || pSystemHash->system.nState == SYSTEM_STATE_LOST)
			return false;

		body.m_collisionGroup = pSystemHash->system.nCollisionGroup; //Set collision group to that of the system

		m_PendingRigidBodies.Add(body);
		
		return true;
	}
	//-------------------------------------------------------------------------------------------------
	void ChangeSimulationState()
	{
		for(SYSTEM_HASH* pCurSystemHash = HashGetFirst(mtSystemsBodiesIds); pCurSystemHash; pCurSystemHash = HashGetNext(mtSystemsBodiesIds, pCurSystemHash))
            if(pCurSystemHash->system.gpuStepped == NUM_GPU_STEPS_NEEDED_AVOID_INTERSYSTEM_COLLISIONS)
			{
				m_pFXWorld->setRigidBodyBodyCollision( pCurSystemHash->system.nCollisionGroup, pCurSystemHash->system.nCollisionGroup, true);
			}
			else if(pCurSystemHash->system.gpuStepped > 100)
			{
				if( pCurSystemHash->system.hRegionStart < 0 )
					pCurSystemHash->system.nState = SYSTEM_STATE_DIRTY;
			}

#ifdef CHECK_COLLISION_PROBLEMS
        
		hkFxWorld::FxStats curStats = m_pFXWorld->getCurrentStats();

		if( curStats.m_numCollisionBatches > MAX_NUM_COLLIDING_BATCHES_TOLERATED && 
			    stepsElapsedSinceLastAdd >= NUM_STEPS_FOR_ADD && 
				stepsElapsedSinceLastRem >= NUM_STEPS_FOR_REM)
			{ 
				unsigned int lowestTime = -1;
                SYSTEM_HASH* HashToRemove = NULL;

				for(SYSTEM_HASH* pCurSystemHash = HashGetFirst(mtSystemsBodiesIds); pCurSystemHash; pCurSystemHash = HashGetNext(mtSystemsBodiesIds, pCurSystemHash))
				{    if( pCurSystemHash->system.nUsedBodies > 300 ) //crude check for no hive blade
					{
                        if( lowestTime == -1 || pCurSystemHash->system.timeCreated<lowestTime)
						{
							lowestTime = pCurSystemHash->system.timeCreated;
							HashToRemove = pCurSystemHash; 
						}	
				    }
				}   
				
				if(HashToRemove)
				  HashToRemove->system.nState = SYSTEM_STATE_LOST;
										
		   }
#endif

		CreateSystems();
		AddPendingFXRigidBodies();
		AddPendingH3RigidBodies();
		RemoveSystems();
	}
	void AddAction(
		hkFxRigidBodyBehavior * pAction );

	void AddParticleBehavior(
		hkFxParticleBehavior * pParticleAction );

	void AddParticleEmitter(
		hkFxParticleEmitter * pParticleEmitter );
	
	void RemoveParticleEmitter();

	void SetParam( 
		HAVOKFX_PHYSICS_SYSTEM_PARAM eParam, 
		const hkVector4 & vVal,int sysid);

	void HavokFXWorldWrapper::SetParam( 
		char* szName, 
		float fValue,
		int sysid);

	void SetParam( 
		HAVOKFX_PHYSICS_SYSTEM_PARAM eParam, 
		float fVal,int sysid);

	bool NumVerticesToSkip(int sysid, int& vertexCount);

	void notifyRegionChange( hkFxShapeHandle regionShapeHandle, hkFxRigidBodyHandle oldFirstBody, hkFxRigidBodyHandle newFirstBody, int regionSize )
	{
		int count = HashGetCount( mtSystemsBodiesIds);
		for(SYSTEM_HASH* pCurSystemHash = HashGetFirst(mtSystemsBodiesIds); pCurSystemHash; pCurSystemHash = HashGetNext(mtSystemsBodiesIds, pCurSystemHash))
			//Since our systems are defined by shapes we need to update the region the system occupies
			if(pCurSystemHash->system.shapeHandle == regionShapeHandle) 
			{
				pCurSystemHash->system.hRegionStart = newFirstBody;
				pCurSystemHash->system.nRegionSize = regionSize;
			}

		for(UINT i = 0; i < m_DeactivatedSystems.Count(); ++i)
			if(m_DeactivatedSystems[i].shapeHandle == regionShapeHandle)
			{
				m_DeactivatedSystems[i].hRegionStart = newFirstBody;
				m_DeactivatedSystems[i].nRegionSize = regionSize;
			}

	}

	void KilSystem( int sysid )
	{
		SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);

		if(pSystemHash)
		{
			if(pSystemHash->system.nUsedBodies == 0)
				pSystemHash->system.nState = SYSTEM_STATE_LOST;
			else
				pSystemHash->system.nState = SYSTEM_STATE_DIRTY;
		}
	}

	bool AddSystem(int sysid,HAVOKFX_PARTICLE_TYPE type)
	{
		if(type == HAVOKFX_RIGID_BODY)
		{
			SYSTEM_HASH * pSystemHash = NULL;

			if( m_DeactivatedSystems.Count() > 0 )
			{
				//add the system to the hash
				HashAdd(mtSystemsBodiesIds,sysid);
     			pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
				ASSERT_RETFALSE( pSystemHash );


				pSystemHash->system = m_DeactivatedSystems[0];
				m_DeactivatedSystems.RemoveByIndex(0);

				pSystemHash->system.nState = SYSTEM_STATE_DORMANT;
			}
			else if(mtSystemsBodiesIds.GetCount()<m_iMaxSystems)
			{  
				//trace( "\nAdding rigid body system %d\n\n", sysid );
		        
				hkFxShape fxScarabShape;
				fxScarabShape.setDefaults(); 
				hkTransform shapeFxTransform; 
				shapeFxTransform.setIdentity();

				//assign this system a free id for the constants in the shader
				int systemUniformParamId = -1;
				for(int i=0;i<m_iMaxSystems;i++)
				{	if(costantParameterAssigned[i]==false)
					{
						systemUniformParamId = i;  
						costantParameterAssigned[i]= true;
						break;
					}
				}
				ASSERT_RETFALSE(systemUniformParamId>=0);


				hkGeometry geom;
				hkVector4 spheres[hkFxHeightMapShapeRep::NUM_SPHERES];
				float fSphereRadius = 0.06f; 
				hkShape* pShape = new hkSphereShape ( fSphereRadius );
				for(int num=0;num<12;num++)
					spheres[ num ].set( 0.0f, 0.0f, 0.0f, fSphereRadius );
				hkFxHeightMapShapeRep* shapeRep = hkFxBuildCubeMapRepresentation( pShape, 16, 0 ); //hkFxBuildCubeMapRepresentation( pShape, 16, 0, fxScarabShape );
				g_ScarabShapeRep = shapeRep;
				hkString::memCpy( shapeRep->m_spheres, spheres, sizeof(hkVector4)*hkFxHeightMapShapeRep::NUM_SPHERES);
				fxScarabShape.setShapeRep(shapeRep);
				fxScarabShape.m_invLinearDamping.setAll ( 0.98f ); //0.98f 
				fxScarabShape.m_invAngularDamping.setAll( 0.98f );//0.98f
				hkReal mass = fSphereRadius * fSphereRadius * fSphereRadius;
				fxScarabShape.m_invInertia.setAll( 1.0f / (mass*fSphereRadius) );
				fxScarabShape.m_invInertia(3) = 1.0f / mass; // stores 1/mass

				//add the system to the hash
				HashAdd(mtSystemsBodiesIds,sysid);
     			pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
				ASSERT_RETFALSE( pSystemHash );

				pSystemHash->system.ShapeRep = fxScarabShape;

				pSystemHash->system.hRegionStart = -2;
				pSystemHash->system.nRegionSize = 0;

				pSystemHash->system.uniformParamId = systemUniformParamId; //assign a shape uniform parameter id to the system
				pSystemHash->system.nCollisionGroup = m_pCollisionGroupPool->GetID();
				
				pSystemHash->system.nState = SYSTEM_STATE_NEW;
			}
			else
				return false;

			pSystemHash->system.gpuStepped = 0;
			pSystemHash->system.nUsedBodies = 0;
			pSystemHash->system.nParticleType = HAVOKFX_RIGID_BODY;

			return true; 
		}
		else
		{
			ASSERT_RETFALSE(m_nParticleEmitterCount==1);
			HashAdd(gtSystemParticlesIds,sysid);
     		SYSTEM_PARTICLE_HASH * pSystemHash = HashGet(gtSystemParticlesIds,sysid);
			pSystemHash->nState = SYSTEM_STATE_NEW;
			pSystemHash->gpuStepped = 0;

			//pSystemHash->system.nParticleType = HAVOKFX_PARTICLE;
			return true;
		}

	return false;
	}

	void putSystemToSleep( int sysid )
	{
	    SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
		if(pSystemHash)
		{	m_pFXWorld->setRigidBodyBodyCollision( 0, pSystemHash->system.nCollisionGroup, false);  //Turn off collisions with landscapes
			m_pFXWorld->setRigidBodyBodyCollision( 2, pSystemHash->system.nCollisionGroup, false);  //Turn off collisions with Ragdolls
			m_pFXWorld->setRigidBodyBodyCollision( pSystemHash->system.nCollisionGroup, pSystemHash->system.nCollisionGroup, false); //Turn off collisions with self
		}
	}

	void wakeupSystem( int sysid )
	{
	    SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
		if(pSystemHash)
		{	m_pFXWorld->setRigidBodyBodyCollision( 0, pSystemHash->system.nCollisionGroup, true);  //Turn off collisions with landscapes
			m_pFXWorld->setRigidBodyBodyCollision( 2, pSystemHash->system.nCollisionGroup, true);  //Turn off collisions with Ragdolls
			m_pFXWorld->setRigidBodyBodyCollision( pSystemHash->system.nCollisionGroup, pSystemHash->system.nCollisionGroup, true); //Turn off collisions with self
		}
	}

};

bool HavokFXWorldWrapper::NumVerticesToSkip(int sysid, int& vertexCount)
{
	SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);

    if(!pSystemHash)
	{
		vertexCount = 0;
		return FALSE;	
	}

	if(pSystemHash->system.gpuStepped < NUM_GPU_STEPS_NEEDED)
	{
		vertexCount = 0;
		return false;
	}

	//Skip all transforms before the start of our system
	vertexCount = (int)pSystemHash->system.hRegionStart;

	return true;
}

static HavokFXWorldWrapper * sgHavokFXWorld = NULL;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//called once in the beginning when application is started 
//-------------------------------------------------------------------------------------------------

void e_HavokFXSystemInit()
{
//-------------------------------------------------------------------------------------------------
// Disable uneeded assertions
//-------------------------------------------------------------------------------------------------
	hkError::getInstance().setEnabled(0x360fdc47, false); //Minrects complaint.
	hkError::getInstance().setEnabled(0x475d86b1, false); //Vector to small to normalize complaint.
	hkError::getInstance().setEnabled(0x360fdc6b, false);
	hkError::getInstance().setEnabled(0x394e9c6c, false); // Assert : 'i >= 0 && i < m_size'
	hkError::getInstance().setEnabled(0x21c8ab2a, false); //Assert : 'isNormalized4() hkVector4 too small for normalize4'
	hkError::getInstance().setEnabled(0x360fdc35, false); //Assert : '!m_bound'
	hkError::getInstance().setEnabled(0x3279fedd, false); // Assert : '(m_inGpuWork || m_transitionStraightFromCpuToGpu)
	hkError::getInstance().setEnabled(0x43240312, false); // Assert : 's Removing a non existant shape from the world.'
	hkError::getInstance().setEnabled(0x360fdc2a, false); // Warning : 'Could not add shape as it there is no room left in the preallocated shape array!
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
	
    HashInit( gtSystemParticlesIds, NULL, 16 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_HavokFXSystemClose()
{
	//remove all the systems from the world
	//rigid body systems handled in destructor
	SYSTEM_PARTICLE_HASH* pCurParticleSystemHash;
	while( pCurParticleSystemHash = HashGetFirst(gtSystemParticlesIds) )
	{
		e_HavokFXRemoveSystem(pCurParticleSystemHash->nId);
	}

	if ( sgHavokFXWorld )
	{
		delete sgHavokFXWorld;
		sgHavokFXWorld = NULL;
	}
	
	HashFree( gtSystemParticlesIds );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_HavokFXDeviceInit()
{
	if ( ! e_HavokFXIsEnabled() )
		return;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dxC_HavokFXDeviceLost()
{	
	if ( !e_HavokFXIsEnabled() )
		return;

	if ( ! sgHavokFXWorld )
		return;

	hkFxWorldGpu * pFXWorld = sgHavokFXWorld->GetWorld();
	if ( pFXWorld )
		pFXWorld->releaseD3D();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dxC_HavokFXDeviceReset()
{	
	if ( !e_HavokFXIsEnabled() )
		return;

	if ( ! sgHavokFXWorld )
		return;	

	hkFxWorldGpu * pFXWorld = sgHavokFXWorld->GetWorld();
	if ( pFXWorld )
		pFXWorld->resetD3D();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dxC_HavokFXDeviceRelease( )
{	
	if ( !e_HavokFXIsEnabled() )
		return;

	if(sgHavokFXWorld)
	{
		delete sgHavokFXWorld;
		sgHavokFXWorld = NULL;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define HAVOKFX_DRIVER_HIGH		393230
#define HAVOKFX_DRIVER_LOW		664481

BOOL e_HavokFXIsEnabled()
{
#ifndef HAVOKFX_ENABLED
	return FALSE;
#endif // HAVOKFX_ENABLED

#if defined(ENGINE_TARGET_DX9)

#ifdef _DEBUG
	GLOBAL_DEFINITION * pGlobalDef = DefinitionGetGlobal();
	if ( ( pGlobalDef->dwFlags & GLOBAL_FLAG_HAVOKFX_ENABLED ) == 0 )
		return FALSE;
#endif
	return ( dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) == DT_NVIDIA_VENDOR_ID &&
			 dx9_CapGetValue( DX9CAP_MAX_VS_VERSION ) >= D3DVS_VERSION(3, 0)
			 //&& dx9_CapIsDriverVersion( HAVOKFX_DRIVER_HIGH, HAVOKFX_DRIVER_LOW) 
			 );
#else
	return FALSE;
#endif

}

//-------------------------------------------------------------------------------------------------
//-----------------------------------------------------------`--------------------------------------

static void sApplyBoundingBoxToHavokAabb(
	const BOUNDING_BOX & tBoundingBox,
	hkAabb & tHavokBoundingBox )
{
	tHavokBoundingBox.m_min.set( tBoundingBox.vMin.fX, tBoundingBox.vMin.fY, tBoundingBox.vMin.fZ );
	tHavokBoundingBox.m_max.set( tBoundingBox.vMax.fX, tBoundingBox.vMax.fY, tBoundingBox.vMax.fZ );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_HavokFXInitWorld(
	hkWorld * pWorld,
	const BOUNDING_BOX & tBoundingBox,
	BOOL bAddFloor,
	float fFloorBottom,
	int maxSystems)
{
	if ( ! e_HavokFXIsEnabled() )
		return;

//-------------------------------------------------------------------------------------------------
// Disable uneeded assertions
//-------------------------------------------------------------------------------------------------
	hkError::getInstance().setEnabled(0x360fdc47, false); //Minrects complaint.
	hkError::getInstance().setEnabled(0x475d86b1, false); //Vector to small to normalize complaint.
	hkError::getInstance().setEnabled(0x360fdc6b, false);// Assert : 'dataSize > 0'
	hkError::getInstance().setEnabled(0x394e9c6c, false);// Assert : 'i >= 0 && i < m_size'
	hkError::getInstance().setEnabled(0xf0323454, false); //Assert : 'newCollisionGroup >= 0 && newCollisionGroup < in.m_numCollisionGroups Invalid CollisionGroup
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

	ASSERT_RETURN( pWorld );


	hkAabb tHavokBoundingBox;
	sApplyBoundingBoxToHavokAabb( tBoundingBox, tHavokBoundingBox );

	hkFxWorldCinfo cinfo;
	cinfo.setDefaults();

    cinfo.m_gpuInterface = hkGpuInterfaceCreate(FILE_PATH_HAVOKFX_SHADERS, false);

	cinfo.setBroadphaseExtents( tHavokBoundingBox );

	const hkReal fxDt = HAVOKFX_TIMESTEP * HAVOKFX_TIMESTEP;			
	
	cinfo.m_deltaTime = HAVOKFX_TIMESTEP;
	cinfo.m_gravity.set(0.0f, 0.0f, 0.0f, 0.0f);//( 0,0, -9.81f * fxDt);
	cinfo.m_broadphaseMemSizeGuessInBytes = 15204768;

	cinfo.m_maxNumPlanarShapes = MAX_NUM_LANDSCAPES; //landscapes
	cinfo.m_maxNumCubeShapes = MAX_NUM_SYSTEMS + MAX_NUM_H3_SHAPES; //fx shapes and havok3 shapes (other than landscapes)
	cinfo.m_maxNumRigidBodies =  MAX_NUM_RIGIDBODIES;
	cinfo.m_numRigidCollisionGroups = MAX_NUM_SYSTEMS + 3; //Max number of user systems plus 3 hkFx managed systems
	cinfo.m_collisionsAllocedPerBody  = MAX_NUM_COLLISIONS_PER_BODY;

	cinfo.m_landscapeDepth = -100.0f;
	//cinfo.m_planarShapeRes = 128;
	//cinfo.m_cubeShapeRes = 128;
	cinfo.m_enableLandscapeFiltering = true;
	cinfo.m_verticalRepulsionFactor = 100.0f;

	GLOBAL_DEFINITION * pGlobalDef = DefinitionGetGlobal();
	bool bThreading =  ( pGlobalDef->dwFlags & GLOBAL_FLAG_MULTITHREADED_HAVOKFX_ENABLED ) != 0;

	cinfo.m_enableThreadingCPU = bThreading;
	cinfo.m_enableThreadingGPU = false;  //Don't really need it now since we're not doing AFR physics.

	
    //-------particles stuff----------------------------------------------------------------------------
    cinfo.m_maxNumParticles = 16384;
    cinfo.m_particleInfo.m_gravity.set( 0.0f, 0.0f, -9.81f * fxDt);
    cinfo.m_particleInfo.m_invLinearDamping = 0.98f;
    cinfo.m_particleInfo.m_friction = 0.99f;
	cinfo.m_particleInfo.m_restitution = 0.01f;
    cinfo.m_particleInfo.m_minVelocity = 0.001f;
    cinfo.m_particleInfo.m_enableOneLandscapeCollisions = true; 
    cinfo.m_particleInfo.m_enableParticleBodyCollisions = true;
	cinfo.m_particleInfo.m_enableParticleParticleCollisions = true;
	cinfo.m_particleInfo.m_maxNumPairsPerParticle = 30;
	cinfo.m_particleInfo.m_collisionTolerance = 0.01f;
    //-------------------------------------------------------------------------------------------------
	{
#if defined(ENGINE_TARGET_DX9)  //KMNV TODO HACK: hkFxWorldGpu won't work with a dx10 device so we can't use dxC_GetDevice() 
		LPD3DCDEVICE pDevice = dxC_GetDevice();
		ASSERT_RETURN( pDevice );
		sgHavokFXWorld = new HavokFXWorldWrapper( cinfo, maxSystems, FILE_PATH_HAVOKFX_SHADERS, pDevice );
#else  // defined(ENGINE_TARGET_DX9)
		return;
#endif // defined(ENGINE_TARGET_DX9)

	cinfo.m_gpuInterface->removeReference(); // let the world delete it
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


bool e_HavokAddSystem(int sysid,HAVOKFX_PARTICLE_TYPE type)
{

	if( !sgHavokFXWorld)
		return FALSE;

	return sgHavokFXWorld->AddSystem( sysid, type);
}


void e_HavokFXRemoveSystem(int sysid)
{
	if( !sgHavokFXWorld)
		return;

	sgHavokFXWorld->KilSystem( sysid );
}

//-------------------------------------------------------------------------------------------------
//called each time a level is changed
//-------------------------------------------------------------------------------------------------

void e_HavokFXCloseWorld()
{
	//Rigid body systems now removed in destructor
	for(SYSTEM_PARTICLE_HASH* pCurParticleSystemHash = HashGetFirst(gtSystemParticlesIds); pCurParticleSystemHash; pCurParticleSystemHash = HashGetNext(gtSystemParticlesIds, pCurParticleSystemHash))
		e_HavokFXRemoveSystem(pCurParticleSystemHash->nId);

	if(sgHavokFXWorld)
	{
		delete sgHavokFXWorld;
		sgHavokFXWorld = NULL;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_HavokFXStepWorld( float )
{
	if(sgHavokFXWorld)
		sgHavokFXWorld->Step();
}


bool e_HavokFXGetSystemVerticesToSkip ( int sysid, int & nVerticesToSkip)
{
	if ( ! sgHavokFXWorld )
		return FALSE;

	hkFxWorldGpu * pFXWorld = sgHavokFXWorld->GetWorld();
	if ( ! pFXWorld )
		return FALSE ;

    SYSTEM_PARTICLE_HASH * pSystemParticlesHash = HashGet(gtSystemParticlesIds,sysid);
    if(pSystemParticlesHash) //This is a havok FX particle system, so we assume it starts at the beginning
	{
	    nVerticesToSkip = 0;
		return FALSE;
	}

	return sgHavokFXWorld->NumVerticesToSkip( sysid, nVerticesToSkip);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_HavokFXGetSystemParticleCount ( int sysid )
{
	if( ! sgHavokFXWorld)
		return 0;

	hkFxWorldGpu * pFXWorld = sgHavokFXWorld->GetWorld();
	if ( ! pFXWorld )
		return 0;

	//temp sarah !! change 

	return pFXWorld->getNumUsedParticles();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
 int dxC_HavokFXGetRigidAllTransforms(
	IDirect3DVertexBuffer9 * ppRigidBodyTransforms[ HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS ],
	LPD3DCVB* posVB,
	LPD3DCVB* velVB
)
{
#if defined(ENGINE_TARGET_DX9)
 	if ( ! sgHavokFXWorld )
		 return FALSE;

	hkFxWorldGpu * pFXWorld = sgHavokFXWorld->GetWorld();


	if(ppRigidBodyTransforms)
		for ( int i = 0; i < HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS; i++ )
		{
			if ( ! pFXWorld )
				ppRigidBodyTransforms[ i ] = NULL;
			else
				pFXWorld->getTransformVertexArray( i, ppRigidBodyTransforms[ i ] );
		}
	
	 if(posVB)
   		pFXWorld->getParticlePositionVertexArray(*posVB); 
	 
	 if(velVB)
		pFXWorld->getParticleVelocityVertexArray(*velVB);

 #elif defined(ENGINE_TARGET_DX10)
//KMNV TODO: Insert DX10 hkfx here, we can work around this with a readback if need be
#endif

	 return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool e_HavokFXAddH3RigidBody( 
	hkRigidBody * body, 
	BOOL bLandscape,
	FX_BRIDGE_BODY_SAMPLEDIR sampleDir
	)
{
	if ( ! sgHavokFXWorld )	
    	return false;

	if ( ! sgHavokFXWorld->bCanAddH3RigidBodies())
    	return false;

	BridgeRigidBody addbody;
	addbody.pRigidBody = body;
   
	if(bLandscape)
		addbody.AddBodyParams.m_sampleDir = (hkFxPhysicsBridge::SampleDir)sampleDir;  //KMNV: quick dirty
	else
		addbody.AddBodyParams.m_centerShape = TRUE;

	addbody.AddBodyParams.m_maxHeightDifferential = bLandscape ? 2.0f : 10000.0f;  //2.0 To do walls for landscapes 10000.0f to do normal action for ragdolls
	addbody.AddBodyParams.m_extraRadius = 0.0f;
	addbody.AddBodyParams.m_collisionGroup = bLandscape ? 0 : 2;  //0 Landscape 2 ragdoll

	sgHavokFXWorld->m_PendingH3Bodies.Add( addbody );

	return true; //We will return an invalid handle if we failed to add the body
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool e_HavokFXAddFXRigidBody(
	const MATRIX & mWorld,
	const VECTOR & vVelocityIn,
	int sysid)
{
	if( !sgHavokFXWorld)	
		return false;

	hkFxRigidBody body;
	body.setDefaults();
    
    body.m_shapeIndex = sysid;  //Using shape index to store system ID

	body.m_transform.set4x4ColumnMajor( (float *) &mWorld );
	body.m_orientation.setAndNormalize( body.m_transform.getRotation() );
	VECTOR vVelocity = vVelocityIn;
	body.m_linearVelocity.set( vVelocity.x, vVelocity.y, vVelocity.z );
	body.m_angularVelocity.set(0,0,0);

	return sgHavokFXWorld->AddFXRigidBody( body, sysid);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_HavokFXRemoveH3RigidBody (
	hkRigidBody * body )
{
	if( ! sgHavokFXWorld)
		return;

	for ( UINT i = 0; i < sgHavokFXWorld->m_PendingH3Bodies.Count(); i++ )
	{
		if ( sgHavokFXWorld->m_PendingH3Bodies[ i ].pRigidBody == body )
		{
			sgHavokFXWorld->m_PendingH3Bodies.RemoveByIndex( i );
			return;
		}
	}
	hkFxWorldGpu * pFXWorld = sgHavokFXWorld->GetWorld();
    
	if ( ! pFXWorld )	
		return;

	int numH3Objects = pFXWorld->getNumSimulatedRigidBodiesWithPriority( HK_SIM_PRIORITY_FIXED_CUBEMAP );
	if ( numH3Objects < 1)
		return; // none to remove

	hkFxPhysicsBridge * pBridge = sgHavokFXWorld->GetBridge();
	if ( pBridge )
		pBridge->removeRigidBody(body);

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokFXWorldWrapper::AddAction(
	hkFxRigidBodyBehavior * pAction )
{
	m_nActionCount++;
	m_pActions = (hkFxRigidBodyBehavior **) REALLOC( NULL, m_pActions, m_nActionCount * sizeof(hkFxRigidBodyBehavior *));

	m_pActions[ m_nActionCount - 1 ] = pAction;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokFXWorldWrapper::AddParticleBehavior(
									hkFxParticleBehavior * pParticleAction )
{
	m_nParticleBehaviourCount++;
	m_pParticleActions = (hkFxParticleBehavior **) REALLOC( NULL, m_pParticleActions, m_nParticleBehaviourCount * sizeof(hkFxParticleBehavior *));

	m_pParticleActions[ m_nParticleBehaviourCount - 1 ] = pParticleAction;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokFXWorldWrapper::AddParticleEmitter(
	hkFxParticleEmitter * pParticleEmitter )
{
	m_nParticleEmitterCount++;
	m_pParticleEmitters = (hkFxParticleEmitter **) REALLOC( NULL, m_pParticleEmitters, m_nParticleEmitterCount * sizeof(hkFxParticleEmitter *));

	m_pParticleEmitters[ m_nParticleEmitterCount - 1 ] = pParticleEmitter;
}

void HavokFXWorldWrapper::RemoveParticleEmitter()
{
	m_nParticleEmitterCount--;
	m_pParticleEmitters[ m_nParticleEmitterCount] = NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokFXWorldWrapper::SetParam( 
	HAVOKFX_PHYSICS_SYSTEM_PARAM eParam, 
	const hkVector4 & vValue,
	int sysid)
{
	ASSERT_RETURN( eParam >= 0 && eParam < NUM_HAVOKFX_PHYSICS_SYSTEM_PARAMS );

	const HAVOKFX_SYSTEM_PARAM_DATA * pParamData = &sgHavokSystemParamData[ eParam ];
	ASSERT_RETURN( pParamData->bIsVector );

	
	SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
	SYSTEM_PARTICLE_HASH * pSystemParticlesHash = HashGet(gtSystemParticlesIds,sysid);

	if(pSystemHash)
	{
		char szNameSystem[ 100 ];
		sprintf_s(szNameSystem,"%s[%d]", pParamData->szName,pSystemHash->system.uniformParamId);

		//if ( ! m_pvParams[ eParam ].equals3( vValue, 0.001f ) )
		{
			if ( pParamData->szName[ 0 ] != 0 )
			{
				for ( int i = 0; i < m_nActionCount; i++ )
				{
					m_pFXWorld->setBehaviorParameter( m_pActions[ i ], szNameSystem, vValue );
				}
			}
			m_pvParams[ eParam ] = vValue;
		}
	}
	else if(pSystemParticlesHash && eParam >= HAVOKFX_PHYSICS_SYSTEM_BODY_PARAMS)
	{
		
		//if ( ! m_pvParams[ eParam ].equals3( vValue, 0.001f ) )
		{
			if ( pParamData->szName[ 0 ] != 0 )
			{
				
				if(eParam >= HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_SPECIFIC_PARAMS && eParam <HAVOKFX_PHYSICS_SYSTEM_PARTICLE_BEHAVIOR_PARAMS )
				{	
					for ( int i = 0; i < m_nParticleBehaviourCount; i++ )
						m_pFXWorld->setParticleBehaviorParameter( m_pParticleActions[ i ],  pParamData->szName, vValue);
				}
				else if(eParam >= HAVOKFX_PHYSICS_SYSTEM_BODY_PARAMS && eParam < HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_UNIFORM_PARAMS) 
				{
					for ( int i = 0; i < m_nParticleEmitterCount; i++ )
						m_pFXWorld->setParticleEmitterParameter( m_pParticleEmitters[ i ],  pParamData->szName, vValue );
				}

			}
			m_pvParams[ eParam ] = vValue;
		}
	}
	else
		return; //ASSERT_RETURN(FALSE);

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokFXWorldWrapper::SetParam( 
								   char* szName, 
								   float fValue,
								   int sysid)
{


	SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
	SYSTEM_PARTICLE_HASH * pSystemParticlesHash = HashGet(gtSystemParticlesIds,sysid);
    
	
	if(pSystemHash)
	{	
		char szNameSystem[ 100 ];
		sprintf_s(szNameSystem,"%s[%d]",szName,pSystemHash->system.uniformParamId);

		hkVector4 vValue;
		vValue.set(fValue,0,0);
	   if (szName[ 0 ] != 0 )
		{
			for ( int i = 0; i < m_nActionCount; i++ )
			{
				m_pFXWorld->setBehaviorParameter( m_pActions[ i ], szNameSystem, vValue );
			}
		}
	}
	else
		return; //ASSERT_RETURN(FALSE);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HavokFXWorldWrapper::SetParam( 
	HAVOKFX_PHYSICS_SYSTEM_PARAM eParam, 
	float fValue,
	int sysid  )
{
	ASSERT_RETURN( eParam >= 0 && eParam < NUM_HAVOKFX_PHYSICS_SYSTEM_PARAMS );

	const HAVOKFX_SYSTEM_PARAM_DATA * pParamData = &sgHavokSystemParamData[ eParam ];
	ASSERT_RETURN( ! pParamData->bIsVector );

	SYSTEM_HASH * pSystemHash = HashGet(mtSystemsBodiesIds,sysid);
	SYSTEM_PARTICLE_HASH * pSystemParticlesHash = HashGet(gtSystemParticlesIds,sysid);

	if(pSystemParticlesHash && eParam >= HAVOKFX_PHYSICS_SYSTEM_BODY_PARAMS)
	{
		//if ( m_pvParams[ eParam ].getSimdAt( 0 ) != fValue )
		{
			if ( pParamData->szName[ 0 ] != 0 )
			{
				m_pvParams[ eParam ].set( fValue, 0, 0 );

				if( eParam == HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_RATE)
				{
					if(sgHavokFXWorld->m_nParticleEmitterCount==1)
					   sgHavokFXWorld->m_pParticleEmitters[0]->m_rate = fValue;
				}
				else if( eParam == HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_LIFETIME)
				{
					if(sgHavokFXWorld->m_nParticleEmitterCount==1)
						sgHavokFXWorld->m_pParticleEmitters[0]->m_particleLifetime = fValue;
				}
				else if( eParam == HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_LIFETIME)
				{
					if(sgHavokFXWorld->m_nParticleEmitterCount==1)
						sgHavokFXWorld->m_pParticleEmitters[0]->m_lifetime = fValue;
				}
				else if(eParam > HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_SPECIFIC_PARAMS) 
				{	
					for ( int i = 0; i < m_nParticleBehaviourCount; i++ )
						m_pFXWorld->setParticleBehaviorParameter( m_pParticleActions[ i ],  pParamData->szName, m_pvParams[ eParam ] );
				}
				else if(eParam >= HAVOKFX_PHYSICS_SYSTEM_BODY_PARAMS && eParam < HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_UNIFORM_PARAMS )
				{
					for ( int i = 0; i < m_nParticleEmitterCount; i++ )
						m_pFXWorld->setParticleEmitterParameter( m_pParticleEmitters[ i ],  pParamData->szName, m_pvParams[ eParam ] );
				}
			}
		}
	}
	else if( pSystemHash )
	{
		char szNameSystem[ 100 ];
		sprintf_s(szNameSystem,"%s[%d]", pParamData->szName,pSystemHash->system.uniformParamId);
    	//if ( m_pvParams[ eParam ].getSimdAt( 0 ) != fValue )
		{
			if ( pParamData->szName[ 0 ] != 0 )
			{
				m_pvParams[ eParam ].set( fValue, 0, 0 );
				for ( int i = 0; i < m_nActionCount; i++ )
				{
					m_pFXWorld->setBehaviorParameter( m_pActions[ i ], szNameSystem, m_pvParams[ eParam ] );
				}
			}
		}
	}
	else
		return; //ASSERT_RETURN(FALSE);
	

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_HavokFXSetSystemParam(
	HAVOKFX_PHYSICS_SYSTEM_PARAM eParam,
	const VECTOR4 & vValue,
	int sysid )
{
	hkVector4 vHavokVector;
	vHavokVector.set( vValue.fX, vValue.fY,	vValue.fZ, vValue.fW );

	sgHavokFXWorld->SetParam( eParam, vHavokVector,sysid );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_HavokFXSetSystemParam(
	HAVOKFX_PHYSICS_SYSTEM_PARAM eParam,
	float fValue,
	int sysid)
{
	sgHavokFXWorld->SetParam( eParam, fValue,sysid );
}

void e_DumpHavokFXReport(int iBody, bool bHavokFx)
{
#ifdef DEBUG_HAVOK_FX
	if ( ! sgHavokFXWorld)
		return;

	if(bHavokFx)
		sgHavokFXWorld->m_hFXDebugBody = iBody;
	else
		sgHavokFXWorld->m_hH3DebugBody = iBody;
#endif
	return;
}

/*
actual scarab shape :  doesn't work perfectly yet

//load the actual scarab shape
hkBinaryPackfileReader* reader = new hkBinaryPackfileReader();
hkIstream infile( "data\\havokfx\\scarab_default.hkx" );
HK_ASSERT( 0x215d080c, infile.isOk() );
reader->loadEntireFile(infile.getStreamReader());

hkVersionUtil::updateToCurrentVersion( *reader, hkVersionRegistry::getInstance() );

hkArray<hkFxShapeHandle> shapeHandles;

// Get a handle to the memory allocated by the reader, and add a reference to it
//m_allocatedData = reader->getAllocatedData();
//m_allocatedData->addReference();

// Get the top level object in the file
hkRootLevelContainer* container = static_cast<hkRootLevelContainer*>( reader->getContents( hkRootLevelContainerClass.getName() ) );
HK_ASSERT2( 0xa6451543, container, "Could not load root level obejct" );

// Get the root level components, if present
hkxScene*			sceneData;
hkPhysicsData*		physicsData;
hkFxPhysicsData*	fxData;
sceneData = static_cast<hkxScene*>( container->findObjectByType( hkxSceneClass.getName() ));
physicsData = static_cast<hkPhysicsData*>( container->findObjectByType( hkPhysicsDataClass.getName() ) );
fxData = static_cast<hkFxPhysicsData*>( container->findObjectByName( "Fx Data" ) );

hkArray<hkFxRigidBodySphereRep*> uniqueShapeReps;
hkArray<int> repToShapeMap; // give a rep in the fxData, which if the unique shapes does it use.
hkFxFileUtilities::findFxShapes( fxData, uniqueShapeReps, repToShapeMap );

hkArray<int> numEachShapeType;
numEachShapeType.setSize(uniqueShapeReps.getSize(), 0);
for (int rspi=0; rspi < repToShapeMap.getSize(); ++rspi)	
{
numEachShapeType[ repToShapeMap[rspi] ]++;
}

hkFxFileUtilities::addFxShapes(pHash->pWorld, uniqueShapeReps, shapeHandles, 16 );
*/
#endif // USE_HAVOK_4