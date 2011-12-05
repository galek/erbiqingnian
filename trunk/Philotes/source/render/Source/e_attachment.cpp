//----------------------------------------------------------------------------
// e_attachment.cpp
//
// - Implementation for attachment functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_common.h"

#include "e_attachment.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

PFN_ATTACHMENTDEF_NEWSTATICLIGHT	gpfn_AttachmentDefinitionNewStaticLight	= NULL;
PFN_ATTACHMENTDEF_LOAD				gpfn_AttachmentDefinitionLoad			= NULL;
PFN_ATTACHMENT_NEW					gpfn_AttachmentNew						= NULL;
PFN_ATTACHMENT_REMOVE				gpfn_AttachmentRemove					= NULL;
PFN_ATTACHMENT_MODEL_NEW			gpfn_AttachmentModelNew					= NULL;
PFN_ATTACHMENT_MODEL_REMOVE			gpfn_AttachmentModelRemove				= NULL;
PFN_ATTACHMENT_GET_ATTACHED_OF_TYPE	gpfn_AttachmentGetAttachedOfType		= NULL;
PFN_ATTACHMENT_GET_ATTACHED_BY_ID	gpfn_AttachmentGetAttachedById			= NULL;
PFN_ATTACHMENT_SET_MODEL_FLAGBIT	gpfn_AttachmentSetModelFlagbit			= NULL;
PFN_ATTACHMENT_SET_VISIBLE			gpfn_AttachmentSetVisible				= NULL;
PFN_MODEL_ATTACHMENTS_UPDATE		gpfn_ModelAttachmentsUpdate				= NULL;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
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
	PFN_MODEL_ATTACHMENTS_UPDATE pfn_ModelAttachmentsUpdate )
{
	gpfn_AttachmentDefinitionNewStaticLight = pfn_AttachmentDefinitionNewStaticLight;
	gpfn_AttachmentDefinitionLoad = pfn_AttachmentDefinitionLoad;
	gpfn_AttachmentNew = pfn_AttachmentNew;
	gpfn_AttachmentRemove = pfn_AttachmentRemove;
	gpfn_AttachmentModelNew = pfn_AttachmentModelNew;
	gpfn_AttachmentModelRemove = pfn_AttachmentModelRemove;
	gpfn_AttachmentGetAttachedOfType = pfn_AttachmentGetAttachedOfType;
	gpfn_AttachmentGetAttachedById = pfn_AttachmentGetAttachedById;
	gpfn_AttachmentSetModelFlagbit = pfn_AttachmentSetModelFlagbit;
	gpfn_AttachmentSetVisible = pfn_AttachmentSetVisible;
	gpfn_ModelAttachmentsUpdate = pfn_ModelAttachmentsUpdate;

	return S_OK;
}
