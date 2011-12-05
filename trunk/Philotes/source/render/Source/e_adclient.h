//----------------------------------------------------------------------------
// c_adclient.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _E_ADCLIENT_H_
#define _E_ADCLIENT_H_

#include "boundingbox.h"


#define	AD_CLIENT_ENABLED

#if ISVERSION(SERVER_VERSION) || ISVERSION(DEMO_VERSION)
#	undef AD_CLIENT_ENABLED
#endif



//----------------------------------------------------------------------------
//
// DEFINES
//
//----------------------------------------------------------------------------

#define MAX_ADOBJECT_NAME_LEN				64
#define AD_OBJECT_PATH_CODE					MAKEFOURCC('A','D','S','1')
#define NONREPORTING_AD_OBJECT_PATH_CODE	MAKEFOURCC('A','D','N','R')

#define AD_CLIENT_MIN_SAMPLES_PER_METER		1.5f

#define AD_CLIENT_DEBUG_IMPRESSION_ANGLE		0.57f
#define AD_CLIENT_DEBUG_IMPRESSION_SIZE_RATIO	11

//----------------------------------------------------------------------------
//
// TYPEDEFS
//
//----------------------------------------------------------------------------

typedef BOOL (*AD_DOWNLOADED_CALLBACK)(int nAdInstance, const char * pAdData, unsigned int nDataLengthBytes, BOOL & bNonReporting);
typedef void (*AD_CALCULATE_IMPRESSION_CALLBACK)(int nAdInstance, struct AD_CLIENT_IMPRESSION & tImpression);
typedef int  (*PFN_AD_CLIENT_ADD_AD)(const char * pszName, AD_DOWNLOADED_CALLBACK pfnDownloadCallback, AD_CALCULATE_IMPRESSION_CALLBACK pfnImpressionCallback);

//----------------------------------------------------------------------------
//
// ENUMS
//
//----------------------------------------------------------------------------

enum AD_OBJECT_RESOURCE_TYPE
{
	AORTYPE_INVALID = -1,
	AORTYPE_2D_TEXTURE = 0,
	//
	NUM_AD_OBJECT_RESOURCE_TYPES
};

//----------------------------------------------------------------------------
//
// STRUCTS
//
//----------------------------------------------------------------------------

struct AD_CLIENT_IMPRESSION
{
	BOOL			bInView;
	unsigned int	nSize;
	float			fAngle;
};

struct AD_INSTANCE_ENGINE_DATA
{
	int							nId;
	AD_INSTANCE_ENGINE_DATA *	pNext;

	int							nTextureID;
	int							nModelID;
	int							nAdObjectDefIndex;		// index into the modeldef ad object definition arrays

	ORIENTED_BOUNDING_BOX		tOBBw;
	VECTOR						vCenterWorld;
	VECTOR						vNormalWorld;
	VECTOR *					pvSamples;				// grid of additional impression sample points
	int							nSamples;

#if ISVERSION(DEVELOPMENT)
	unsigned int				nDebugDiagPoints[2];
	unsigned int				nDebugSize;
	float						fDebugAngle;
#endif
};

enum AD_OBJECT_DEF_FLAGS
{
	AODF_NONREPORTING_BIT,
};

struct AD_OBJECT_DEFINITION
{
	char						szName[ MAX_ADOBJECT_NAME_LEN ];
	int							nIndex;					// index into the model definition AOD list
	int							nMeshIndex;
	AD_OBJECT_RESOURCE_TYPE		eResType;
	DWORD						dwFlags;

	VECTOR						vCenter_Obj;
	VECTOR						vNormal_Obj;
	BOUNDING_BOX				tAABB_Obj;
};

//----------------------------------------------------------------------------
//
// INLINE
//
//----------------------------------------------------------------------------

inline AD_INSTANCE_ENGINE_DATA * e_AdInstanceGet( int nID )
{
#ifdef AD_CLIENT_ENABLED
	extern CHash<AD_INSTANCE_ENGINE_DATA> gtAdData;
	return HashGet( gtAdData, nID );
#else
	REF( nID );
	return NULL;
#endif
}

//----------------------------------------------------------------------------
//
// HEADERS
//
//----------------------------------------------------------------------------

void e_AdClientInit(
	void );

void e_AdClientDestroy(
	void );

int e_AdClientAddInstance(
	const char * pszName,
	int nModelID,
	int nAdObjectDefIndex );

PRESULT e_AdInstanceFreeAd (
	int nInstanceID );

BOOL e_AdInstanceDownloaded(
	int nAdInstance,
	const char * pAdData,
	unsigned int nDataLengthBytes,
	BOOL & bNonReporting );

BOOL e_AdInstanceDownloadedSetTexture(
	AD_INSTANCE_ENGINE_DATA * pData,
	int nTextureID );

void e_AdInstanceCalculateImpression(
	int nAdInstance,
	AD_CLIENT_IMPRESSION & tImpression );

PRESULT e_AdInstanceSetModelData(
	AD_INSTANCE_ENGINE_DATA * pData );

PRESULT e_AdInstanceUpdateTransform(
	int nInstance,
	AD_OBJECT_DEFINITION * pAdObjDef,
	MATRIX * pmWorld );


#if ISVERSION(DEVELOPMENT)
PRESULT e_AdInstanceDebugRenderInfo(
	int nInstance );
PRESULT e_AdInstanceDebugRenderAll();
#endif // DEVELOPMENT

#endif // _E_ADCLIENT_H_