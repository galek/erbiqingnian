//----------------------------------------------------------------------------
// c_ragdoll.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "appcommon.h"
#ifdef HAVOK_ENABLED
#include "prime.h"
#include "globalindex.h"
#include "boundingbox.h"
#include "level.h"
#include "c_ragdoll.h"
#include "c_appearance.h"
#include "excel.h"
#include "c_sound.h"
#include "room.h"
#include "game.h"
#include "picker.h"
#include "config.h"
#include "performance.h"
#include "e_model.h"
#include "e_havokfx.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static CHash<RAGDOLL_DEFINITION> sgpRagdollDefinitons;
static CHashAligned<RAGDOLL,16>	 sgpRagdolls;
RAND							 sgRagdollsRand;


//ray cast for ragdoll penetration utility
#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
class RAGDOLLRAYCAST : public hkRagdollRaycastInterface
{
public:
	virtual hkBool castRay (const class hkAabbPhantom* phantomIn,  const hkVector4& fromWS, const hkVector4& toWS, hkVector4& hitPointWS, hkVector4& hitNormalWS );
};

hkBool RAGDOLLRAYCAST::castRay(const hkAabbPhantom *phantomIn, const hkVector4 &fromWS, const hkVector4 &toWS, hkVector4 &hitPointWS, hkVector4 &hitNormalWS)
{

	//vdb debug
	//HK_DISPLAY_LINE(fromWS,toWS,hkColor::YELLOWGREEN);


	hkWorldRayCastInput raycastIn;
	hkClosestRayHitCollector rayCollector;
	rayCollector.reset();
	
	raycastIn.m_from = fromWS; 
	raycastIn.m_to = toWS;
	raycastIn.m_filterInfo
		= hkGroupFilter::calcFilterInfo( COLFILTER_RAGDOLL_LAYER );
	//raycastIn.m_enableShapeCollectionFilter = true;

	// do raycast in Aabb phantom
	phantomIn->castRay( raycastIn, rayCollector );			
		
	const hkBool didHit = rayCollector.hasHit();	
	if (didHit)
	{
		const hkWorldRayCastOutput& raycastOut = rayCollector.getHit();				
		// calculate hitPoint in world space by interpolation
		hitPointWS.setInterpolate4(raycastIn.m_from, raycastIn.m_to, raycastOut.m_hitFraction);			
		// set normal
		hitNormalWS = raycastOut.m_normal;

		return true;
	}
	else
	{
		return false;
	}



}

///instance of the raycast impl.
static RAGDOLLRAYCAST *sgpRagdollUtilityRaycast = 0;
#endif //ENABLE_HK_DETECT_RAGDOLL_PENETRATION


#define INITIAL_RAGDOLL_POWER 50.0f

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
extern BOOL sgbHavokOpen;
static BOOL sgbRagdollGameOpen = FALSE;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct RAGDOLL_FILE_CALLBACK_DATA
{
	int nDefinitionId;
};

// Very simple to provide some other deactivation based only on root and two connected bodies
// If the movement of each body over m_numSamplesBeforeRefresh frames is less than the sum movement 
// tolerance then deactivate
class hkDeactivaction: public hkUnaryAction, public hkEntityActivationListener
{
public:
	hkDeactivaction( hkEntity* theRootBody, int theModelId );
	~hkDeactivaction();

	/// The applyAction() method does the actual work of the action, and is called at every simulation step.
	virtual void applyAction( const hkStepInfo& stepInfo );

	// Entity activation stuff -------------

	/// Called when an entity is deactivated.
	virtual void entityDeactivatedCallback(hkEntity* entity);

	/// Called when an entity is activated.
	virtual void entityActivatedCallback(hkEntity* entity){}

	virtual hkAction* clone( const hkArray<hkEntity*>& newEntities, const hkArray<hkPhantom*>& newPhantoms ) const
	{
		return HK_NULL;
	}


private:
	hkRigidBody* m_otherBodyA;
	hkRigidBody* m_otherBodyB;
	int m_sampleCounter;
	int m_numSamplesBeforeRefresh;

	hkVector4 m_samplePosition;
	hkVector4 m_samplePositionA;
	hkVector4 m_samplePositionB;

	hkReal m_sumMovementToleranceSquared; //Default .0025, tolerance of 5 cms
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

hkDeactivaction::hkDeactivaction( hkEntity* theRootBody, int theModelId ): hkUnaryAction( theRootBody, theModelId ), 
m_otherBodyA( HK_NULL ), m_otherBodyB( HK_NULL ),
m_sampleCounter(0), m_numSamplesBeforeRefresh(30),
m_sumMovementToleranceSquared( .0025f )
{
	hkRigidBody* theRootRigidBody = static_cast<hkRigidBody*>( theRootBody );
	theRootBody->addEntityActivationListener( this );
	for ( int constraintIt = 0; constraintIt < theRootRigidBody->getNumConstraints(); constraintIt++ )
	{
		hkConstraintInstance* theConstraint = theRootRigidBody->getConstraint( constraintIt);
		if ( theConstraint->getData()->getType() != hkConstraintData::CONSTRAINT_TYPE_CONTACT)
		{
			hkRigidBody* attachedBody = static_cast<hkRigidBody*>( theConstraint->getEntityA() );
			if ( theConstraint->getEntityB() != theRootRigidBody )
			{
				attachedBody = static_cast<hkRigidBody*>( theConstraint->getEntityB() );
			}
			if ( m_otherBodyA == HK_NULL )
			{
				m_otherBodyA = attachedBody;
			}
			else if ( m_otherBodyB == HK_NULL )
			{
				m_otherBodyB = attachedBody;
			}
		} 
		if (m_otherBodyA !=HK_NULL && m_otherBodyB!=HK_NULL )
		{
			break;
		}
	}

	m_samplePosition = theRootRigidBody->getPosition();
	if( m_otherBodyA )
	{
		m_otherBodyA->addReference();
		m_samplePositionA = m_otherBodyA->getPosition();
	}
	if( m_otherBodyB )
	{
		m_otherBodyB->addReference();
		m_samplePositionB = m_otherBodyB->getPosition();
	}

}

hkDeactivaction::~hkDeactivaction()
{
	if (m_otherBodyA)
	{
		m_otherBodyA->removeReference();
		m_otherBodyA = HK_NULL;
	}
	if (m_otherBodyB)
	{
		m_otherBodyB->removeReference();
		m_otherBodyB = HK_NULL;
	}
}

/*virtual*/ void hkDeactivaction::applyAction( const hkStepInfo& stepInfo )
{
	if ( m_sampleCounter == m_numSamplesBeforeRefresh )
	{
		hkBool doBreak = false;

		hkRigidBody* theBody = static_cast<hkRigidBody*>( m_entity );
		hkVector4 shift;
		{
			shift.setSub4( theBody->getPosition(), m_samplePosition);
			if (shift.lengthSquared3() < m_sumMovementToleranceSquared )
			{
				doBreak = true;
			}
			m_samplePosition = theBody->getPosition();
		}
		if( m_otherBodyA )
		{
			shift.setSub4( m_otherBodyA->getPosition(), m_samplePosition);
			if (shift.lengthSquared3() < m_sumMovementToleranceSquared )
			{
				doBreak = true;
			}
			m_samplePositionA = m_otherBodyA->getPosition();
		}
		if( m_otherBodyB )
		{
			shift.setSub4( m_otherBodyB->getPosition(), m_samplePosition);
			if (shift.lengthSquared3() < m_sumMovementToleranceSquared )
			{
				doBreak = true;
			}
			m_samplePositionB = m_otherBodyB->getPosition();
		}

		if ( doBreak )
		{
   			RAGDOLL* pRagdoll = HashGet(sgpRagdolls, getUserData() );
			if (!pRagdoll)
			{
				return;
			}
			pRagdoll->dwFlags |= RAGDOLL_FLAG_CAN_REMOVE_FROM_WORLD;
		}
		m_sampleCounter = 0;
	}
	else
	{
		m_sampleCounter++;
	}
}

void hkDeactivaction::entityDeactivatedCallback( hkEntity* entity )
{
	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, getUserData() );
	if (!pRagdoll)
	{
		return;
	}

	pRagdoll->dwFlags |= RAGDOLL_FLAG_CAN_REMOVE_FROM_WORLD;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sRagdollLoadedCallback( 
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	if (data.m_IOSize == 0) 
	{
		return S_FALSE;
	}

	RAGDOLL_FILE_CALLBACK_DATA * pCallbackData = (RAGDOLL_FILE_CALLBACK_DATA *) spec->callbackData;
	ASSERT_RETFAIL( pCallbackData );

	hkSkeleton * pSkeletonKey = c_AppearanceDefinitionGetSkeletonHavok( pCallbackData->nDefinitionId );
	ASSERT_RETFAIL( pSkeletonKey );

	RAGDOLL_DEFINITION* pRagdollDef = HashGet(sgpRagdollDefinitons, pCallbackData->nDefinitionId);

	if ( ! spec->buffer )
		return S_FALSE;

	if ( ! pRagdollDef )
		return S_FALSE;

	hkPhysicsData* physicsData = NULL;
	{
		hkMemoryStreamReader tStreamReader( spec->buffer, data.m_IOSize, hkMemoryStreamReader::MEMORY_INPLACE );

		pRagdollDef->pAllocatedData = HK_NULL;

		physicsData = hkHavokSnapshot::load( &tStreamReader, &pRagdollDef->pAllocatedData );
	}

	// figure out what physics system has the ragdoll
	int nPhysicsSystemCount = physicsData->getPhysicsSystems().getSize();
	int nPhysicsSytemToUse = INVALID_ID;
	for ( int i = 0; i < nPhysicsSystemCount; i++ )
	{
		hkPhysicsSystem* pSystemCurrent = physicsData->getPhysicsSystems()[i];
		if ( pSystemCurrent->getConstraints().getSize() > 0 )
		{
			nPhysicsSytemToUse = i;
			break;
		}
	}

	ASSERT_RETFAIL( nPhysicsSytemToUse != INVALID_ID );
	hkPhysicsSystem* origSystem = physicsData->getPhysicsSystems()[ nPhysicsSytemToUse ];

	hkArray<hkRigidBody*> rigidBodies; 
	rigidBodies = origSystem->getRigidBodies();

	for ( int j = rigidBodies.getSize() - 1; j >= 0; j-- )
	{
		if ( rigidBodies[ j ]->getMotionType() == hkMotion::MOTION_FIXED )
		{
			rigidBodies.removeAt( j );
		}
		//@@CB thin box motion seems to be causing trouble - HVK-3176 (hk400)?
		else if ( rigidBodies[ j ]->getMotionType() == hkMotion::MOTION_THIN_BOX_INERTIA )
		{
			rigidBodies[ j ]->setMotionType( hkMotion::MOTION_BOX_INERTIA );
		}
	}

	for ( int j = rigidBodies.getSize() - 1; j >= 0; j-- )
	{
		rigidBodies[ j ]->setQualityType(HK_COLLIDABLE_QUALITY_MOVING);
	}

	hkArray<hkConstraintInstance*> newConstraints;
	{
		const int numConstraints = origSystem->getConstraints().getSize();
		hkPositionConstraintMotor* defaultMotor = new hkPositionConstraintMotor( );
		defaultMotor->m_tau = 0.8f;
		defaultMotor->m_maxForce = 10.0f;
		defaultMotor->m_proportionalRecoveryVelocity = 5.0f;
		defaultMotor->m_constantRecoveryVelocity = 0.2f;
		for (int cit=0; cit< numConstraints; cit++)
		{
			hkConstraintInstance* powered = hkConstraintUtils::convertToPowered(origSystem->getConstraints()[cit], defaultMotor);
			newConstraints.pushBack(powered);
		}
		// Replace old constraints and add new ones
		{
			for (int cit=0; cit<numConstraints;cit++)
			{
				origSystem->removeConstraint(0);
			}
			for (int cit2=0; cit2<numConstraints;cit2++)
			{
				origSystem->addConstraint(newConstraints[cit2]);
				newConstraints[cit2]->removeReference();
			}
		}
		defaultMotor->removeReference();
	}

	hkRagdollUtils::reorderAndAlignForRagdoll( rigidBodies, newConstraints );

	pRagdollDef->pSkeletonRagdoll = hkRagdollUtils::constructSkeletonForRagdoll( rigidBodies, newConstraints );

	//if ( pRagdollDef->pSkeletonRagdoll )
	//{
	//	int nBoneCount = pRagdollDef->pSkeletonRagdoll->m_numBones
	//	for ( int i = 0; i < nBoneCount; i++ )
	//	{
	//		hkQsTransform * pTransform = &pRagdollDef->pSkeletonRagdoll->m_referencePose[ i ];
	//		if ( ! pTransform->getRotation().isOk() )
	//		{
	//			pTransform->setRotation( hkQuaternion::getIdentity() );
	//		}
	//	}
	//}

	pRagdollDef->pRagdollInstance = new hkRagdollInstance( rigidBodies, newConstraints, pRagdollDef->pSkeletonRagdoll );

	hkSkeletonUtils::lockTranslations( *pSkeletonKey, false );

	hkSkeletonMapperData tMapperDataKeyToRagdoll;
	hkSkeletonMapperData tMapperDataRagdollToKey;
	{
		hkSkeletonMapperUtils::Params tMapperParams;

		tMapperParams.m_skeletonA = pSkeletonKey;
		tMapperParams.m_skeletonB = pRagdollDef->pSkeletonRagdoll;
		tMapperParams.m_compareNames = NULL;
		tMapperParams.m_positionMatchTolerance = 0.1f;

		hkSkeletonUtils::lockTranslations( *pSkeletonKey, true );

		// Explicit mappings
		// Re-parent the forearms twists to the fore arm
		//{
		//	hkSkeletonMapperUtils::UserMapping mapping;
		//	mapping.m_boneIn = "L_ForeArm";
		//	mapping.m_boneOut = "L_ForeTwist";
		//	tMapperParams.m_userMappingsBtoA.pushBack(mapping);
		//	mapping.m_boneIn = "R_ForeArm";
		//	mapping.m_boneOut = "R_ForeTwist";
		//	tMapperParams.m_userMappingsBtoA.pushBack(mapping);
		//}

		tMapperParams.m_autodetectChains = true;

		hkSkeletonMapperUtils::createMapping( tMapperParams, 
			tMapperDataKeyToRagdoll, 
			tMapperDataRagdollToKey );

	}

	pRagdollDef->pSkeletonMapperKeyToRagdoll = new hkSkeletonMapper( tMapperDataKeyToRagdoll );
	pRagdollDef->pSkeletonMapperRagdollToKey = new hkSkeletonMapper( tMapperDataRagdollToKey );


	const int numSimpleMappings = pRagdollDef->pSkeletonMapperRagdollToKey->m_mapping.m_simpleMappings.getSize();
	const int numChainMappings = pRagdollDef->pSkeletonMapperRagdollToKey->m_mapping.m_chainMappings.getSize();
	ASSERT_RETFAIL( numSimpleMappings || numChainMappings );
	for ( int i = 0; i <numSimpleMappings; i++ )
	{
		const hkSkeletonMapperData::SimpleMapping& simpleMapping = pRagdollDef->pSkeletonMapperRagdollToKey->m_mapping.m_simpleMappings[ i ];

		if ( simpleMapping.m_boneA == 0 )
		{
			pSkeletonKey->m_bones[ simpleMapping.m_boneB ]->m_lockTranslation = false;
			break;
		}
	}

	{
		const hkArray<hkConstraintInstance*> & constraints = pRagdollDef->pRagdollInstance->getConstraintArray();
		hkInertiaTensorComputer::optimizeInertiasOfConstraintTree( constraints.begin(), constraints.getSize(),  pRagdollDef->pRagdollInstance->getRigidBodyOfBone( 0 ) );
	}

	pRagdollDef->bLoaded = TRUE;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_RagdollDefinitionNew(
	const char * pszFileName,
	int nAppearanceDefId,
	hkSkeleton * pSkeletonKey )
{
	//return FALSE;
	RAGDOLL_DEFINITION* pRagdollDef = HashAdd(sgpRagdollDefinitons, nAppearanceDefId);

	pRagdollDef->bLoaded = FALSE;
	pRagdollDef->nHavokFXWorld = INVALID_ID;

	// Shouldn't be checking on disk      -rli
/*	if (!FileExists(pszFileName))
	{
		char szMessage[ 2048 ];
		PStrPrintf( szMessage, 2048, "Ragdoll file '%s' does not exist", pszFileName );
		ASSERTX_RETFALSE( 0, szMessage );
	}*/

	RAGDOLL_FILE_CALLBACK_DATA * pCallbackData = (RAGDOLL_FILE_CALLBACK_DATA *) MALLOCZ( NULL, sizeof( RAGDOLL_FILE_CALLBACK_DATA ) );
	pCallbackData->nDefinitionId = nAppearanceDefId;

	DECLARE_LOAD_SPEC(spec, pszFileName);
	spec.flags = PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.priority = ASYNC_PRIORITY_RAGDOLLS;
	spec.fpLoadingThreadCallback = sRagdollLoadedCallback;
	spec.callbackData = pCallbackData;
	PakFileLoad(spec);

	return TRUE;
}


class CSoundCollisionListener : public hkCollisionListener
{
public:

	/*
	* Construction and Destruction
	*/

	CSoundCollisionListener( ):
			m_newContactDistanceThreshold( 2.0f ),
			m_crashTolerance( 4.0f ),
			m_bounceTolerance( 8.0f ),
			m_normalSeparationVelocity( 0.01f ),
			m_slidingThreshold( 0.5f ),
			m_rollingContactMaxVelocity( 0.1f ),
			m_offNormalCOMRelativeVelocityThreshold( 0.5f ),
			m_playSound(false),
			m_playBounce(false)
	{
	    nGoreId = GlobalIndexGet( GI_SOUND_BODY_CRASH );
	    nGoreId2 = GlobalIndexGet( GI_SOUND_BODY_BOUNCE );
	}

	/*
	* Members from base class hkCollisionListener which must be implemented:
	*/

	// Called after a contact point was added 
	void contactPointAddedCallback(	hkContactPointAddedEvent& event )
	{
		// If the relative velocity of the objects projected onto the contact normal is greater than m_crashTolerance
		// it could be a bounce or a crash, we check if the new point is sufficiently distant from other existing
		// contacts and if so we register a bounce / crash
		if ( abs(event.m_projectedVelocity) > m_crashTolerance )
		{
			if (event.m_type == hkContactPointAddedEvent::TYPE_MANIFOLD)
			{
				//trace("Impact collision at %.2f\n", event.m_projectedVelocity);
				m_playSound = true;
				hkArray<hkContactPointId> pointIds;
				event.m_internalContactMgr.getAllContactPointIds( pointIds );
				hkBool printCrash = true;
				hkManifoldPointAddedEvent hkManifoldEvent = event.asManifoldEvent();
				for( int i=0; i < pointIds.getSize(); i++ )
				{
					if ( pointIds[i] != hkManifoldEvent.m_contactPointId )
					{
						const hkVector4& contactPos = event.m_contactPoint->getPosition();
						const hkVector4& otherContactPos = event.m_internalContactMgr.getContactPoint( pointIds[i] )->getPosition();
						hkVector4 diff;
						diff.setSub4( contactPos, otherContactPos );
						if ( diff.lengthSquared3() < m_newContactDistanceThreshold )
						{
							m_playSound = false;
							break;
						}
					}
				}
			}
		}

		if (m_playSound)
		{
			if ( abs(event.m_projectedVelocity) > m_bounceTolerance )
			{
				m_playBounce = true;
			}
		}

		if ( m_playSound )
		{
			m_playSound = false;
			VECTOR pos;
			hkVector4 position = event.m_contactPoint->getPosition();
			pos.fX = position(0);
			pos.fY = position(1);
			pos.fZ = position(2);

			if ( m_playBounce )
			{
				CLT_VERSION_ONLY( c_SoundPlay( nGoreId2, &pos ) );
			}
			else
			{
				CLT_VERSION_ONLY( c_SoundPlay( nGoreId, &pos ) );
			}
		}
	}

	void contactPointConfirmedCallback( hkContactPointConfirmedEvent& event) {};

	// Called before a contact point gets removed. We do not implement this for this demo.
	void contactPointRemovedCallback( hkContactPointRemovedEvent& event )
	{ }

	// Called just before the collisionResult is passed to the constraint system (solved).
	void contactProcessCallback( hkContactProcessEvent& event )
	{
		//hkRigidBody* rbA = static_cast<hkRigidBody*> (event.m_collidableA.getOwner());
		//hkRigidBody* rbB = static_cast<hkRigidBody*> (event.m_collidableB.getOwner());

		//hkProcessCollisionOutput& result = event.m_collisionResult;
		//for (int i = 0; i < HK_MAX_CONTACT_POINT; i++ )
		//{
			// const hkVector4& start = result.m_contactPoints[i].m_contact.getPosition();
			// hkVector4 normal = result.m_contactPoints[i].m_contact.getNormal();

			// // Get (relative) velocity of contact point. It's arbitrary whether we choose A-B or B-A
			// hkVector4 velPointOnA, velPointOnB, worldRelativeVelocity;
			// rbA->getPointVelocity(start, velPointOnA);
			// rbB->getPointVelocity(start, velPointOnB);
			// worldRelativeVelocity.setSub4(velPointOnB, velPointOnA);

			// // Get velocity in normal direction
			// hkReal velocityInNormalDir = worldRelativeVelocity.dot3(normal);
			// hkReal velocityOffNormal = 0.0f;

			// // And velocity vector not in normal: velocity - (normal * velProjectedOnNormal)
			// {
				//  hkVector4 offNormalVelocityVector;
				//  offNormalVelocityVector.setMul4(velocityInNormalDir, normal);
				//  offNormalVelocityVector.setSub4(worldRelativeVelocity, offNormalVelocityVector);
				//  velocityOffNormal = offNormalVelocityVector.length3();
			// }

			// // If the bodies have a minimal separating velocity (along the contact normal) 
			// // and they have a sufficient velocity off normal then we identify a sliding contact
			// if (	(hkMath::fabs(velocityInNormalDir) < m_normalSeparationVelocity ) 
				//  && (velocityOffNormal > m_slidingThreshold) ) 
			// {
				//  //hkcout << velocityOffNormal << " SLIDING\n";
				//  VECTOR pos;
				//  //VECTOR vel;
				//  hkVector4 position = result.m_currentContactPoint->m_contact.getPosition();
				//  pos.fX = position(0);
				//  pos.fY = position(1);
				//  pos.fZ = position(2);
				//  /*
				//  hkVector4 velocity = event.m_contactPoint->getPosition();
				//  vel.fX = velocity(0);
				//  vel.fY = velocity(1);
				//  vel.fZ = velocity(2);
				//  // */
				//  //trace("Slide\n");
				//  //c_SoundPlay( nGoreId, &pos );
			// }
			// else
			// {
				//  // Test for rolling, first see if the relative velocity of the bodies at the contact point is below
				//  // a given threshold: for rolling contacts the effect of the angular velocity of the body and the linear
				//  // velocity tend to cancel each other out giving the contact point a zero world velocity.
				//  // Once this has been satisfied we look to see if the component of the relative velocity of the bodies' 
				//  // centers of mass not in the direction of the contact normal is above another threshold and if so
				//  // we recognise a rolling contact. The defaults for these values are arbitrary and should be tweaked to 
				//  // achieve a satisfactory result for each use case.
				//  if (worldRelativeVelocity.length3() < m_rollingContactMaxVelocity)
				//  {
				//	  hkVector4 relativeVelocityOfCOMs;
				//	  relativeVelocityOfCOMs.setSub4(rbB->getLinearVelocity(), rbA->getLinearVelocity());
				//	  hkReal relativeCOMVelocityInNormalDir = relativeVelocityOfCOMs.dot3(normal);

				//	  // Find component of centres of mass relative velocity not in direction of contact normal
				//	  hkVector4 velCOMNotInNormal;
				//	  {
				//		  velCOMNotInNormal.setMul4(relativeCOMVelocityInNormalDir, normal);
				//		  velCOMNotInNormal.setSub4(relativeVelocityOfCOMs, velCOMNotInNormal);
				//	  }

				//	  if (velCOMNotInNormal.length3() > m_offNormalCOMRelativeVelocityThreshold)
				//	  {
				//		  //hkcout <<  velCOMNotInNormal.length3() <<" ROLLING\n";
				//		  VECTOR pos;
				//		  //VECTOR vel;
				//		  hkVector4 position = result.m_currentContactPoint->m_contact.getPosition();
				//		  pos.fX = position(0);
				//		  pos.fY = position(1);
				//		  pos.fZ = position(2);
				//		  /*
				//		  hkVector4 velocity = event.m_contactPoint->getPosition();
				//		  vel.fX = velocity(0);
				//		  vel.fY = velocity(1);
				//		  vel.fZ = velocity(2);
				//		  // */
				//		  //trace("Roll\n");
				//		  //c_SoundPlay( nGoreId, &pos );
				//	  }
				//  }
			// }
		//}
	}

private:

	//
	// Add Contact Values
	//

	// If new contacts are less than this distance from an existing contact then they are ignored 
	// and do not generate a crash/ bounce.
	hkReal m_newContactDistanceThreshold;

	// These two values look at the projected velocity of the bodies' relative velocity onto the contact normal 
	// If it is above m_crashTolerance and passes m_newContactDistanceThreshold then it either generates
	// A crash or a bounce depending on whether or not it exceeds m_bounceTolerance (bounce > crash)
	hkReal m_crashTolerance;
	hkReal m_bounceTolerance;

	//
	// Process Contact Values
	//

	// When processing contacts if the bodies are not separating ( < m_normalSeparationVelocity is the relative 
	// velocity along the contact normal ) and the contact's velocity not in the direction of the contact normal
	// is greater than m_slidingThreshold we recognise a sliding contact
	hkReal m_normalSeparationVelocity;
	hkReal m_slidingThreshold;

	// If the relative velocity of the bodies at the contact point is below m_rollingContactMaxVelocity 
	// and the component of the relative velocity of the bodies' centers of mass not in the direction of the 
	// contact normal is above m_offNormalCOMRelativeVelocityThreshold then we identify a Rolling contact.
	hkReal m_rollingContactMaxVelocity;
	hkReal m_offNormalCOMRelativeVelocityThreshold;

	hkBool m_playSound;
	hkBool m_playBounce;

	// The ID of the temporary 'gore' Sound
	int nGoreId;
	int nGoreId2;
};

//*************************************************************************************************
//  ragdoll - these share ids with models
//*************************************************************************************************
static void sApplyDefinitionToRagdoll(
	RAGDOLL * pRagdoll,
	RAGDOLL_DEFINITION * pRagdollDef )
{
	// copy the ragdoll instance
	pRagdoll->pRagdollInstance = pRagdollDef->pRagdollInstance->clone();
	pRagdoll->pRagdollRigidBodyController = new hkRagdollRigidBodyController( pRagdoll->pRagdollInstance );

	// dial this down to make the ragdoll flop around more
	const float scaleFactor = 1.0f;
	{
		// tune down the pose matching parameters. We override for the root
		// anyway, using hkKeyFrameUtility.
		hkKeyFrameHierarchyUtility::ControlData& cd0 = 
			pRagdoll->pRagdollRigidBodyController->m_controlDataPalette[0];
		{
			cd0.m_accelerationGain = 0.4f * scaleFactor;
			cd0.m_velocityGain = 0.2f * scaleFactor;
			cd0.m_velocityDamping = 0.05f * scaleFactor;
		}
	}

	pRagdoll->fAverageMass = 0.0f;
	int nBoneCount = pRagdollDef->pRagdollInstance->getNumBones();
	for(int i=0; i < nBoneCount; i++)
	{
		hkRigidBody * pRigidBody = pRagdoll->pRagdollInstance->getRigidBodyOfBone( i );
		//if (IS_CLIENT(pGame))
		//{
		//	CSoundCollisionListener * pListener = new CSoundCollisionListener;
		//	pRigidBody->addCollisionListener( pListener );
		//}
		pRagdoll->fAverageMass += pRigidBody->getMass();
	}
	pRagdoll->fAverageMass /= nBoneCount;

	pRagdoll->dwFlags |= RAGDOLL_FLAG_DEFINITION_APPLIED;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_RagdollNew(
	GAME * pGame, 
	int nAppearanceDefId,
	int nModelId )
{
	if (! sgbRagdollGameOpen )
		return FALSE;

	RAGDOLL_DEFINITION* pRagdollDef = HashGet(sgpRagdollDefinitons, nAppearanceDefId);
	if (!pRagdollDef)
	{
		return FALSE;
	}

	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDefId );
	ASSERT_RETFALSE( pAppearanceDef );

	if ( HashGet( sgpRagdolls, nModelId ) )
		return TRUE;

	RAGDOLL* pRagdoll = HashAdd(sgpRagdolls, nModelId);

	if ( pRagdollDef->bLoaded )
	{
		sApplyDefinitionToRagdoll( pRagdoll, pRagdollDef );
	}

	pRagdoll->nDefinitionId = nAppearanceDefId;

	pRagdoll->dwFlags |= RAGDOLL_FLAG_DISABLED;

	ArrayInit(pRagdoll->pImpacts, NULL, 2);

	pRagdoll->bAddedToFXWorld = false;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollRemove(
	 int nModelId )
{
	if (! sgbRagdollGameOpen )
		return;

	if ( sgbHavokOpen == FALSE )
		return;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll)
	{
		return;
	}

	if ( pRagdoll->pRagdollInstance )
	{
		for(int i=0; i < pRagdoll->pRagdollInstance->getNumBones(); i++)
		{
			hkRigidBody * pRigidBody = pRagdoll->pRagdollInstance->getRigidBodyOfBone( i );
			if ( pRigidBody )
			{
				const hkArray<hkCollisionListener*>& pListeners = pRigidBody->getCollisionListeners();
				for ( int i = 0; i < pListeners.getSize(); i++ )
				{
					delete pListeners[ i ];
					pRigidBody->removeCollisionListener( pListeners[ i ] );
				}
				//pGame->pHavokWorld->removeEntity( pRigidBody );
				//pRigidBody->removeReference();
			}
		}

		hkWorld * pWorld = pRagdoll->pRagdollInstance->getWorld();
		if ( pRagdoll->pRagdollInstance->getWorld() )
		{
			if ( pRagdoll->pDeactivateAction )
			{
				pWorld->removeAction( pRagdoll->pDeactivateAction );
				pRagdoll->pRagdollInstance->getRigidBodyOfBone(0)->removeEntityActivationListener(pRagdoll->pDeactivateAction);
				pRagdoll->pDeactivateAction->removeReference();
				pRagdoll->pDeactivateAction = NULL;
			}

			pRagdoll->pRagdollInstance->removeFromWorld();
		}
		hkArray<hkRigidBody*>& rigidBodyArray = pRagdoll->pRagdollInstance->m_rigidBodies;
		for ( int rigidBodyIterator = 0; 
			rigidBodyIterator < rigidBodyArray.getSize(); 
			rigidBodyIterator++ )
		{
			rigidBodyArray[rigidBodyIterator]->clearAndDeallocateProperties();
		}

		if( pRagdoll->pRagdollRigidBodyController )
		{
			delete pRagdoll->pRagdollRigidBodyController;
			pRagdoll->pRagdollRigidBodyController = NULL;
		}
		pRagdoll->pRagdollInstance->removeReference();
		pRagdoll->pRagdollInstance = NULL;
	}
	pRagdoll->pImpacts.Destroy();




	HashRemove(sgpRagdolls, nModelId);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollEnable( 
	int nModelId)
{
	if (! sgbRagdollGameOpen )
		return;

#ifdef _DEBUG
	const GLOBAL_DEFINITION * pGlobalDefinition = DefinitionGetGlobal();	
	ASSERT(pGlobalDefinition);
	if ( pGlobalDefinition->dwFlags & GLOBAL_FLAG_NORAGDOLLS )
		return;
#endif

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll)
	{
		return;
	}

	if ( ( pRagdoll->dwFlags & RAGDOLL_FLAG_DEFINITION_APPLIED ) == 0 )
	{
		int nDefId = c_AppearanceGetDefinition( nModelId );
		RAGDOLL_DEFINITION* pRagdollDef = HashGet(sgpRagdollDefinitons, nDefId );
		if ( pRagdollDef && pRagdollDef->bLoaded )
			sApplyDefinitionToRagdoll( pRagdoll, pRagdollDef );
		else if ( (pRagdoll->dwFlags & (RAGDOLL_FLAG_DISABLED | RAGDOLL_FLAG_FROZEN)) == 0 )
		{
			pRagdoll->dwFlags |= RAGDOLL_FLAG_NEEDS_TO_ENABLE;
			return;
		}
	} else if ( (pRagdoll->dwFlags & (RAGDOLL_FLAG_DISABLED | RAGDOLL_FLAG_FROZEN)) == 0 )
		return;

	BOOL bUseHavokFX = FALSE;
	hkWorld* pWorld = HavokGetWorld( nModelId, &bUseHavokFX );

	if ( pWorld && pRagdoll->pRagdollInstance )
	{
		pRagdoll->pRagdollInstance->addToWorld( pWorld, FALSE );

		//CC@HAVOK added: force ragdoll bodies to belong to ragdoll collision layer
		const hkArray<hkRigidBody*>* bodiesOfRagdoll = &(pRagdoll->pRagdollInstance->getRigidBodyArray());
		int nBodies = bodiesOfRagdoll->getSize();
		for(int x=0; x < nBodies; ++x)
		{
			bodiesOfRagdoll->operator [](x)
				->setCollisionFilterInfo(
					hkGroupFilter::calcFilterInfo(COLFILTER_RAGDOLL_LAYER));

			pWorld->updateCollisionFilterOnEntity(
				bodiesOfRagdoll->operator [](x),
				HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,
				HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS);
		}
		//

		
		
		//create ragdoll penetration instance and phantom (used by utility)
#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
		hkDetectRagdollPenetration::Setup utilitySetup;
		utilitySetup.m_ragdollSkeleton = pRagdoll->pRagdollInstance->getSkeleton();
		utilitySetup.m_raycastInterface = sgpRagdollUtilityRaycast;

		//the utility sets the aabb extents when called, so we can use an empty aabb to initialize the phantom.
		hkAabb dummyAabb = hkAabb();
		dummyAabb.m_max.setZero4();
		dummyAabb.m_min.setZero4();
		pRagdoll->pRagdollPenetrationUtilityPhantom
			= utilitySetup.m_ragdollPhantom
			= new hkAabbPhantom(
				dummyAabb,
				hkGroupFilter::calcFilterInfo( COLFILTER_RAGDOLL_PENETRATION_DETECTION_LAYER ) );

		pWorld->addPhantom(
			pRagdoll->pRagdollPenetrationUtilityPhantom );

		pRagdoll->pRagdollPenetrationUtilityInstance
			= new hkDetectRagdollPenetration(utilitySetup);
#endif


		if ( ! pRagdoll->pDeactivateAction )
		{
			pRagdoll->pDeactivateAction = 
				new hkDeactivaction( pRagdoll->pRagdollInstance->getRigidBodyArray()[0], pRagdoll->nId );
			pWorld->addAction( pRagdoll->pDeactivateAction );
		}
		
		//add the doll to the havokfx world, if it exists
		GLOBAL_DEFINITION * pGlobalDef = DefinitionGetGlobal();
		if ( ( pGlobalDef->dwFlags & GLOBAL_FLAG_HAVOKFX_RAGDOLL_ENABLED ) != 0 )
		{
			if( bUseHavokFX && e_HavokFXIsEnabled() && !pRagdoll->bAddedToFXWorld )
			{
				hkArray<hkRigidBody *> rigidBodies;
				rigidBodies = pRagdoll->pRagdollInstance->getRigidBodyArray();
				int nRigidBodyCount = rigidBodies.getSize();
				//hkFxRigidBodyHandle hRagdoll;
				for ( int i = 0; i < nRigidBodyCount; i++ )
					pRagdoll->bAddedToFXWorld = e_HavokFXAddH3RigidBody( rigidBodies[ i ], false);
			}
		}

		//HavokSaveSnapshot( pWorld );
	}

	pRagdoll->dwFlags &= ~( RAGDOLL_FLAG_DISABLED | RAGDOLL_FLAG_FROZEN | RAGDOLL_FLAG_NEEDS_TO_ENABLE );
	pRagdoll->dwFlags |= RAGDOLL_FLAG_NEEDS_WORLD_TRANSFORM;
	pRagdoll->fTimeEnabled = 0.0f;

	// a little test... it doesn't look good yet... We will set up a data path when it does
	//c_RagdollRemoveConstraints( nModelId );

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollStartPulling(
	int nRagdoll )
{
	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nRagdoll);
	if (!pRagdoll)
	{
		return;
	}

	pRagdoll->dwFlags |= RAGDOLL_FLAG_IS_BEING_PULLED;

	c_RagdollEnable( nRagdoll ); 
	if ( pRagdoll->pRagdollInstance )
		pRagdoll->pRagdollInstance->getRigidBodyOfBone(0)->setMotionType( hkMotion::MOTION_KEYFRAMED );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollStopPulling(
	int nRagdoll )
{
	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nRagdoll);
	if (!pRagdoll)
	{
		return;
	}

	pRagdoll->dwFlags &= ~RAGDOLL_FLAG_IS_BEING_PULLED;
	if ( pRagdoll->pRagdollInstance )
		pRagdoll->pRagdollInstance->getRigidBodyOfBone(0)->setMotionType( hkMotion::MOTION_DYNAMIC );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollDisable( 
	int nModelId )
{
	if (! sgbRagdollGameOpen )
		return;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll)
	{
		return;
	}
	if ( pRagdoll->dwFlags & RAGDOLL_FLAG_DISABLED )
		return;

	pRagdoll->dwFlags |= RAGDOLL_FLAG_DISABLED;
	pRagdoll->dwFlags &= ~RAGDOLL_FLAG_NEEDS_TO_ENABLE;



}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static hkConstraintInstance* sCreateBallAndSocketEquivalent( hkConstraintInstance* originalConstraint )
{
	hkConstraintInstance* newConstraint = NULL;
	hkBallAndSocketConstraintData* theBallAndSocketData = new hkBallAndSocketConstraintData;
	hkRigidBody* bodyA = originalConstraint->getRigidBodyA();
	hkRigidBody* bodyB = originalConstraint->getRigidBodyB();
	hkConstraintData* constraintData = originalConstraint->getData();

	switch (constraintData->getType())
	{
	case hkConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
		{
			hkLimitedHingeConstraintData* oldConstraintData = static_cast<hkLimitedHingeConstraintData*> (constraintData);
			theBallAndSocketData->m_atoms.m_pivots.m_translationA = oldConstraintData->m_atoms.m_transforms.m_transformA.getTranslation();
			theBallAndSocketData->m_atoms.m_pivots.m_translationB = oldConstraintData->m_atoms.m_transforms.m_transformB.getTranslation();
			break;
		}

	case hkConstraintData::CONSTRAINT_TYPE_RAGDOLL:
		{
			hkRagdollConstraintData* oldConstraintData = static_cast<hkRagdollConstraintData*> (constraintData);
			theBallAndSocketData->m_atoms.m_pivots.m_translationA = oldConstraintData->m_atoms.m_transforms.m_transformA.getTranslation();
			theBallAndSocketData->m_atoms.m_pivots.m_translationB = oldConstraintData->m_atoms.m_transforms.m_transformB.getTranslation();
			break;
		}
	default:
		{
			HK_WARN_ALWAYS (0xadda1b34, "Cannot convert constraint to ball and socket constraint.");
		}
	}
	newConstraint = new hkConstraintInstance (bodyA, bodyB, theBallAndSocketData, originalConstraint->getPriority());
	theBallAndSocketData->removeReference();
	return newConstraint;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// This function just uses the AABB, possibly shifted, if the capsule isn't defined around the center
// If capsules are off axis this could look wrong so keep an eye out when using this function for bad boxes
// Warning too....
static void sChangeCapsuleToBox( hkRigidBody* theBody )
{
	// Make sure we have a capsule first of all
	if ( theBody->getCollidable()->getShape()->getType() == HK_SHAPE_CAPSULE )
	{
		const hkCapsuleShape* theCapsule = static_cast<const hkCapsuleShape*>( theBody->getCollidable()->getShape() );

#ifdef _DEBUG
		hkVector4 boxAxis;
		boxAxis.setSub4( theCapsule->getVertex(1), theCapsule->getVertex(0) );
		hkReal cos15 = .26f;
		hkReal cos75 = .966f;
		if ( (cos15 < boxAxis(0) && boxAxis(0) < cos75) ||
			(cos15 < boxAxis(1) && boxAxis(1) < cos75) ||
			(cos15 < boxAxis(2) && boxAxis(2) < cos75))
		{
			HK_ASSERT(0x0, "You're trying to make a box from a capsule that's more than 15 degrees off axis.\n");
		} 
#endif
		
		hkAabb capsuleAabb;
		theCapsule->getAabb( hkTransform::getIdentity(), 0.0f, capsuleAabb );
		hkVector4 halfExtents;
		halfExtents.setSub4( capsuleAabb.m_max, capsuleAabb.m_min);
		halfExtents.mul4(0.5f);
		hkVector4 boxCenter;
		boxCenter.setAdd4( capsuleAabb.m_min, halfExtents );

		hkBoxShape* theBox = new hkBoxShape( halfExtents, 0.02f );
		
		if ( !boxCenter.equals3( hkVector4::getZero() ) )
		{
			hkConvexTranslateShape* theConvexTranslateShape = new hkConvexTranslateShape( theBox, boxCenter );
			theBox->removeReference();
			theBody->setShape( theConvexTranslateShape );
			theConvexTranslateShape->removeReference();
		}
		else
		{
			theBody->setShape( theBox );
			theBox->removeReference();
		}
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollFallApart(
	int nModelId,
	BOOL removeConstraints,
	BOOL switchConstraintsToBallAndSocket,
	BOOL disableAllCollisions,
	BOOL dampBodies,
	float newBodyAngularDamping,
	BOOL changeCapsulesToBoxes)
{
	//return;
	if (! sgbRagdollGameOpen )
		return;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll)
	{
		return;
	}

	if ( ! pRagdoll->pRagdollInstance )
		return;

	hkWorld * pWorld = pRagdoll->pRagdollInstance->getWorld();
	//if ( ! pWorld )
	//	return;

	if ( removeConstraints )
	{
		pRagdoll->dwFlags |= RAGDOLL_FLAG_CONSTRAINTS_REMOVED;
		for (int c=0; c < pRagdoll->pRagdollInstance->m_constraints.getSize(); ++c)
		{
			if ( pWorld	)
				pWorld->removeConstraint( pRagdoll->pRagdollInstance->m_constraints[c] );		
			pRagdoll->pRagdollInstance->m_constraints[c]->removeReference();
		}
		// TYLER WE'RE CHANGING IT FOR THE RAGDOLL SKELETON DEFINITION MAPPER HERE IF YOU WANT TO MAKE IT DATA
		// DRIVEN, DO SO IN THE FUTURE. THE ONE THING YOU MIGHT WANT TO TRACK DOWN IS THAT THIS SKELETON SEEMS TO BE
		// DIFFERENT FROM THE ONES IN THE ACTUAL SKELETON DEFS. I DON'T THINK THE MAPPER NEEDS TO COPY THEM (NOT 100%)
		// SO YOU MIGHT WANT TO TRACK IT DOWN SOMETIME, NOT CRITICAL NOW

		RAGDOLL_DEFINITION* ragdollSkeletonDef = HashGet( sgpRagdollDefinitons, pRagdoll->nDefinitionId );
		hkSkeleton* definitionSkeleton = const_cast<hkSkeleton*>( ragdollSkeletonDef->pSkeletonMapperRagdollToKey->m_mapping.m_skeletonB );
		for (int defBoneIt=0; defBoneIt < definitionSkeleton->m_numBones; defBoneIt++)
		{
			definitionSkeleton->m_bones[defBoneIt]->m_lockTranslation = false;
		}

		pRagdoll->pRagdollInstance->m_constraints.clear();
	} 
	else if ( switchConstraintsToBallAndSocket )
	{
		pRagdoll->dwFlags |= RAGDOLL_FLAG_CONSTRAINTS_REMOVED;
		for (int c=0; c < pRagdoll->pRagdollInstance->m_constraints.getSize(); ++c)
		{
			hkConstraintInstance* originalConstraint = pRagdoll->pRagdollInstance->m_constraints[c];
			hkConstraintInstance* ballSocket = sCreateBallAndSocketEquivalent( originalConstraint );
			
			if ( pWorld )
			{
				pWorld->removeConstraint( originalConstraint );		
				pWorld->addConstraint( ballSocket );
			}
			pRagdoll->pRagdollInstance->m_constraints[c] = ballSocket;
			originalConstraint->removeReference();
		}
	}

	if ( disableAllCollisions )
	{
		hkUint32 newFilterInfo = hkGroupFilter::calcFilterInfo( COLFILTER_RAGDOLL_LAYER, 1337);
		for ( int bodiesIt=0; bodiesIt < pRagdoll->pRagdollInstance->getRigidBodyArray().getSize(); bodiesIt++)
		{
			hkRigidBody* theBody = pRagdoll->pRagdollInstance->getRigidBodyArray()[bodiesIt];
			theBody->setCollisionFilterInfo( newFilterInfo );
			if ( pWorld )
			{
				pWorld->updateCollisionFilterOnEntity( theBody, HK_UPDATE_FILTER_ON_ENTITY_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY, 
					HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS);
			}
		}
	}

	if ( dampBodies==TRUE )
	{
		for ( int bodiesIt=0; bodiesIt < pRagdoll->pRagdollInstance->getRigidBodyArray().getSize(); bodiesIt++)
		{
			hkRigidBody* theBody = pRagdoll->pRagdollInstance->getRigidBodyArray()[bodiesIt];
			theBody->setAngularDamping( newBodyAngularDamping );
		}
	}

	if ( changeCapsulesToBoxes==TRUE)
	{
		for ( int bodiesIt=0; bodiesIt < pRagdoll->pRagdollInstance->getRigidBodyArray().getSize(); bodiesIt++)
		{
			sChangeCapsuleToBox( pRagdoll->pRagdollInstance->getRigidBodyArray()[bodiesIt] );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollFreeze( 
	int nModelId)
{
	if (! sgbRagdollGameOpen )
		return;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if ( !pRagdoll || !pRagdoll->pRagdollInstance )
	{
		return;
	}
	if ( pRagdoll->dwFlags & RAGDOLL_FLAG_FROZEN )
		return;

	pRagdoll->dwFlags |= RAGDOLL_FLAG_FROZEN;
	pRagdoll->dwFlags &= ~(RAGDOLL_FLAG_CAN_REMOVE_FROM_WORLD | RAGDOLL_FLAG_NEEDS_TO_ENABLE);


	//free ragdoll penetration utility resources
#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
	if(pRagdoll->pRagdollPenetrationUtilityPhantom)
	{
		hkWorld * pWorld = pRagdoll->pRagdollPenetrationUtilityPhantom->getWorld();
		if ( pWorld )
		{
			pWorld->removePhantom(
				pRagdoll->pRagdollPenetrationUtilityPhantom );
		}
		pRagdoll->pRagdollPenetrationUtilityPhantom->removeReference();
		pRagdoll->pRagdollPenetrationUtilityPhantom = 0;
	}
	if(pRagdoll->pRagdollPenetrationUtilityInstance)
	{
		pRagdoll->pRagdollPenetrationUtilityInstance->removeReference();
		pRagdoll->pRagdollPenetrationUtilityInstance = 0;
	}
#endif


	hkWorld * pWorld = pRagdoll->pRagdollInstance->getWorld();
	if ( pWorld )
	{
		if ( pRagdoll->pDeactivateAction )
		{
			pWorld->removeAction( pRagdoll->pDeactivateAction );
			pRagdoll->pRagdollInstance->getRigidBodyOfBone(0)->removeEntityActivationListener(pRagdoll->pDeactivateAction);
			pRagdoll->pDeactivateAction->removeReference();
			pRagdoll->pDeactivateAction = NULL;
		}
		pRagdoll->pRagdollInstance->removeFromWorld();
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollSetPower( 
	int nModelId,
	float fRagdollPower )
{
	if (! sgbRagdollGameOpen )
		return;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll)
	{
		return;
	}

	if ( fRagdollPower > 0.0f && (pRagdoll->dwFlags & (RAGDOLL_FLAG_DISABLED | RAGDOLL_FLAG_FROZEN | RAGDOLL_FLAG_CONSTRAINTS_REMOVED)) == 0 )
	{
		if ( ! pRagdoll->bJointMotorsOn )
		{
			ASSERT_RETURN( pRagdoll->pRagdollInstance );

			hkBool startMotors = true;
			{
				for( hkInt32 i = 0; i < pRagdoll->pRagdollInstance->getConstraintArray().getSize(); ++i )
				{
					if( pRagdoll->pRagdollInstance->getConstraintArray()[i]->getInternal() == NULL || pRagdoll->pRagdollInstance->getConstraintArray()[i]->getRuntime() == HK_NULL )
					{
						startMotors = false;
					}
				}
			}

			if ( startMotors )
			{
				hkRagdollPoweredConstraintController::startMotors( pRagdoll->pRagdollInstance );
				pRagdoll->bJointMotorsOn = TRUE;
			}
		}
	}
	else if ( pRagdoll->bJointMotorsOn )
	{
		//pRagdoll->pControler->stopMotors();
		pRagdoll->bJointMotorsOn = FALSE;
	}

	pRagdoll->fPower = fRagdollPower;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

class hkRagdollBlasterTM
{
public:
	// Given an axis for the ragdoll this will attempt to spin all of the bodies about this axis
	//static void spinRagdoll( hkRagdollInstance* theSystem, const hkVector4& centerOfAxis, 
	//	const hkVector4& axis, hkReal spinSpeedRadPS);

	// Given a starting rigidbody this function will apply an impulse to the given body and will then
	// walk the constraints to find attached bodies and then apply an impulse to them too, it will walk a max
	// of numBranches sections fading out the impulse applied by forceGain on each section and taking into account
	// the masses of the bodies so one doesn't get a huge velocity relative to the others
	// Be careful of circles of constraints
	// If you want all forces to be applied from center of starting body as opposed to through the COM of each body
	// use usePointImpulse
	static void pushTree( hkRigidBody* startingBody, hkReal pushForce, const hkVector4& pushDirection, 
		hkReal numBranches, hkReal forceGain, hkBool usePointImpulse = false);

	// Get's the closest body to point in world space, purely based on the body's position
	// Should be wrapped into functions if used a lot but fine for building up tests
	// Direction will be the vector pointing from the pointInWorld to the body
	static hkRigidBody* getClosestBody( hkRagdollInstance* theSystem, const hkVector4& pointInWorld, hkVector4& direction, hkReal& distanceSquared );

private:

	// used by push tree, pushes the bodies in attachedBodies
	static void pushAndGrowTree(
		hkArray<hkRigidBody*> & toBePushedBodies,
		hkArray<hkRigidBody*> & newAttachedBodies,
		hkArray<hkRigidBody*> & parentsOfPushedBodies,
		hkArray<hkRigidBody*> & parentsOfNewAttachments,
		BOOL usePointImpulse,
		hkVector4 & gainedImpulse,
		const hkVector4& pointImpulseOrigin
		);

};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//void hkRagdollBlasterTM::spinRagdoll( hkRagdollInstance* theInstance, const hkVector4& centerOfAxis, 
//									const hkVector4& adjacentAxis, hkReal rotationsPerSecond)
//{
//	hkReal angularSpeed = rotationsPerSecond * hkMath::HK_REAL_PI * 2.0f;
//	hkVector4 angularVelocity( adjacentAxis);
//	angularVelocity.mul4( angularSpeed );
//	hkVector4 hypotenuse;
//
//	for ( int bodyIt=0; bodyIt < theInstance->getRigidBodyArray().getSize(); bodyIt++ )
//	{
//		hkRigidBody* theBody = theInstance->getRigidBodyArray()[bodyIt];
//		hkReal theMass = theBody->getMass();
//		hypotenuse.setSub4( theBody->getCenterOfMassInWorld(), centerOfAxis );
//		hkVector4 perpAxis;
//		perpAxis.setCross( adjacentAxis, hypotenuse ); // So length of perpAxis = radius
//		//length of cross = sin(angle) * |hypotenuse| * |axis| = |O| / |H| * |H| * 1 = |O| = radius
//#ifdef USE_VISUAL_DEBUGGER
//#define DRAW_DEBUG_SPINS
//#endif
//
//#ifdef DRAW_DEBUG_SPINS
//		HK_DISPLAY_STAR( theBody->getCenterOfMassInWorld(), 0.2f, hkColor::GREEN );
//		hkVector4 lineEnd;
//		lineEnd.setAdd4( theBody->getCenterOfMassInWorld(), perpAxis );
//		HK_DISPLAY_LINE( theBody->getCenterOfMassInWorld(), lineEnd, hkColor::YELLOW );
//#endif
//
//		perpAxis.mul4( angularSpeed * theMass ); // length of perpAxis now equals radius * angularSpeed = linearSpeed 
//	
//		//theBody->setLinearVelocity( perpAxis );
//
//		theBody->applyLinearImpulse( perpAxis );
//
//	//	angularVelocity.mul4( theMass );
//	//	theBody->applyAngularImpulse( angularVelocity );
//	//	angularVelocity.mul4( theBody->getMassInv() );
//	}
//
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#ifdef USE_VISUAL_DEBUGGER
#define DRAW_DEBUG_PUSHTREE
#endif

void hkRagdollBlasterTM::pushAndGrowTree(
	 hkArray<hkRigidBody*> & toBePushedBodies,
	 hkArray<hkRigidBody*> & newAttachedBodies,
	 hkArray<hkRigidBody*> & parentsOfPushedBodies,
	 hkArray<hkRigidBody*> & parentsOfNewAttachments,
	 BOOL usePointImpulse,
	 hkVector4 & gainedImpulse,
	 const hkVector4& pointImpulseOrigin
	 )
{
	newAttachedBodies.clear();
	parentsOfNewAttachments.clear();
	for ( int bodyIt=0; bodyIt < toBePushedBodies.getSize(); bodyIt++ )
	{
		hkRigidBody* theBody = toBePushedBodies[bodyIt];
		if ( ! theBody )
			continue;
		for ( int constraintIt = 0; constraintIt < theBody->getNumConstraints(); constraintIt++ )
		{
			hkConstraintInstance* theConstraint = theBody->getConstraint( constraintIt);
			if ( ! theConstraint )
				continue;
			if ( theConstraint->getData()->getType() != hkConstraintData::CONSTRAINT_TYPE_CONTACT)
			{
				hkRigidBody* parentBody = parentsOfPushedBodies[bodyIt];
				hkRigidBody* attachedBody = static_cast<hkRigidBody*>( theConstraint->getEntityA() );
				if ( attachedBody != parentBody && theConstraint->getEntityB() != parentBody )
				{
					if (attachedBody == theBody)
					{
						attachedBody = static_cast<hkRigidBody*>( theConstraint->getEntityB() );
					}
					if ( attachedBody->getMotionType()!= hkMotion::MOTION_FIXED && attachedBody->getMotionType()!= hkMotion::MOTION_KEYFRAMED)
					{
						newAttachedBodies.pushBack( attachedBody );
						parentsOfNewAttachments.pushBack( theBody );
					}
				}
			} 
		}
#ifdef DRAW_DEBUG_PUSHTREE
		const hkCollidable* theCollidable = theBody->getCollidable();
		HK_SET_OBJECT_COLOR( (hkUint32) theCollidable, hkColor::rgbFromFloats( 1.0f * hkMath::pow(0.5f, (hkReal) branchIt ), 0.0f, 0.0f ) );
#endif
		//gainedImpulse.setMul4( theBody->getMass(), gainedVelocity );
		//just to take mass into account we won't try and set an absolute velocity
		if ( gainedImpulse.isOk4() )
		{
			if ( usePointImpulse )
			{
				theBody->applyPointImpulse( gainedImpulse, pointImpulseOrigin );
			} 
			else
			{
				theBody->applyLinearImpulse( gainedImpulse );
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


void hkRagdollBlasterTM::pushTree( hkRigidBody* startingBody, hkReal pushForce, const hkVector4& pushDirection, 
					 hkReal numBranches, hkReal forceGain, hkBool usePointImpulse)
{
	ASSERT_RETURN( numBranches > 0 );
	
	const hkVector4& pointImpulseOrigin = startingBody->getCenterOfMassInWorld();

	hkVector4 gainedImpulse;
	gainedImpulse.setMul4( pushForce, pushDirection );

	hkArray<hkRigidBody*> attachedBodiesA( 8 );	attachedBodiesA.clear();
	hkArray<hkRigidBody*> attachedBodiesB( 8 );	attachedBodiesB.clear();
	hkArray<hkRigidBody*> ignoredBodiesA ( 8 );	ignoredBodiesA.clear();
	hkArray<hkRigidBody*> ignoredBodiesB ( 8 );	ignoredBodiesB.clear();

	attachedBodiesA.pushBack( startingBody );
	ignoredBodiesA.pushBack( HK_NULL );
	BOOL useListA = TRUE;

	for ( int branchIt=0; branchIt < numBranches; branchIt++ )
	{
		if (useListA)
		{
			hkRagdollBlasterTM::pushAndGrowTree( attachedBodiesA, attachedBodiesB, ignoredBodiesA, ignoredBodiesB, usePointImpulse, gainedImpulse, pointImpulseOrigin );
		} 
		else
		{
			hkRagdollBlasterTM::pushAndGrowTree( attachedBodiesB, attachedBodiesA, ignoredBodiesB, ignoredBodiesA, usePointImpulse, gainedImpulse, pointImpulseOrigin );
		}
		useListA = ! useListA;
		gainedImpulse.mul4( forceGain );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
hkRigidBody* hkRagdollBlasterTM::getClosestBody( hkRagdollInstance* theInstance, const hkVector4& pointInWorld, 
								   hkVector4& direction, hkReal& minDistanceSquared )
{
	minDistanceSquared = hkMath::HK_REAL_MAX;
	hkVector4 tempDirection;
	hkReal tempDistSq;
	hkRigidBody* closestBody = HK_NULL;

	for ( int bodyIt=0; bodyIt < theInstance->getRigidBodyArray().getSize(); bodyIt++ )
	{
		hkRigidBody* theBody = theInstance->getRigidBodyArray()[bodyIt];
		tempDirection.setSub4( theBody->getPosition(), pointInWorld );
		tempDistSq = tempDirection.lengthSquared3();
		if ( tempDistSq < minDistanceSquared )
		{
			minDistanceSquared = tempDistSq;
			if ( tempDistSq < 0.001f )
				direction = theBody->getLinearVelocity();
			else
				direction = tempDirection;
			closestBody = theBody;
		}
	}
	direction.fastNormalize3();
	return closestBody;
}
#ifdef USE_VISUAL_DEBUGGER
#define DRAW_IMPACT_DEBUG
#endif
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRagdollApplyImpact ( 
	RAGDOLL * pRagdoll,
	const HAVOK_IMPACT & tHavokImpact )
{
	hkVector4 theSource( tHavokImpact.vSource.fX, tHavokImpact.vSource.fY, tHavokImpact.vSource.fZ );
#ifdef DRAW_IMPACT_DEBUG
	HK_DISPLAY_ARROW( pRagdoll->pRagdollInstance->getRigidBodyArray()[ 0 ]->getPosition(), hkVector4(0.0f, 0.0f, 1.0f), hkColor::RED);
	HK_DISPLAY_STAR( theSource, 0.3f, hkColor::PURPLE );
#endif

	hkVector4 pushDirection;
	hkReal distSq;
	hkRigidBody* bodyToPush = hkRagdollBlasterTM::getClosestBody( pRagdoll->pRagdollInstance, theSource, pushDirection, distSq );
	if(!bodyToPush)
		return;

	hkReal blastDirectionGain = 0.8f;
	hkVector4 spinAxis( 0.0f, 0.0f, 1.0f );

	// Let's put most of the push in the direction of the blast
	hkVector4 vDirection( tHavokImpact.vDirection.fX, tHavokImpact.vDirection.fY, tHavokImpact.vDirection.fZ );

	if ( !vDirection.isOk3() || vDirection.lengthSquared3() < 0.1f )
	{
		hkVector4 vSource( tHavokImpact.vSource.fX, tHavokImpact.vSource.fY, tHavokImpact.vSource.fZ );
		vDirection.setSub4( bodyToPush->getPosition(), vSource );
	}
	vDirection.fastNormalize3();

	if ( vDirection.isOk3() && vDirection.lengthSquared3() > 0.99f )
	{
#ifdef DRAW_IMPACT_DEBUG
		HK_DISPLAY_ARROW( theSource, vDirection, hkColor::PURPLE );
#endif
		hkVector4 theCross;
		theCross.setCross( vDirection, spinAxis );
		if ( theCross.dot3( pushDirection ) > 0 )
		{
			spinAxis.mul4( -1.0f );
		}

		if ( pushDirection.isOk3() && pushDirection.lengthSquared3() > 0.99f )
			pushDirection.setInterpolate4( pushDirection, vDirection, blastDirectionGain );
		else
			pushDirection = vDirection;
		pushDirection.normalize3();
	} else {
		pushDirection.set( 0, 0, -1.0f, 0 );
	}
	//hkRagdollBlasterTM::spinRagdoll( pRagdoll->pRagdollInstance, 
	//	pRagdoll->pRagdollInstance->getRigidBodyArray()[ 0 ]->getPosition()/*bodyToPush->getPosition()*/,
	//	spinAxis , 0.8f);

	hkRagdollBlasterTM::pushTree( bodyToPush, tHavokImpact.fForce * 2.0f, pushDirection, 3, 0.8f );


	//HavokRigidBodyApplyImpact( sgRagdollsRand, pRigidBody, tHavokImpact );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define TICKS_TO_IGNORE_IMPACT 10
static void sRemoveOldImpacts ( 
	RAGDOLL* pRagdoll )
{
	GAME * pGame = AppGetCltGame();
	if ( ! pGame )
		return;

	DWORD dwGameTick = GameGetTick( pGame );

	for ( UINT i = 0; i < pRagdoll->pImpacts.Count(); i++ )
	{
		HAVOK_IMPACT & tImpact = pRagdoll->pImpacts[ i ];
		if ( dwGameTick - tImpact.dwTick > TICKS_TO_IGNORE_IMPACT )
		{
			ArrayRemoveByIndex(pRagdoll->pImpacts, i);
			i--;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollAddImpact ( 
	int nModelId, 
	const HAVOK_IMPACT & tHavokImpactIn,
	float fModelScale,
	float fForceMultiplier )
{
	if (! sgbRagdollGameOpen )
		return;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll)
	{
		return;
	}

	HAVOK_IMPACT tHavokImpact = tHavokImpactIn;

	if ( ( tHavokImpact.dwFlags & HAVOK_IMPACT_FLAG_MULTIPLIER_APPLIED ) == 0 &&
		fForceMultiplier != 1.0f )
	{
		tHavokImpact.fForce *= fForceMultiplier;
		tHavokImpact.dwFlags |= HAVOK_IMPACT_FLAG_MULTIPLIER_APPLIED;
	}

	if ( tHavokImpact.fForce <= 0.0f )
		return;

	if ( (tHavokImpact.dwFlags & HAVOK_IMPACT_FLAG_IGNORE_SCALE) == 0 && fModelScale != 0.0f )
		tHavokImpact.fForce /= fModelScale;

	// either apply the impact or add it to a list
	if ( (pRagdoll->dwFlags & ( RAGDOLL_FLAG_DISABLED | RAGDOLL_FLAG_NEEDS_WORLD_TRANSFORM )) == 0 )
	{
		c_sRagdollApplyImpact ( pRagdoll, tHavokImpact );

	} else {
		sRemoveOldImpacts( pRagdoll );
		BOOL bAddImpact = TRUE;
		if ( tHavokImpact.dwFlags & HAVOK_IMPACT_FLAG_ONLY_ONE )
		{
			for ( UINT i = 0; i < pRagdoll->pImpacts.Count(); i++ )
			{
				if ( pRagdoll->pImpacts[ i ].dwFlags & HAVOK_IMPACT_FLAG_ONLY_ONE )
				{
					pRagdoll->pImpacts[ i ].dwTick = tHavokImpact.dwTick;
					bAddImpact = FALSE;
					break;
				}
			}
		}

		if ( bAddImpact )
			ArrayAddItem(pRagdoll->pImpacts, tHavokImpact);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollApplyAllImpacts ( 
	int nModelId )
{
	if (! sgbRagdollGameOpen )
		return;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll)
	{
		return;
	}

	sRemoveOldImpacts( pRagdoll );

	if ( (pRagdoll->dwFlags & ( RAGDOLL_FLAG_DISABLED | RAGDOLL_FLAG_NEEDS_WORLD_TRANSFORM )) == 0 )
	{

		for ( UINT i = 0; i < pRagdoll->pImpacts.Count(); i++ )
		{
			c_sRagdollApplyImpact( pRagdoll, pRagdoll->pImpacts[ i ] );
		}

		ArrayClear(pRagdoll->pImpacts);
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollDriveToPose ( 
	RAGDOLL * pRagdoll, 
	const hkQsTransform* desiredPose )
{
	if ( pRagdoll->dwFlags & RAGDOLL_FLAG_CONSTRAINTS_REMOVED )
		return;

	const int numBones = pRagdoll->pRagdollInstance->getNumBones();

	hkLocalArray<hkPositionConstraintMotor*> motors( numBones );

	for ( int b = 0; b < numBones; b++ )
	{
		hkConstraintInstance* constraint = pRagdoll->pRagdollInstance->getConstraintOfBone(b);

		if (!constraint)
		{
			continue;
		}

		const hkQsTransform& RBfromRA = desiredPose[b];

		hkConstraintData* constraintData = constraint->getData();

		switch (constraintData->getType())
		{
		case hkConstraintData::CONSTRAINT_TYPE_HINGE:
			{
				hkLimitedHingeConstraintData* powered = static_cast<hkLimitedHingeConstraintData*> (constraintData);

				// Calculate the setup relationship between both constrained spaces
				hkRotation rConstraintFromOriginalA;
				{
					rConstraintFromOriginalA.setCols(
						powered->m_atoms.m_transforms.m_transformA.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_AXLE), 
						powered->m_atoms.m_transforms.m_transformA.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_PERP_TO_AXLE_1), 
						powered->m_atoms.m_transforms.m_transformA.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_PERP_TO_AXLE_2));
				}

				hkRotation rConstraintFromOriginalB;
				{
					hkVector4 perp; perp.setCross(
						powered->m_atoms.m_transforms.m_transformB.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_PERP_TO_AXLE_2), 
						powered->m_atoms.m_transforms.m_transformB.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_AXLE));
					rConstraintFromOriginalB.setCols(
						powered->m_atoms.m_transforms.m_transformB.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_AXLE), 
						perp, 
						powered->m_atoms.m_transforms.m_transformB.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_PERP_TO_AXLE_2));
				}

				hkRotation rOriginalAFromOriginalB;
				{
					//originalAFromOriginalB = inv(constFromOriginalA) * constFromOriginalB
					hkRotation rOriginalAFromConstraint; rOriginalAFromConstraint.setTranspose(rConstraintFromOriginalA);
					rOriginalAFromOriginalB.setMul(rOriginalAFromConstraint, rConstraintFromOriginalB);
				}

				// From now on use quaternions
				hkQuaternion qOriginalAFromOriginalB (rOriginalAFromOriginalB);

				// We want OriginalAfromA
				// which is OriginalAfromA = OriginalAFromOriginalB * OrginalBFromB * BfromA
				// In this case, BfromOriginalB is the identity since we work only one way
				hkQuaternion qOriginalAfromA; qOriginalAfromA.setMul(qOriginalAFromOriginalB, RBfromRA.getRotation());

				const hkVector4& desiredRotAxis = powered->m_atoms.m_transforms.m_transformA.getColumn(hkLimitedHingeConstraintData::Atoms::AXIS_AXLE);
				hkQuaternion rest;
				hkReal desiredAngle;

				qOriginalAfromA.decomposeRestAxis(desiredRotAxis, rest, desiredAngle);
				powered->setMotorTargetAngle( desiredAngle );

				hkPositionConstraintMotor* motor = (hkPositionConstraintMotor*)powered->getMotor();
				if (motors.indexOf(motor) == -1)
				{
					motors.pushBack( motor );
				}

				break;
			}

		case hkConstraintData::CONSTRAINT_TYPE_RAGDOLL:
			{
				hkRagdollConstraintData* powered = static_cast<hkRagdollConstraintData*> (constraintData);
				hkRotation desired; desired.set (RBfromRA.getRotation());

				powered->setTargetFromRelativeFrame(desired);

				hkPositionConstraintMotor* motor = (hkPositionConstraintMotor*)powered->getPlaneMotor();
				if (motors.indexOf(motor) == -1)
				{
					motors.pushBack( motor );
				}
				motor = (hkPositionConstraintMotor*)powered->getConeMotor();
				if (motors.indexOf(motor) == -1)
				{
					motors.pushBack( motor );
				}
				motor = (hkPositionConstraintMotor*)powered->getTwistMotor();
				if (motors.indexOf(motor) == -1)
				{
					motors.pushBack( motor );
				}

				break;
			}
		}

	}

	for (int m=0 ;m < motors.getSize(); m++)
	{
		motors[m]->m_maxForce = INITIAL_RAGDOLL_POWER * pRagdoll->fPower;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollGetDefAndRagdoll(
	int nAppearanceDefId,
	int nModelId,
	RAGDOLL_DEFINITION ** ppRagdollDef,
	RAGDOLL ** ppRagdoll )
{
	if (! sgbRagdollGameOpen )
		return;

	if (ppRagdollDef)
	{
		*ppRagdollDef = HashGet(sgpRagdollDefinitons, nAppearanceDefId);
	}
	if (ppRagdoll)
	{
		*ppRagdoll = HashGet(sgpRagdolls, nModelId);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_RagdollGetPosition(
	int nModelId, VECTOR &vPosition )
{
	if (! sgbRagdollGameOpen )
		return FALSE;

	RAGDOLL* pRagdoll = HashGet(sgpRagdolls, nModelId);
	if (!pRagdoll || !pRagdoll->pRagdollInstance)
	{
		return FALSE;
	}

	vPosition = VECTOR(0);

	if ( (pRagdoll->dwFlags & RAGDOLL_FLAG_DISABLED) != 0 )
		return FALSE;

	if ( (pRagdoll->dwFlags & RAGDOLL_FLAG_NEEDS_WORLD_TRANSFORM) != 0 )
		return FALSE;

	hkRigidBody * pRigidBody = pRagdoll->pRagdollInstance->getRigidBodyOfBone( 0 );
	if ( pRigidBody )
	{
		hkVector4 vPositionHavok = pRigidBody->getPosition();
		vPosition.fX = vPositionHavok.getSimdAt( 0 );
		vPosition.fY = vPositionHavok.getSimdAt( 1 );
		vPosition.fZ = vPositionHavok.getSimdAt( 2 );
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//  ragdoll System 
//----------------------------------------------------------------------------
void c_RagdollGameInit(
	GAME * pGame )
{
	if ( ! sgbRagdollGameOpen )
		HashInit(sgpRagdolls, NULL, 64);
	sgbRagdollGameOpen = TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollGameClose(
	GAME * pGame)
{
	if ( sgbHavokOpen )
	{
		for (RAGDOLL* pRagdoll = HashGetFirst(sgpRagdolls); pRagdoll; pRagdoll = HashGetNext(sgpRagdolls, pRagdoll))
		{
			if ( pRagdoll->pRagdollRigidBodyController )
			{
				delete pRagdoll->pRagdollRigidBodyController;
				pRagdoll->pRagdollRigidBodyController = NULL;
			}

			if ( pRagdoll->pRagdollInstance )
			{
				pRagdoll->pRagdollInstance->removeReference();
				pRagdoll->pRagdollInstance = NULL;
			}
		}
	}
	sgbRagdollGameOpen = FALSE;
	HashFree(sgpRagdolls);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollSystemInit()
{
	HashInit(sgpRagdollDefinitons, NULL, 32);
	RandInit(sgRagdollsRand);

#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
	sgpRagdollUtilityRaycast = new RAGDOLLRAYCAST();
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollSystemClose()
{
	if (sgbHavokOpen == TRUE)
	{
		for (RAGDOLL_DEFINITION* pRagdollDef = HashGetFirst(sgpRagdollDefinitons); pRagdollDef; pRagdollDef = HashGetNext(sgpRagdollDefinitons, pRagdollDef))
		{
			if ( pRagdollDef->pRagdollInstance )
			{
				hkArray<hkRigidBody*>& rigidBodyArray = pRagdollDef->pRagdollInstance->m_rigidBodies;
				for ( int rigidBodyIterator = 0; 
					rigidBodyIterator < rigidBodyArray.getSize(); 
					rigidBodyIterator++ )
				{
					rigidBodyArray[rigidBodyIterator]->clearAndDeallocateProperties();
				}
				pRagdollDef->pRagdollInstance->removeReference();
				pRagdollDef->pRagdollInstance = NULL;
			}

			if ( pRagdollDef->pSkeletonMapperKeyToRagdoll )
			{
				delete pRagdollDef->pSkeletonMapperKeyToRagdoll;
				pRagdollDef->pSkeletonMapperKeyToRagdoll = NULL;
			}

			if ( pRagdollDef->pSkeletonMapperRagdollToKey )
			{
				delete pRagdollDef->pSkeletonMapperRagdollToKey;
				pRagdollDef->pSkeletonMapperRagdollToKey = NULL;
			}

			if ( pRagdollDef->pSkeletonRagdoll )
			{
				hkRagdollUtils::destroySkeleton( pRagdollDef->pSkeletonRagdoll );
				pRagdollDef->pSkeletonRagdoll = NULL;
			}

			if ( pRagdollDef->pAllocatedData )
			{
				pRagdollDef->pAllocatedData->removeReference();
				pRagdollDef->pAllocatedData = HK_NULL;
			}
		}
	}

	//cleanup ragdoll utility raycast instance
#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
	if(sgpRagdollUtilityRaycast)
	{
		delete sgpRagdollUtilityRaycast;
		sgpRagdollUtilityRaycast = 0;
	}
#endif

	HashFree(sgpRagdollDefinitons);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RagdollGameEnableAll(
	GAME * pGame )
{
	for (RAGDOLL* pRagdoll = HashGetFirst(sgpRagdolls); pRagdoll; pRagdoll = HashGetNext(sgpRagdolls, pRagdoll))
	{
		c_RagdollEnable( pRagdoll->nId );
	}

}

#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
void c_RagdollCheckAndCorrectPenetrations(RAGDOLL* pRagdoll)
{


	//CC@HAVOK todo: might be able to save cycles by not using HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK 
	// for some cases, and to not use at all
	// HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS. depends on how the geometries
	// are setup.


	//precalculate filter info
	hkUint32 recoveryColFilterInfo = hkGroupFilter::calcFilterInfo(COLFILTER_RAGDOLL_PENETRATION_RECOVERY_LAYER);
	hkUint32 normalRagdollColFilterInfo = hkGroupFilter::calcFilterInfo(COLFILTER_RAGDOLL_LAYER);


	//prepare output of penetration test
	hkDetectRagdollPenetration::Output output;

	//grab current pose
	hkPose ragdollPose( pRagdoll->pRagdollInstance->getSkeleton() );
	pRagdoll->pRagdollInstance->getPoseWorldSpace( ragdollPose.writeAccessPoseModelSpace().begin() );

	//check for penetrations
	pRagdoll->pRagdollPenetrationUtilityInstance
		->detectPenetration(ragdollPose,output);

	//CC@HAVOK todo: make better early out logic. need to make sure utility is called at least once.
	//... early out logic here would keep leaf bones from ever disabling when the ragdoll is known
	// to not be intersecting anywhere initially (e.g. a ragdoll dropped from the air striking the ground).

	////if everything is clear, early out.
	//if(pRagdoll->pRagdollPenetrationUtilityInstance->m_leafBonesMarkedAsDisabled.getSize() == 0
	//	&& output.m_penetrations.getSize() == 0)
	//{
	//	return;
	//}


	//respond to penetrations or lack there of
	const hkArray<hkDetectRagdollPenetration::BonePenetrationStatus> * pBoneStati
		= &(pRagdoll->pRagdollPenetrationUtilityInstance->getBoneStatusArray());
	int nBoneStati = pBoneStati->getSize();



	for( int b=0; b < nBoneStati; ++b )
	{
		switch(pBoneStati->operator[](b))
		{


			//first penetrating bone
		case hkDetectRagdollPenetration::HK_YESP:
			{
				/////////
				//disable collisions. apply corrective impulse

				hkRigidBody * bodyOfBone = pRagdoll->pRagdollInstance->getRigidBodyOfBone(b);

				if(bodyOfBone->getCollisionFilterInfo() != recoveryColFilterInfo)
				{
					//disable collisions
					bodyOfBone->setCollisionFilterInfo( recoveryColFilterInfo );

					hkWorld * pWorld = bodyOfBone->getWorld();
					if ( pWorld )
						pWorld->updateCollisionFilterOnEntity(
							bodyOfBone,
							HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,
							HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS );
				}






				//find surface normal of detected penetration
				hkVector4 hitpoint;
				hkVector4 hitnormal;

				

				int nPenetrationResults = output.m_penetrations.getSize();
				for(int i=0; i < nPenetrationResults; ++i)
				{
					if(output.m_penetrations[i].m_boneBeginIndex == b)
					{
						hitpoint = output.m_penetrations[i].m_penetrationPoint;
						hitnormal = output.m_penetrations[i].m_penetrationNormal;
						break;
					}
				}


				//prepare an impulse vector
				hkVector4 impulse;
				{

					// Impulse = limbmass*(vdesires - vactual*cos(alpha))
					const hkReal desiredVel = 1.5f;
					const hkReal impulseLimit = 15.0f;

					// get point velocity in hit point
					hkVector4 pointVel; bodyOfBone->getPointVelocity(hitpoint,pointVel);
					hkReal velMag = pointVel.length3();

					hkReal cosAlpha;

					// correct problem - unable normalize very small vector
					if ( velMag > hkMath::HK_REAL_EPSILON )
					{
						pointVel.normalize3();
						cosAlpha = hitnormal.dot3(pointVel);
					}
					else 
					{
						cosAlpha = 0.0f;
					}

					hkReal impulseMag = bodyOfBone->getMass()*( desiredVel-cosAlpha*velMag );
										
					impulseMag = (impulseMag > impulseLimit) ? impulseLimit : impulseMag;
					impulseMag = (impulseMag < -impulseLimit) ? -impulseLimit : impulseMag;

					impulse.setMul4( impulseMag,hitnormal );
				}


				//apply impulse
#ifdef _DEBUG
				//debug
				hkVector4 debugarrow;
				debugarrow = impulse;
				debugarrow.mul4(0.1f);
				HK_DISPLAY_ARROW(hitpoint,debugarrow,hkColor::PURPLE);
#endif			

				bodyOfBone->applyPointImpulse(impulse,hitpoint);

			}
			break;



			//child of a penetrating bone
		case hkDetectRagdollPenetration::HK_YESP_DESCENDANT:
		case hkDetectRagdollPenetration::HK_YESP_LEAF:
			{
				/////////
				//disable collisions. do not apply corrective impulse.


				hkRigidBody * bodyOfBone = pRagdoll->pRagdollInstance->getRigidBodyOfBone(b);

				if(bodyOfBone->getCollisionFilterInfo() != recoveryColFilterInfo)
				{

					//disable collisions
					bodyOfBone->setCollisionFilterInfo( recoveryColFilterInfo );

					hkWorld * pWorld = bodyOfBone->getWorld();
					if ( pWorld )
						pWorld->updateCollisionFilterOnEntity(
							bodyOfBone,
							HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,
							HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS );
				}

			}
			break;



			//bone is ok
		case hkDetectRagdollPenetration::HK_NOP:
			{
				/////////
				//reenable collisions.

				hkRigidBody * bodyOfBone = pRagdoll->pRagdollInstance->getRigidBodyOfBone(b);

				if(bodyOfBone->getCollisionFilterInfo() != normalRagdollColFilterInfo)
				{
					//disable collisions
					bodyOfBone->setCollisionFilterInfo( normalRagdollColFilterInfo );

					hkWorld * pWorld = bodyOfBone->getWorld();
					if ( pWorld )
						pWorld->updateCollisionFilterOnEntity(
							bodyOfBone,
							HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,
							HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS );
				}
			}
			break;


		}
	}
	
}
#else
void c_RagdollCheckAndCorrectPenetrations(RAGDOLL* pRagdoll)
{ return; }
#endif



///////////////////////////////
// RAGDOLL PENETRATION UTILITY
#ifdef ENABLE_HK_DETECT_RAGDOLL_PENETRATION
//COPIED FROM hkDetectedRagdollPenetration.cpp v4.6
#include <hkbase/hkBase.h>
#include <hkragdoll/hkRagdoll.h>

// Rig
#include <hkanimation/rig/hkPose.h>

// Ragdoll
#include <hkragdoll/instance/hkRagdollInstance.h>

// Phantoms
#include <hkdynamics/phantom/hkAabbPhantom.h>

//// Penetration Detection
//#include <hkragdoll/penetration/hkDetectRagdollPenetration.h>

//missing from current havok version
inline void hkAabb_setEmpty(hkAabb & aabb)
{
	aabb.m_min.setAll(hkMath::HK_REAL_MAX);
	aabb.m_max.setAll(-hkMath::HK_REAL_MAX);
}

//missing from current havok version
inline void hkAabb_includePoint(hkAabb & aabb, const hkVector4& point)
{
	aabb.m_min.setMin4(aabb.m_min,point);
	aabb.m_max.setMax4(aabb.m_max,point);
}

#define IN_RANGE(x,a,b) (((x)>=(a))&&((x)<=(b)))


hkDetectRagdollPenetration::hkDetectRagdollPenetration( const Setup& setup ) 
:
	m_setup(setup)	
{
	HK_ASSERT2 (0x7d917164, m_setup.m_ragdollSkeleton !=HK_NULL,"Invalid pointer to skeleton in detect penetration setup!");
	HK_ASSERT2 (0x7d917265, m_setup.m_raycastInterface !=HK_NULL,"Invalid pointer to raycast inteface in detect penetration setup!");
	HK_ASSERT2 (0x7d917266, m_setup.m_ragdollPhantom !=HK_NULL,"Invalid pointer to ragdoll Aabb phantom in detect penetration setup!");

	// set number of bones in skeleton
	m_numBones = m_setup.m_ragdollSkeleton->m_numBones;
	
	m_boneStates.setSize(m_numBones, HK_YESP );


	
	//CC@HAVOK MOD
	m_leafBones.reserve(m_numBones/2);

	//find which bones are leaf bones
	const hkSkeleton * cachedSkeletonPointer = m_setup.m_ragdollSkeleton;
	for(int x=0; x<m_numBones; ++x)
	{
		//does bone X have any children?
		bool boneXhasChildren = false;
		for(int y=1; y<m_numBones; ++y)
		{
			if(cachedSkeletonPointer->m_parentIndices[y] == x)
			{
				boneXhasChildren = true;
				break;
			}
		}

		if(boneXhasChildren)
		{
			continue;
		}

		if(m_leafBones.indexOf(x) == -1)
		{
			m_leafBones.pushBack(x);
		}
	}

	m_leafBonesMarkedAsDisabled.reserve(m_numBones);	
	//END CC@HAVOK MOD




	m_ragdollIsOut = false;
	
}

hkDetectRagdollPenetration::BonePenetration::BonePenetration()
: m_boneBeginIndex(0),
  m_boneEndIndex(1)
{
	m_penetrationPoint.setZero4();
	m_penetrationNormal.setZero4();  	
}

void hkDetectRagdollPenetration::resetBoneStates()
{
	for(int k=0; k < m_boneStates.getSize(); ++k)
	{
		m_boneStates[k] = HK_YESP;
	}

	m_ragdollIsOut = false;
}


hkDetectRagdollPenetration::BonePenetration::~BonePenetration()
{
}

hkBool HK_CALL hkDetectRagdollPenetration::detectPenetration( const hkPose& ragdollPoseIn, Output& output )
{
	// clear output array
	output.m_penetrations.clear();

	// if whole pose out ! 
	if (!m_ragdollIsOut)
	{
	
		// set Aabb from pose
		{
			hkVector4 aabbPointFrom;
			hkVector4 aabbPointTo; 

			hkAabb aabb;
			hkAabb_setEmpty(aabb);

			for (hkInt16 bone = 1; bone < m_numBones; bone++)
			{

				const hkInt16 parent = (hkInt16) m_setup.m_ragdollSkeleton->m_parentIndices[bone];

				if ( m_boneStates[parent] != hkDetectRagdollPenetration::HK_NOP)
				{
					aabbPointFrom = ragdollPoseIn.getBoneModelSpace( parent ).getTranslation();
					aabbPointTo = ragdollPoseIn.getBoneModelSpace( bone ).getTranslation();

					hkAabb_includePoint(aabb,aabbPointFrom);
					hkAabb_includePoint(aabb,aabbPointTo);	
				}
            }

			if (aabb.isValid())
			{
				m_setup.m_ragdollPhantom->setAabb(aabb);
			}

		}


		hkVector4 bonePointFrom;
		hkVector4 bonePointTo; 

		hkVector4 hitPoint;
		hkVector4 hitNormal;

		// loop over all bones in pose
		for (hkInt16 bone = 1; bone < m_numBones; bone++)
		{

			const hkInt16 parent = (hkInt16) m_setup.m_ragdollSkeleton->m_parentIndices[bone];

			if ( m_boneStates[parent] != HK_NOP)
			{

				bonePointFrom = ragdollPoseIn.getBoneModelSpace( parent ).getTranslation();
				bonePointTo = ragdollPoseIn.getBoneModelSpace( bone ).getTranslation();


				//debugging 
				HK_DISPLAY_LINE(bonePointFrom,bonePointTo,hkColor::GREEN);


				hkBool properHit = m_setup.m_raycastInterface->castRay(m_setup.m_ragdollPhantom, bonePointFrom, bonePointTo, hitPoint, hitNormal );

				if (properHit) 
				{
					BonePenetration bp;
		            
					bp.m_boneBeginIndex = parent;
					bp.m_boneEndIndex = bone;
           			bp.m_penetrationPoint = hitPoint; 
					bp.m_penetrationNormal = hitNormal; 

					output.m_penetrations.pushBack(bp);										
				}				
			}

    	}

	}
	

	// update states of bones ( not penetrated, penetrated or descendant )
	{

		// fill boneState array with no penetration
		for(int k=0; k < m_boneStates.getSize();k++)
		{
			m_boneStates[k] = HK_NOP;
		}

		// if no penetration, change poseState to do nothing in the next step
        if (output.m_penetrations.getSize() == 0)
		{
			m_ragdollIsOut = true;
		}
		else
		{

			for (int i = 0; i < output.m_penetrations.getSize(); i++)
			{
				const hkInt16 penBone = output.m_penetrations[i].m_boneBeginIndex;

				if (m_boneStates[penBone] != HK_YESP_DESCENDANT)
				{
					m_boneStates[penBone] = HK_YESP;
				}

				hkArray<hkInt16> descendantBones;

				hkSkeletonUtils::getDescendants( *m_setup.m_ragdollSkeleton, penBone, descendantBones );

				for (int j=0; j < descendantBones.getSize(); j++)
				{
					const hkInt16 dbId = descendantBones[j];

					if ( m_boneStates[dbId] != HK_YESP_DESCENDANT )
					{
						m_boneStates[dbId] = HK_YESP_DESCENDANT;				
					}




					////////
					//CC@HAVOK MOD
					//find leaf children

					//search for children of bone 'dbId', if none then bone 'dbId' is associated with a leaf rigid body.
					int cachedNumBones = m_setup.m_ragdollSkeleton->m_numBones;
					bool descendantBoneHasChild = false;
					for(int e=1; e<cachedNumBones; ++e)
					{
						if(m_setup.m_ragdollSkeleton->m_parentIndices[e] == dbId)
						{
							descendantBoneHasChild = true;
							break;
						}
					}

					if(descendantBoneHasChild)
					{
						//bone 'dbId' is not a leaf rigid body.
						continue;
					}

					//save record of this disabled leaf bone.
					if(m_leafBonesMarkedAsDisabled.indexOf(dbId) <= -1)
					{
						m_leafBonesMarkedAsDisabled.pushBack(dbId);
					}

					//END CC@HAVOK MOD
					/////////

				}
			}
		}

		////
		//CC@HAVOK MOD
		//special case: perform raycasts at leaf bones to look for penetrations.
		//performs raycasts in various directions at the pivot point to guess if penetrating.


		//increase this value if leaf bones are getting stuck.
		//descrease this value if leaf bone collisions are being turned off too liberally
		const float lengthOfRaycastForLeafBoneGuess = 0.075f;

		for(int x=0; x<m_leafBones.getSize(); ++x)
		{
			//if leaf bone already disabled, dont need to raycast further, since its going to stay disabled.
			if(m_leafBonesMarkedAsDisabled.indexOf(m_leafBones[x]) != - 1)
			{
				continue;
			}


			const int nDirectionsToCheck = 6;
			

			for(int j=0; j<nDirectionsToCheck; ++j)
			{
				hkVector4 startingPoint = ragdollPoseIn.getBoneModelSpace( m_leafBones[x] ).getTranslation();
				hkVector4 endingPoint;
				switch(j)
				{
				default:
				case 0:
					//check downward in worldspace
					endingPoint.set(0,0,-lengthOfRaycastForLeafBoneGuess); 
					break;
				case 1:
					//check upward in worldspace
					endingPoint.set(0,0,lengthOfRaycastForLeafBoneGuess); 
					break;

					//check other directions. may not be neccessary in practice.
				case 2:
					endingPoint.set(0,-lengthOfRaycastForLeafBoneGuess,0); 
					break;
				case 3:
					endingPoint.set(0,lengthOfRaycastForLeafBoneGuess,0); 
					break;
				case 4:
					endingPoint.set(-lengthOfRaycastForLeafBoneGuess,0,0); 
					break;
				case 5:
					endingPoint.set(lengthOfRaycastForLeafBoneGuess,0,0); 
					break;
				}
				endingPoint.add3clobberW(startingPoint);

				hkVector4 hitPoint2;
				hkVector4 hitNormal2;

				//debugging 
#ifdef _DEBUG
				HK_DISPLAY_LINE(startingPoint,endingPoint,hkColor::LIME);
#endif


				hkBool hit = m_setup.m_raycastInterface->castRay(m_setup.m_ragdollPhantom, startingPoint, endingPoint, hitPoint2, hitNormal2 );

				if (hit) 
				{									
					//save record of this disabled leaf bone.
					if(m_leafBonesMarkedAsDisabled.indexOf(m_leafBones[x]) <= -1)
					{
						m_leafBonesMarkedAsDisabled.pushBack(m_leafBones[x]);
						break;
					}
				}

			}
		}

		////
		for(int x=0; x<m_leafBonesMarkedAsDisabled.getSize(); ++x)
		{
			m_boneStates[m_leafBonesMarkedAsDisabled[x]] = HK_YESP_LEAF;
		}
			
	}

	return true;

}

#endif //ENABLE_HK_DETECT_RAGDOLL_PENETRATION







#endif //HAVOK