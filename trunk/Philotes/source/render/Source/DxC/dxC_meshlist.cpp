//----------------------------------------------------------------------------
// dx9_meshlist.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_model.h"
#include "dxC_meshlist.h"
#include "memoryallocator_stl.h"
#include <map>

using namespace std;
using namespace FSCommon;

namespace FSSE
{

//----------------------------------------------------------------------------
typedef map<DWORD, ModelDraw, less<DWORD>, CMemoryAllocatorSTL<pair<DWORD, ModelDraw> > > MODEL_DRAW_TABLE;
typedef MODEL_DRAW_TABLE::iterator MODEL_DRAW_TABLE_ITR;

MeshDrawBucket *	g_MeshDrawBucket	= NULL;
MeshListVector *	g_MeshLists			= NULL;

//----------------------------------------------------------------------------

PRESULT dxC_MeshListsInit()
{

	ASSERT_RETFAIL( NULL == g_MeshDrawBucket );
	g_MeshDrawBucket	= MALLOC_NEW( NULL, MeshDrawBucket );
	g_MeshDrawBucket->Init();
	ASSERT_RETVAL( g_MeshDrawBucket, E_OUTOFMEMORY );

	ASSERT_RETFAIL( NULL == g_MeshLists );
	g_MeshLists			= MALLOC_NEW( NULL, MeshListVector );
	ASSERT_RETVAL( g_MeshLists, E_OUTOFMEMORY );

	return S_OK;
}

PRESULT dxC_MeshListsDestroy()
{
	if ( g_MeshDrawBucket )
	{
		g_MeshDrawBucket->Destroy( NULL );
		FREE_DELETE( NULL, g_MeshDrawBucket,	MeshDrawBucket );
		g_MeshDrawBucket = NULL;
	}
	if ( g_MeshLists )
	{
		FREE_DELETE( NULL, g_MeshLists,			MeshListVector );
		g_MeshLists = NULL;
	}

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT dxC_MeshListsClear()
{
	if ( g_MeshDrawBucket )
		g_MeshDrawBucket->Clear();
	if ( g_MeshLists )
		g_MeshLists->clear();

	return S_OK;
}


//----------------------------------------------------------------------------

PRESULT dxC_MeshListBeginNew( int &nID, MeshList *& pMeshList )
{
	ASSERT_RETFAIL( g_MeshLists );

//	if ( gnMeshListNextID < 0 )
//		gnMeshListNextID = 0;
//	nID = gnMeshListNextID++;
//
//	DWORD dwMeshListID = static_cast<DWORD>(nID);
//
//#if ISVERSION(DEVELOPMENT) && defined(_DEBUG)
//	MeshListMap::iterator iMeshList = g_MeshLists->find( dwMeshListID );
//	ASSERTV_RETVAL( iMeshList == g_MeshLists->end(), S_FALSE, "Existing mesh list found with this ID (%d)!", nID );
//#endif
//
//	pMeshList = &(*g_MeshLists)[ dwMeshListID ];
//	pMeshList->clear();

	MeshList tMeshList;

	g_MeshLists->push_back( tMeshList );
	nID = SIZE_TO_INT(g_MeshLists->size() - 1);
	pMeshList = &(*g_MeshLists)[ nID ];
	pMeshList->clear();

	return S_OK;
}

PRESULT dxC_MeshListAddMesh( MeshList & tMeshList, MeshDraw & tMesh )
{
	ASSERT_RETFAIL( g_MeshDrawBucket );

	BOOL bSuccess = g_MeshDrawBucket->PListPushTail( tMesh );
	ASSERTX_RETFAIL( bSuccess, "Failed to push MeshDraw onto tail of MeshDrawBucket!" );

	MeshDrawSortable tSortable;

	tSortable.pMeshDraw		= &g_MeshDrawBucket->GetPrev( NULL )->Value;		// Get the tail of the list
	ASSERT_RETFAIL( tSortable.pMeshDraw );
	tSortable.fDist			= tMesh.fDist;
	tSortable.pTechnique	= tMesh.pTechnique;

	if ( ! dxC_IsPixomaticActive() )
	{
		ASSERT_RETFAIL( tSortable.pTechnique );
	}

	tMeshList.push_back( tSortable );

	return S_OK;
}

PRESULT dxC_MeshListGet( int nMeshListID, MeshList *& pMeshList )
{
	ASSERT_RETFAIL( g_MeshLists );
	ASSERT_RETINVALIDARG( nMeshListID >= 0 );

//	DWORD dwMeshListID = static_cast<DWORD>(nMeshListID);
//
//#if ISVERSION(DEVELOPMENT) && defined(_DEBUG)
//	MeshListVector::iterator iMeshList = g_MeshLists->find( dwMeshListID );
//	ASSERTV_RETVAL( iMeshList != g_MeshLists->end(), S_FALSE, "Existing mesh list not found with this ID (%d)!", nMeshListID );
//#endif

	pMeshList = &(*g_MeshLists)[ nMeshListID ];
	return S_OK;
}


}; // namespace FSSE
