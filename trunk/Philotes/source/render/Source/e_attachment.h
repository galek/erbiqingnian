//----------------------------------------------------------------------------
// e_attachment.h
//
// - Header for attachment functions\structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_ATTACHMENT__
#define __E_ATTACHMENT__




//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

typedef enum {
	ATTACHMENT_LIGHT,
	ATTACHMENT_PARTICLE,
	ATTACHMENT_MODEL,
	ATTACHMENT_SOUND,
	ATTACHMENT_PARTICLE_ROPE_END,
	ATTACHMENT_FOOTSTEP,
	// count
	NUM_ATTACHMENT_TYPES
}ATTACHMENT_TYPE;

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void (*PFN_ATTACHMENTDEF_NEWSTATICLIGHT)( int nModelID, struct STATIC_LIGHT_DEFINITION & pStaticLightDef, BOOL bSpecularOnly );
typedef void (*PFN_ATTACHMENTDEF_LOAD)( struct ATTACHMENT_DEFINITION & tDef, int nAppearanceDefId, const char* pszFolder );
typedef int (*PFN_ATTACHMENT_NEW)( int nModelID, struct ATTACHMENT_DEFINITION * pDef );
typedef void (*PFN_ATTACHMENT_REMOVE)( int nModelId, int nId, DWORD dwFlags );
typedef void (*PFN_ATTACHMENT_MODEL_NEW)( int nModelID );
typedef void (*PFN_ATTACHMENT_MODEL_REMOVE)( int nModelID );
typedef int  (*PFN_ATTACHMENT_GET_ATTACHED_OF_TYPE)( int nOwnerModelID, ATTACHMENT_TYPE eType, int & nNextAttachmentIndex );
typedef int  (*PFN_ATTACHMENT_GET_ATTACHED_BY_ID)( int nModelId, int nAttachmentId );
typedef void (*PFN_ATTACHMENT_SET_MODEL_FLAGBIT)( int nModelID, int nFlagBit, BOOL bSet );
typedef void (*PFN_ATTACHMENT_SET_VISIBLE)( int nModelID, BOOL bSet );
typedef void (*PFN_MODEL_ATTACHMENTS_UPDATE)( int nModelID, MODEL_SORT_TYPE SortType );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
void e_AttachmentDefInit( struct ATTACHMENT_DEFINITION &tDef );

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct ATTACHMENT_DEFINITION 
{
	ATTACHMENT_TYPE eType;
	DWORD			dwFlags;

	int			    nVolume;

	char			pszAttached[ MAX_XML_STRING_LENGTH ];
	int				nAttachedDefId;

	char			pszBone	   [ MAX_XML_STRING_LENGTH ];
	int				nBoneId;

	VECTOR			vPosition;	
	VECTOR			vNormal;
	float			fRotation;
	VECTOR			vScale;

	float			fYaw;
	float			fPitch;
	float			fRoll;

	// *constructor*
	ATTACHMENT_DEFINITION( void ) { e_AttachmentDefInit( *this ); }
	
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern PFN_ATTACHMENTDEF_NEWSTATICLIGHT gpfn_AttachmentDefinitionNewStaticLight;
extern PFN_ATTACHMENTDEF_LOAD gpfn_AttachmentDefinitionLoad;
extern PFN_ATTACHMENT_NEW gpfn_AttachmentNew;
extern PFN_ATTACHMENT_REMOVE gpfn_AttachmentRemove;
extern PFN_ATTACHMENT_MODEL_NEW gpfn_AttachmentModelNew;
extern PFN_ATTACHMENT_MODEL_REMOVE gpfn_AttachmentModelRemove;
extern PFN_ATTACHMENT_GET_ATTACHED_OF_TYPE gpfn_AttachmentGetAttachedOfType;
extern PFN_ATTACHMENT_GET_ATTACHED_BY_ID gpfn_AttachmentGetAttachedById;
extern PFN_ATTACHMENT_SET_MODEL_FLAGBIT gpfn_AttachmentSetModelFlagbit;
extern PFN_ATTACHMENT_SET_VISIBLE gpfn_AttachmentSetVisible;
extern PFN_MODEL_ATTACHMENTS_UPDATE gpfn_ModelAttachmentsUpdate;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void e_AttachmentDefInit  ( 
	ATTACHMENT_DEFINITION & tDef )
{
	tDef.eType				= ATTACHMENT_MODEL;
	tDef.dwFlags			= 0;
	tDef.nVolume			= 1;
	tDef.nAttachedDefId		= INVALID_ID;
	tDef.nBoneId			= INVALID_ID;
	tDef.vPosition			= VECTOR( 0.0f );
	tDef.vNormal			= VECTOR( 0.0f, 1.0f, 0.0f );
	tDef.fRotation			= 0.0f;	// same as yaw, but here for legacy reasons
	tDef.fYaw				= 0.0f;
	tDef.fPitch				= 0.0f;
	tDef.fRoll				= 0.0f;
	tDef.vScale				= VECTOR( 1.0f, 1.0f, 1.0f );
	tDef.pszAttached[ 0 ]	= 0;
	tDef.pszBone[ 0 ]		= 0;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_AttachmentInitCallbacks(
	PFN_ATTACHMENTDEF_NEWSTATICLIGHT pfn_AttachmentDefinitionNewStaticLight,
	PFN_ATTACHMENTDEF_LOAD pfn_AttachmentDefinitionLoad,
	PFN_ATTACHMENT_NEW pfn_AttachmentNew,
	PFN_ATTACHMENT_REMOVE pfn_AttachmentRemove,
	PFN_ATTACHMENT_MODEL_NEW pfn_AttachmentModelNew,
	PFN_ATTACHMENT_MODEL_REMOVE pfn_AttachmentModelRemove,
	PFN_ATTACHMENT_GET_ATTACHED_OF_TYPE pfn_AttachmentGetAttachedOfType,
	PFN_ATTACHMENT_GET_ATTACHED_BY_ID pfn_AttachmentGetAttachedById,
	PFN_ATTACHMENT_SET_MODEL_FLAGBIT pfn_AttachmentSetModelFlagbit,
	PFN_ATTACHMENT_SET_VISIBLE pfn_AttachmentSetVisible,
	PFN_MODEL_ATTACHMENTS_UPDATE pfn_ModelAttachmentsUpdate );

#endif // __E_ATTACHMENT__