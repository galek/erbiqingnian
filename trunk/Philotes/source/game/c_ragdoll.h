#pragma once
// c_ragdoll.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "havok.h"
#include <hkserialize\packfile\xml\hkXmlPackfileReader.h>
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


//enables ragdoll penetration utility backport from Havok Complete 4.6
#define ENABLE_HK_DETECT_RAGDOLL_PENETRATION



#define RAGDOLL_FILE_EXTENSION "hkx"

struct RAGDOLL_DEFINITION
{
	hkPackfileReader::AllocatedData* pAllocatedData;

	class hkRagdollInstance		*pRagdollInstance;

	class hkSkeletonMapper		*pSkeletonMapperKeyToRagdoll;
	class hkSkeletonMapper		*pSkeletonMapperRagdollToKey;

	class hkSkeleton			*pSkeletonRagdoll;

	BOOL						bLoaded;

	int							nHavokFXWorld;

	int						nId;
	RAGDOLL_DEFINITION		*pNext;
};

//-------------------------------------------------------------------------------------------------
#define RAGDOLL_FLAG_DISABLED				MAKE_MASK(0) // don't pay attention to the ragdoll - it's turned off
#define RAGDOLL_FLAG_NEEDS_WORLD_TRANSFORM	MAKE_MASK(1)
#define RAGDOLL_FLAG_FROZEN					MAKE_MASK(2) // use the ragdoll position, but it's turned off
#define RAGDOLL_FLAG_DEFINITION_APPLIED		MAKE_MASK(3) 
#define RAGDOLL_FLAG_CONSTRAINTS_REMOVED	MAKE_MASK(4) 
#define RAGDOLL_FLAG_IS_BEING_PULLED		MAKE_MASK(5) 
#define RAGDOLL_FLAG_CAN_REMOVE_FROM_WORLD	MAKE_MASK(6) 
#define RAGDOLL_FLAG_NEEDS_TO_ENABLE		MAKE_MASK(7) 

#define COLFILTER_RAGDOLL_LAYER		1 
#define COLFILTER_RAGDOLL_SYSTEM	1
#define COLFILTER_RAGDOLL_PENETRATION_RECOVERY_LAYER	6
#define COLFILTER_RAGDOLL_PENETRATION_DETECTION_LAYER		7


__declspec(align(16)) struct RAGDOLL
{
	hkQsTransform				mPreviousWorldTransform; // keeping this first helps with alignment - which is required
	DWORD						dwFlags;
	class hkRagdollInstance		*pRagdollInstance;

	//ragdoll interpenetration utility instance
#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
	class hkDetectRagdollPenetration	*pRagdollPenetrationUtilityInstance;
	class hkPhantom						*pRagdollPenetrationUtilityPhantom;
#endif

	class hkRagdollRigidBodyController *pRagdollRigidBodyController;

	BOOL						bJointMotorsOn;
	BOOL						bAddedToFXWorld;

	SIMPLE_DYNAMIC_ARRAY<HAVOK_IMPACT>	pImpacts;

	float						fAverageMass;
	float						fPower;
	class hkDeactivaction *		pDeactivateAction;

	VECTOR						vPullOffset;
	float						fTimeEnabled; // how long has it been enabled?

	int							nDefinitionId;
	int							nId;
	RAGDOLL						*pNext;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL c_RagdollDefinitionNew(
	const char * pszFileName,
	int nAppearanceDefId,
	class hkSkeleton * pSkeletonKey );

BOOL c_RagdollNew(
	GAME * pGame, 
	int nAppearanceDefId,
	int nModelId );

void c_RagdollRemove(
	int nModelId );

void c_RagdollSetPower( 
	int nModelId,
	float fRagdollPower );

void c_RagdollAddImpact ( 
	int nModelId, 
	const HAVOK_IMPACT & tHavokImpactIn,
	float fModelScale,
	float fForceMultiplier );

void c_RagdollApplyAllImpacts ( 
	int nModelId );

void c_RagdollEnable( 
	int nModelId );

void c_RagdollFreeze( 
	int nModelId );

void c_RagdollDisable( 
	int nModelId );

void c_RagdollGetDefAndRagdoll(
	int nAppearanceDefId,
	int nModelId,
	RAGDOLL_DEFINITION ** ppRagdollDef,
	RAGDOLL ** ppRagdoll );

BOOL c_RagdollGetPosition(
	int nModelId,
	VECTOR &vPosition );

void c_RagdollGameInit(
	GAME * pGame );
void c_RagdollGameClose(
	GAME * pGame );

void c_RagdollSystemInit();
void c_RagdollSystemClose();

void c_RagdollGameEnableAll(
	GAME * pGame );

void c_RagdollFallApart(
	int nModelId,
	BOOL removeConstraints,
	BOOL switchConstraintsToBallAndSocket,
	BOOL disableAllCollisions,
	BOOL dampBodies,
	float newBodyAngularDamping,
	BOOL changeCapsulesToBoxes);

void c_RagdollDriveToPose ( 
	RAGDOLL * pRagdoll, 
	const class hkQsTransform* desiredPose );

void c_RagdollStartPulling(
	int nRagdoll );

void c_RagdollStopPulling(
	int nRagdoll );


void c_RagdollCheckAndCorrectPenetrations(
	RAGDOLL * pRagdoll );


///////////////////////////////
// RAGDOLL PENETRATION UTILITY
#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
//COPIED FROM hkDetectedRagdollPenetration.h v4.6


/// This interface class defines a single method, castRay(), and it's used by the hkDetectRagdollPenetration object to cast rays
/// into the application's world. 
class hkRagdollRaycastInterface
{
	public:

		virtual ~hkRagdollRaycastInterface() { }

			/// Abstract method, should be implemented by the user in a derived class. Given two points, "from" and "to", specified in
			/// world space, and Aabb phantom (encapsulates ragdoll), it should return whether a ray between those two points intersects the scene or not. If it does (if it returns true), 
			/// the hitPoint and the hitNormal are returned. 
		virtual hkBool castRay (const class hkAabbPhantom* phantomIn,  const hkVector4& fromWS, const hkVector4& toWS, hkVector4& hitPointWS, hkVector4& hitNormalWS ) = 0;

};

/// hkDetectRagdollPenetration class detects penetration of ragdoll with landscape. The method takes as input actual hkPose and performs
/// raycasting of all bones with landscape. Output is the array of the penetrated bones. Each output for penetrated bone contains the begin and
/// end indexes of the bone, penetration point in world coordinates and the normal of penetrated landscape.
/// Raycasting is defined in hkRagdollRaycastInterface (mainly correct landscape layer); 
/// The class also contains very important static utility method isBonePenetratedOrDescendant for checking particular bone status in input 
/// ragdoll according to results of penetration detection. The method returns three states for the bone: no penetrated, penetrated or descendant 
/// of penetrated bone. 
class hkDetectRagdollPenetration : public hkReferencedObject
{
	public:

		HK_DECLARE_CLASS_ALLOCATOR( HK_MEMORY_CLASS_ANIM_RUNTIME );

		/// This structure is passed on construction of an hkDetectRagdollPenetration, and contains information about
		/// the ragdoll skeleton, the interface to raycast and the AAbb phantom
		struct Setup
		{
			/// Ragdoll skeleton
			const class hkSkeleton* m_ragdollSkeleton;

			/// An interface to ray cast functionality. The implementation should only hit objects that the user wants to
			/// check for collisions (ussually landscape). For example, it may ignore debris objects.
			class hkRagdollRaycastInterface* m_raycastInterface;

			/// pointer to hkAabbPhantom which encapsulates ragdoll 
			class hkAabbPhantom* m_ragdollPhantom;

			/// Constructor. Sets some defaults.
			Setup(){}

		};

		/// This structure is filled by the detection routine, and contains information about the start 
		/// and end point of penetrated bone and about hit point and hit normal
		struct BonePenetration
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_DEMO, BonePenetration);

			/// Index of the bone (from)
			hkInt16 m_boneBeginIndex;

			/// Index of the next bone (to)
			hkInt16 m_boneEndIndex;

			/// Penetration point in world space
			hkVector4 m_penetrationPoint;

			/// Penetration normal in world space
			hkVector4 m_penetrationNormal;

			/// Constructor. Sets some defaults.
			BonePenetration();

			/// Destructor. Requested for hkObjectArray
			~BonePenetration();			
					
		};

		/// Penetration Status for particular bone return by static method isInPenetrationOrDescendant(bone, boneArray)
		enum BonePenetrationStatus
		{
			HK_NOP = 0,	// No penetration
			HK_YESP = 1, // Yes, bone penetrates (penetration point lies on this bone)
			HK_YESP_DESCENDANT = 2,	// Yes, bone is descendant of penetrated bones
			HK_YESP_LEAF = 3 //CC@HAVOK. for leaf bones that can't be checked exactly but assumed to be penetrating.
		};

		/// Output structure holds array of BonePenetration objects
		struct Output
		{
			hkObjectArray<BonePenetration> m_penetrations;
			
		};
		
		/// Constructor input is setup structure. 
		hkDetectRagdollPenetration ( const Setup& setup );


		/// Function detects penetrated bones stored in output array stucture, 
		hkBool HK_CALL detectPenetration( const class hkPose& ragdollPoseIn, Output& output );
				
		/// Set m_poseReset to ensure reset of bonesStates after ragdoll keyframed state  
		void resetBoneStates();

		/// Get array of bones states (not penetrated, penetrated or descendant)
		const hkArray<BonePenetrationStatus>& getBoneStatusArray() {return m_boneStates;}


		//CC@HAVOK MOD
		//for tracking leaf bones which were disabled. they need to remain disabled.
		hkArray<hkInt32> m_leafBonesMarkedAsDisabled;

		//cache of which bone indices are leaf bones.
		hkArray<hkInt32> m_leafBones;

	private:

        Setup m_setup;

		hkInt32 m_numBones;

		/// array with bone states (not penetrated, penetrated or descendant)
		hkArray<BonePenetrationStatus> m_boneStates;
		
		/// No penetration on all bones?
		hkBool m_ragdollIsOut;
		
        		
};

#endif //ENABLE_HK_DETECT_RAGDOLL_PENETRATION


