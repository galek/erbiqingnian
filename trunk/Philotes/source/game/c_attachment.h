#pragma once
// c_attachment.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_attachment.h"
#include "e_common.h"
#include "e_light.h"



typedef enum 
{
	ATTACHMENT_OWNER_NONE = 0,
	ATTACHMENT_OWNER_SKILL,
	ATTACHMENT_OWNER_ANIMATION,
	ATTACHMENT_OWNER_EVENT,
	ATTACHMENT_OWNER_WARDROBE,
	ATTACHMENT_OWNER_STATE,
	ATTACHMENT_OWNER_HOLYRADIUS,
	ATTACHMENT_OWNER_MISSILE,
	NUM_ATTACHMENT_OWNER_TYPES,
};

#define ATTACHMENT_FLAGS_PARTICLE_ROPE_END		MAKE_MASK(0)
#define ATTACHMENT_FLAGS_FORCE_NEW				MAKE_MASK(1)
#define ATTACHMENT_FLAGS_WEAPON_INDEX_0			MAKE_MASK(2)
#define ATTACHMENT_FLAGS_WEAPON_INDEX_1			MAKE_MASK(3)
#define ATTACHMENT_SHIFT_WEAPON_INDEX			2
#define ATTACHMENT_MASK_WEAPON_INDEX			(ATTACHMENT_FLAGS_WEAPON_INDEX_0 | ATTACHMENT_FLAGS_WEAPON_INDEX_1)
#define ATTACHMENT_FLAGS_ABSOLUTE_POSITION		MAKE_MASK(5)
#define ATTACHMENT_FLAGS_ABSOLUTE_NORMAL		MAKE_MASK(6)
#define ATTACHMENT_FLAGS_FLOAT					MAKE_MASK(7)
#define ATTACHMENT_FLAGS_POINT_AT_CAMERA_FOCUS	MAKE_MASK(8)
#define ATTACHMENT_FLAGS_FIND_CHECK_NORMAL		MAKE_MASK(9)
#define ATTACHMENT_FLAGS_DONT_MUTE_SOUND		MAKE_MASK(10)
#define ATTACHMENT_FLAGS_NEED_BBOX_UPDATE		MAKE_MASK(11)
#define ATTACHMENT_FLAGS_FORCE_3D				MAKE_MASK(12)
#define ATTACHMENT_FLAGS_FORCE_2D				MAKE_MASK(13)
#define ATTACHMENT_FLAGS_IS_FOOTSTEP			MAKE_MASK(14)
#define ATTACHMENT_FLAGS_SCALE_OFFSET			MAKE_MASK(15)
#define ATTACHMENT_FLAGS_INHERIT_PARENT_MODES	MAKE_MASK(16)
#define ATTACHMENT_FLAGS_DONT_MOVE_AFTER_CREATE	MAKE_MASK(17)
#define ATTACHMENT_FLAGS_DONT_MOVE				MAKE_MASK(18)
#define ATTACHMENT_FLAGS_MIRROR					MAKE_MASK(19)


struct ATTACHMENT 
{
	ATTACHMENT_TYPE eType;
	int				nId;
	DWORD			dwFlags;
	int				nAttachedId;
	int				nBoneId;
	int				nVolume;
	VECTOR			vPositionInObject;
	VECTOR			vNormalInObject;
	float			fRotation;		// same as yaw, but here for legacy reasons
	float			fPitch;
	float			fYaw;
	float			fRoll;
	VECTOR			vScale;
	char			pszBoneName[MAX_XML_STRING_LENGTH];

	struct OWNER
	{
		int		nType;
		int		nId;
		OWNER( int nTypeIn, int nIdIn )
		{
			nType = nTypeIn;
			nId   = nIdIn;
		}
		OWNER()
		{
			nType = ATTACHMENT_OWNER_NONE;
			nId   = INVALID_ID;
		}
	};
	OWNER			tOwner;

	ATTACHMENT(void)
	:	eType( ATTACHMENT_MODEL ),
		nId ( INVALID_ID ),
		dwFlags( 0 ),
		nAttachedId ( INVALID_ID ),
		nBoneId( INVALID_ID ),
		nVolume( 1 ),
		vPositionInObject( 0.0f ),
		vNormalInObject( 0.0f, 1.0f, 0.0f ),
		vScale( 1.0f, 1.0f, 1.0f ),
		fRotation( 0.0f ),
		fPitch( 0.0f ),
		fYaw( 0.0f ),
		fRoll( 0.0f ),
		tOwner( ATTACHMENT_OWNER_NONE, INVALID_ID )
	{
		memset(pszBoneName, 0, MAX_XML_STRING_LENGTH);
	}
};

#define ATTACHMENT_HOLDER_FLAGS_NODRAW				MAKE_MASK( 0 )
#define ATTACHMENT_HOLDER_FLAGS_NOLIGHTS			MAKE_MASK( 1 )
#define ATTACHMENT_HOLDER_FLAGS_NOSOUND				MAKE_MASK( 2 )
#define ATTACHMENT_HOLDER_FLAGS_VISIBLE				MAKE_MASK( 3 )
#define ATTACHMENT_HOLDER_FLAGS_FIRST_PERSON_PROJ	MAKE_MASK( 4 )
#define ATTACHMENT_HOLDER_FLAGS_DONTPLAYSOUND		MAKE_MASK( 5 )

struct ATTACHMENT_HOLDER 
{
	// needed for the hash
	int					nId;
	ATTACHMENT_HOLDER	*pNext;

	ATTACHMENT				*pAttachments;
	DWORD					dwFlags;
	LIGHT_PRIORITY_TYPE		eLightPriority;
	int						nRegionID;
	int						nCount;
	int						nAllocated;
	ATTACHMENT_HOLDER(void)
		:	pAttachments( NULL ),
		nRegionID ( -1 ),
		nCount ( 0 ),
		nAllocated( 0 )
	{}
};

// this is for the list of some bones that can be used for attachments - Skills use them
struct BONE_DATA 
{
	char			szName[ DEFAULT_INDEX_SIZE ];
};

#define ATTACHMENT_FUNC_FLAG_FLOAT				MAKE_MASK(0)
#define ATTACHMENT_FUNC_FLAG_FORCE_DESTROY		MAKE_MASK(1)
#define ATTACHMENT_FUNC_FLAG_REMOVE_CHILDREN	MAKE_MASK(2)

void c_AttachmentDefinitionLoad( 
	struct GAME* game,
	ATTACHMENT_DEFINITION& tDef, 
	int nAppearanceDefId, 
	const char* pszFolder );

void c_AttachmentDefinitionFlagSoundsForLoad( 
	struct GAME* game,
	ATTACHMENT_DEFINITION& tDef,
	BOOL bLoadNow);

int c_AttachmentNew( 
	ATTACHMENT_DEFINITION& tDef, 
	ATTACHMENT_HOLDER& tHolder, 
	ROOMID idRoom, 
	const MATRIX& tWorldMatrix,
	const ATTACHMENT::OWNER & tOwner,
	int nAttachedId = INVALID_ID);

int c_AttachmentNew( 
	ATTACHMENT_DEFINITION& tDef, 
	ATTACHMENT& tAttachment, 
	ROOMID idRoom, 
	const MATRIX& tWorldMatrix,
	const ATTACHMENT::OWNER & tOwner,
	int nAttachedId = INVALID_ID);

BOOL c_AttachmentCanDestroy( 
	ATTACHMENT & tAttachment );

void c_AttachmentDestroy( 
	ATTACHMENT& tAttachment,
	DWORD dwFlags );

void c_AttachmentDestroy( 
	int nId, 
	ATTACHMENT_HOLDER& tHolder,
	DWORD dwFlags );

void c_AttachmentDestroyAll( 
	ATTACHMENT_HOLDER& tHolder,
	DWORD dwFlags );

void c_AttachmentDestroyAllByOwner( 
	ATTACHMENT_HOLDER& tHolder,
	ATTACHMENT::OWNER tOwner,
	DWORD dwFlags );

void c_AttachmentHolderDestroy( 
	ATTACHMENT_HOLDER& tHolder );

void c_AttachmentMove( 
	ATTACHMENT& tAttachment, 
	int nModelId,
	const MATRIX& matWorld, 
	ROOMID idRoom,
	MODEL_SORT_TYPE Sort = MODEL_DYNAMIC );



void c_AttachmentNewStaticLightCallback( 
	int nModelID,
	STATIC_LIGHT_DEFINITION & tStaticLightDef, 
	BOOL bSpecularOnly );

void c_AttachmentNewStaticLight( 
	struct GAME* game,
	ATTACHMENT_HOLDER & tHolder, 
	ROOMID idRoom, 
	const MATRIX& tWorldMatrix, 
	struct STATIC_LIGHT_DEFINITION & tStaticLightDef, 
	BOOL bSpecularOnly);

void c_AttachmentSetVisible( 
	ATTACHMENT& tAttachment, 
	BOOL bSet);

void c_AttachmentSetDraw( 
	ATTACHMENT & tAttachment, 
	BOOL bSet );

void c_ModelSetDrawLayer( 
	int nModelId, 
	DRAW_LAYER eDrawLayer );

void c_AttachmentSetDrawLayer( 
	ATTACHMENT & tAttachment, 
	BYTE bDrawLayer );

void c_ModelSetRegion( 
	int nModelId, 
	int nRegionID );

void c_AttachmentSetRegion( 
	ATTACHMENT & tAttachment, 
	int nRegionID );

void c_AttachmentSetFirstPerson( 
	ATTACHMENT & tAttachment, 
	BOOL bSet );

int c_AttachmentFind( 
	ATTACHMENT_DEFINITION & tDef,
	ATTACHMENT_HOLDER & tHolder,
	int nAttachedId );

int c_AttachmentFind( 
	ATTACHMENT_HOLDER & tHolder,
	int nOwnerType,
	int nOwnerId );

void c_AttachmentsRemoveNonExisting( 
	int nModelId );
 
int c_AttachmentFindModelWithBone( 
	ATTACHMENT_HOLDER & tHolder, 
	ATTACHMENT_DEFINITION & tAttachmentDef,
	int * pnBoneId );

void c_AttachmentSetWeaponIndex ( 
	struct GAME * pGame,
	ATTACHMENT_DEFINITION & tAttachmentDef,
	struct UNIT * pContainer,
	UNITID idWeapon );

void c_AttachmentHolderSetRegion ( 
	ATTACHMENT_HOLDER & tAttachmentHolder,
	int nRegionID );

void c_AttachmentHolderSetFlag ( 
	ATTACHMENT_HOLDER & tAttachmentHolder,
	DWORD dwFlag,
	BOOL bValue );

ATTACHMENT * c_AttachmentGet( 
	int nAttachmentId,
	ATTACHMENT_HOLDER & tHolder );

void c_AttachmentSetCameraFocus( 
	const VECTOR & vPosition );

//#ifdef HAMMER
extern const struct SYMBOL gtAttachmentTypeSymbols[ ];
//#endif

int c_ModelAttachmentAdd( 
	int nModelId, 
	ATTACHMENT_DEFINITION & tAttachmentDef, 
	const char * pszFolder,
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	int nAttachedId );

void c_ModelAttachmentAddBBox( 
	int nModelId, 
	int nCount,
	ATTACHMENT_DEFINITION & tAttachmentDef, 
	const char * pszFolder,
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	int nAttachedId,
	BOOL bFloatAttachments );
	
void c_ModelAttachmentAddToRandomBones( 
	int nModelId, 
	int nCount,
	ATTACHMENT_DEFINITION & tAttachmentDef, 
	const char * pszFolder,
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	int nAttachedId,
	BOOL bFloatAttachments );

void c_ModelAttachmentAddAtBoneWeight( 
	int nModelId, 
	const ATTACHMENT_DEFINITION & tAttachmentDef, 
	BOOL bForceNew,
	int nOwnerType,
	int nOwnerId,
	BOOL bFloat);

int c_ModelAttachmentFind( 
	int nModelId,
	ATTACHMENT_DEFINITION & tDef,
	int nAttachedId = INVALID_ID );

int c_ModelAttachmentFind( 
	int nModelId, 
	int nOwnerType, 
	int nOwnerId );

void c_ModelAttachmentRemove( 
	int nModelId, 
	ATTACHMENT_DEFINITION & tDef, 
	const char * pszFolder, 
	DWORD dwFlags );

void c_ModelAttachmentFloat(
	int nModelId,
	int nAttachmentId );

void c_ModelAttachmentRemove(
	int nModelId,
	int nId,
	DWORD dwFlags );

void c_ModelAttachmentRemoveAll(
	int nModelId,
	DWORD dwFlags );

void c_ModelAttachmentRemoveAllByOwner( 
	int nModelId, 
	int nOwnerType, 
	int nOwnerId,
	DWORD dwFlags = ATTACHMENT_FUNC_FLAG_REMOVE_CHILDREN );

ATTACHMENT * c_ModelAttachmentGet(
	int nModelId,
	int nAttachmentId );

void c_ModelAttachmentsUpdate(
	int nModelID,
	MODEL_SORT_TYPE SortType = MODEL_DYNAMIC );

void c_ModelAttachmentWarpToCurrentPosition( 
	int nModelID );

void c_ModelSetAttachmentNormals( 
	int nModelID,
	const VECTOR & vNormal );

void c_ModelSetParticleSystemParam( 
	int nModelId, 
	enum PARTICLE_SYSTEM_PARAM eParam,
	float fValue);

void c_ModelSetLightPriorityType( 
	int nModelId,
	LIGHT_PRIORITY_TYPE eType );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Attachment holder IDs are the same as appearance and model IDs
inline ATTACHMENT_HOLDER * c_AttachmentHolderGet( int nID )
{
	extern CHash<ATTACHMENT_HOLDER> gtAttachmentHolders;
	return HashGet( gtAttachmentHolders, nID );
}

inline ATTACHMENT_HOLDER * c_AttachmentHolderGetFirst()
{
	extern CHash<ATTACHMENT_HOLDER> gtAttachmentHolders;
	return HashGetFirst( gtAttachmentHolders );
}

inline ATTACHMENT_HOLDER * c_AttachmentHolderGetNext( ATTACHMENT_HOLDER * pCur )
{
	extern CHash<ATTACHMENT_HOLDER> gtAttachmentHolders;
	return HashGetNext( gtAttachmentHolders, pCur );
}

inline int c_AttachmentGetAttachedOfType( int nOwnerID, ATTACHMENT_TYPE eType, int & nNextAttachmentIndex )
{
	if ( nNextAttachmentIndex < 0 )
		nNextAttachmentIndex = 0;
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nOwnerID );
	if ( !pHolder )
	{
		ASSERT( pHolder );
		nNextAttachmentIndex = -1;
		return INVALID_ID;
	}
	for ( ; nNextAttachmentIndex < pHolder->nCount; nNextAttachmentIndex++ )
	{
		if ( pHolder->pAttachments[ nNextAttachmentIndex ].eType == eType )
		{
			nNextAttachmentIndex++;
			ASSERT( pHolder->pAttachments[ nNextAttachmentIndex - 1 ].nAttachedId >= 0 );
			return pHolder->pAttachments[ nNextAttachmentIndex - 1 ].nAttachedId;
		}
	}
	return INVALID_ID;
}

inline int c_AttachmentGetAttachedById( int nModelId, int nAttachmentId )
{
	ATTACHMENT * pAttachment = c_ModelAttachmentGet( nModelId, nAttachmentId );
	return pAttachment ? pAttachment->nAttachedId : INVALID_ID;
}

inline void c_AttachmentCountAttachedByType( int nOwnerID, int pnCounts[NUM_ATTACHMENT_TYPES] )
{
	ATTACHMENT_HOLDER * pHolder = c_AttachmentHolderGet( nOwnerID );
	if ( ! pHolder )
		return;
	for ( int nIndex = 0; nIndex < pHolder->nCount; nIndex++ )
		pnCounts[ pHolder->pAttachments[ nIndex ].eType ]++;
}

void c_AttachmentSetModelRegion( int nModelID, int nRegionID );
void c_AttachmentSetModelFlagbit( int nModelID, int nFlagbit, BOOL bSet );

void c_AttachmentInit();
void c_AttachmentDestroy();