//-------------------------------------------------------------------------------------------------
//
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//-------------------------------------------------------------------------------------------------
#pragma once



#define MOPP_FILE_EXTENSION "mop"

// If anyone EVER change the value of HAVOK_SHAPE_NONE,
// please change the corresponding hardcoded default value
// in excel.cpp for the "HavokShape" field.   -rli
enum {
	HAVOK_SHAPE_NONE	= 0,
	HAVOK_SHAPE_SPHERE  = 1,
	HAVOK_SHAPE_BOX		= 2,
	HAVOK_SHAPE_CAPSULE	= 3,
};

struct HAVOK_SHAPE_HOLDER;

struct HAVOK_IMPACT
{
	DWORD					dwFlags;
	VECTOR					vSource;
	VECTOR					vDirection;
	float					fForce;
	DWORD					dwTick;
};

#define HAVOK_IMPACT_FLAG_TWIST					MAKE_MASK( 0 )
#define HAVOK_IMPACT_FLAG_IGNORE_SCALE			MAKE_MASK( 1 )
#define HAVOK_IMPACT_FLAG_ON_CENTER				MAKE_MASK( 2 )
#define HAVOK_IMPACT_FLAG_MULTIPLIER_APPLIED	MAKE_MASK( 3 )
#define HAVOK_IMPACT_FLAG_ONLY_ONE				MAKE_MASK( 4 )

//#define MAYHEM_UPDATE_STEP 0.0166f
#define MAYHEM_UPDATE_STEP 0.0333f

#define COLFILTER_POSE_PHANTOM_LAYER	5
#define COLFILTER_POSE_PHANTOM_SYSTEM	5

void HavokSystemInit();

void HavokSystemClose();

void HavokInit( 
	struct GAME * pGame );

class hkFxWorldOgl;
void HavokInitWorld(
	class hkWorld** ppHavokWorld,
	BOOL bIsClient,
	BOOL bUseHavokFX,
	const struct BOUNDING_BOX * pBoundingBox = NULL,
	BOOL bSetUpDebugger = FALSE );

class hkWorld* HavokGetWorld( int nModelId, BOOL* pbUseHavokFX = NULL );

void HavokClose( 
	GAME * pGame );

void HavokCloseWorld(
	hkWorld ** pHavokWorld );

void s_HavokUpdate( 
	GAME * pGame,
	float fDelta );

void c_HavokSetupVisualDebugger(
	class hkWorld* physicsWorld );

void c_HavokStepVisualDebugger();

void c_PrintHavokMemoryStats();

void HavokCreateShapeFromMesh(
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	const char * pszMoppFilename, 
	int nVertexStructSize, 
	int nVertexBufferSize, 
	int nVertexCount, 
	void * pVertices, 
	int nIndexBufferSize, 
	int nTriangleCount, 
	WORD * pwIndexBuffer );

void HavokCreateShapeSphere( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	float fRadius );

void HavokCreateShapeBox( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	VECTOR & vSize );

void HavokCreateCapsule( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder,
	const VECTOR & vBoxSize );

void HavokSaveMoppCode( 
	const char * pszMoppFilename, 
	int nVertexStructSize, 
	int nVertexBufferSize, 
	int nVertexCount, 
	void * pVertices, 
	int nIndexBufferSize, 
	int nTriangleCount, 
	WORD * pwIndexBuffer );
int HavokSaveMultiMoppCode( //returns number of moppfiles created.
						   const char * pszFilename, 
						   int nVertexStructSize, 
						   int nVertexBufferSize, 
						   int nVertexCount, 
						   void * pVertices, 
						   int nIndexBufferSize, 
						   int nTriangleCount, 
						   WORD * pwIndexBuffer,
						   VECTOR vHavokShapeTranslation[]);

void HavokReleaseShape( 
	HAVOK_SHAPE_HOLDER & tHavokShapeHolder );

class hkRigidBody * HavokCreateBackgroundRigidBody( 
	struct GAME * pGame, 
	class hkWorld * pWorld,
	class hkShape * pHavokShape, 
	VECTOR * pPosition, 
	struct QUATERNION * qRotation,
		BOOL bFXAdd = false,
		BOOL bFXOnly = false);

class hkRigidBody * HavokCreateUnitRigidBody( 
	struct GAME * pGame, 
	class hkWorld * pWorld,
	class hkShape * pHavokShape, 
	const VECTOR & vPosition, 
	const VECTOR& vFaceDirection,
	const VECTOR & vUpDirection,
	float fBounce = 0.0,
	float fDampening = 0.0f,
	float fFriction = 0.0f );

void HavokReleaseRigidBody( 
	struct GAME * pGame, 
	class hkWorld * pWorld,
	class hkRigidBody * pHavokRigidBody );

void HavokReleaseInvisibleRigidBody( 
	GAME * pGame, 
	hkWorld * pWorld,
	hkRigidBody * pBody );

void HavokRigidBodyRemoveFromWorld( 
	struct GAME * pGame, 
	class hkWorld * pWorld,
	class hkRigidBody * pHavokRigidBody );

void HavokRigidBodyAddToWorld( 
	struct GAME * pGame, 
	class hkWorld * pWorld,
	class hkRigidBody * pHavokRigidBody );

void HavokRigidBodySetVelocity( 
	class hkRigidBody * pRigidBody, 
	VECTOR * pVelocity);

void HavokRigidBodyGetVelocity( 
	hkRigidBody * pRigidBody,
	VECTOR & vVelocity);

void HavokRigidBodyGetPosition( 
	class hkRigidBody * pRigidBody, 
	VECTOR * pPosition);

void HavokRigidBodyWarp(
	struct GAME * pGame,
	class hkWorld * pWorldNew,
	class hkRigidBody * pRigidBody,
	const VECTOR & vPosition, 
	const VECTOR & vFaceDirection, 
	const VECTOR & vUpDirection );

void HavokRigidBodyGetMatrix( 
	class hkRigidBody * pRigidBody, 
	struct MATRIX & mWorld,
	const VECTOR & vOffset );

void HavokRigidBodyApplyImpact( 
	struct RAND& rand,
	class hkRigidBody * pRigidBody, 
	const HAVOK_IMPACT & tHavokImpact );

void HavokAddFloor(
	struct GAME * pGame,
	class hkWorld * pWorld );

void HavokRemoveFloor(
	struct GAME * pGame,
	class hkWorld * pWorld);

typedef void FP_HAVOKHIT( struct GAME * pGame, struct UNIT * pUnit, const VECTOR & vNormal, void * data);

void HavokRigidBodySetHitFunc(
	struct GAME * pGame,
	struct UNIT * pUnit,
	class hkRigidBody * pRigidBody, 
	FP_HAVOKHIT pHitFunc,
	void * pData );

VECTOR HavokGetGravity();

void HavokSaveSnapshot(
	class hkWorld * pWorld );

inline void HavokImpactInit(
	struct GAME * pGame,
	HAVOK_IMPACT & tImpact,
	float fForce,
	const VECTOR & vSource,
	const VECTOR * pvDirection )
{
	ZeroMemory( &tImpact, sizeof( HAVOK_IMPACT ) );
	tImpact.dwTick = GameGetTick( pGame );
	tImpact.vSource = vSource;
	tImpact.vDirection = pvDirection ? *pvDirection : VECTOR(0);
	tImpact.fForce = fForce;
}


hkWorld * HavokGetHammerWorld();
int HavokGetHammerFXWorld();

float HavokLineCollideLen( 
	hkWorld * pWorld,
	const VECTOR& vPosition, 
	const VECTOR& vDirection, 
	float fMaxLen, 
	VECTOR* pvNormal,
	int* pCollideKey = NULL);

void HavokLoadShapeForAppearance( 
	int nAppearanceDefId, 
	const char * pszFilename );

struct HAVOK_THREAD_DATA 
{
	struct MEMORYPOOL *		pMemoryPool;
	BYTE *					pStackBuffer;
	class hkThreadMemory*	pThreadMemory;
};

void HavokThreadInit(
	HAVOK_THREAD_DATA & tThreadData );

void HavokThreadClose(
	HAVOK_THREAD_DATA & tThreadData );


