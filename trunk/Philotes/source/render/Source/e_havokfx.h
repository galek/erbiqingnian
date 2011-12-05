//-------------------------------------------------------------------------------------------------
//
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//-------------------------------------------------------------------------------------------------
#ifndef _INCLUDE_E_HAVOKFX
#define _INCLUDE_E_HAVOKFX

#define MAX_NUM_SYSTEMS 4

/*
typedef enum 
{	HAVOKFX_SHAPE_SPHERE,
    HAVOKFX_SHAPE_SPHERE2,
	NUM_HAVOKFX_SHAPES,
}HAVOKFX_SHAPE;
*/

typedef enum{
	HAVOKFX_RIGID_BODY,
	HAVOKFX_PARTICLE,

    NUM_HAVOKFX_PARTICLE_TYPES
} HAVOKFX_PARTICLE_TYPE;

typedef enum SampleDir
{
			FX_BRIDGE_BODY_SAMPLEDIR_X = 0,
			FX_BRIDGE_BODY_SAMPLEDIR_Y = 1,
			FX_BRIDGE_BODY_SAMPLEDIR_Z = 2,			
			FX_BRIDGE_BODY_SAMPLEDIR_AUTO = 3 // min aabb extent used as sample dir
}FX_BRIDGE_BODY_SAMPLEDIR;

typedef enum 
{
    //rigid bodies for attractor systems
	HAVOKFX_PHYSICS_SYSTEM_PARAM_ATTRACTOR_POSITION_AND_FORCE,
	HAVOKFX_PHYSICS_SYSTEM_PARAM_ATTRACTOR_DESTRUCTION_RADIUS_MIN_MAX,
	HAVOKFX_PHYSICS_SYSTEM_PARAM_Z_ACCELERATION, //only the z value is used, negative to fall to ground
	HAVOKFX_PHYSICS_SYSTEM_PARAM_WIND_FORCE,
    HAVOKFX_PHYSICS_SYSTEM_PARAM_LINEAR_AND_ANGULAR_MULTIPLIERS,  //x: angular, y:linear
	HAVOKFX_PHYSICS_SYSTEM_PARAM_CRAWL_OR_FLY,  //x<0?fly:crawl
    HAVOKFX_PHYSICS_SYSTEM_PARAM_LINEAR_VEL_CAP,
	HAVOKFX_PHYSICS_SYSTEM_PARAM_ANGULAR_VEL_CAP,
	HAVOKFX_PHYSICS_SYSTEM_BODY_PARAMS,

	//particle emitter parameters to set through world
    HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_POS = HAVOKFX_PHYSICS_SYSTEM_BODY_PARAMS,
    HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_SIZE,
	HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_MIN,
    HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_VEL_RANGE,
    HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_RADIUS,
	HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_UNIFORM_PARAMS,

    //particle emitter parameters to set for the emitter itself
    HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_RATE = HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_UNIFORM_PARAMS,
    HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_LIFETIME,
    HAVOKFX_PHYSICS_SYSTEM_PARAM_EMITTER_PARTICLE_LIFETIME,
	HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_SPECIFIC_PARAMS, 

	HAVOKFX_PHYSICS_SYSTEM_PARAM_PARTICLE_BEHAVIOR_ANTIBOUNCE = HAVOKFX_PHYSICS_SYSTEM_PARTICLE_EMITTER_SPECIFIC_PARAMS,
   	HAVOKFX_PHYSICS_SYSTEM_PARTICLE_BEHAVIOR_PARAMS,

	NUM_HAVOKFX_PHYSICS_SYSTEM_PARAMS = HAVOKFX_PHYSICS_SYSTEM_PARTICLE_BEHAVIOR_PARAMS
}HAVOKFX_PHYSICS_SYSTEM_PARAM;


class hkFxWorldGpu;

BOOL e_HavokFXIsEnabled();

void e_HavokFXSystemInit();
void e_HavokFXSystemClose();

bool e_HavokAddSystem(int sysid,HAVOKFX_PARTICLE_TYPE type);
void e_HavokFXRemoveSystem(int sysid);


void e_HavokFXInitWorld(
	class hkWorld * pWorld,
	const BOUNDING_BOX & tBoundingBox,
	BOOL bAddFloor,
	float fFloorBottom,int maxSystems=MAX_NUM_SYSTEMS );

void e_HavokFXCloseWorld();

void e_HavokFXStepWorld( float fDelta );

typedef int hkFxRigidBodyHandle;

bool e_HavokFXAddFXRigidBody( 
	const MATRIX & mWorld,
	const VECTOR & vVelocityIn,
	int sysid);

bool e_HavokFXAddH3RigidBody( 
	hkRigidBody * body, 
	BOOL bLandscape,
	FX_BRIDGE_BODY_SAMPLEDIR sampleDir = FX_BRIDGE_BODY_SAMPLEDIR_AUTO);

void e_HavokFXRemoveH3RigidBody(
	hkRigidBody * body,
	BOOL bLandscape );

bool e_HavokFXGetSystemVerticesToSkip ( int sysid, int & nVerticesToSkip);


void e_HavokFXSetSystemParamVector( int sysid,
	HAVOKFX_PHYSICS_SYSTEM_PARAM eParam,
	const VECTOR4 & vValue );

void e_HavokFXSetSystemParamFloat( int sysid,
	HAVOKFX_PHYSICS_SYSTEM_PARAM eParam,
	float fValue );

int e_HavokFXGetSystemParticleCount ( int sysid );

void e_DumpHavokFXReport(int iBody, bool bHavokFx);
#endif
