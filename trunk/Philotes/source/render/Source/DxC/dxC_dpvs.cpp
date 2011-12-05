//----------------------------------------------------------------------------
// dxC_dpvs.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_model.h"
#include "dxC_texture.h"
#include "dxC_target.h"
#include "dxC_state.h"
#include "dxC_portal.h"
#include "dxC_renderlist.h"
#include "dxC_viewer.h"
#include "e_screenshot.h"

#include "dxC_dpvs.h"

using namespace DPVS;
using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//#define ENABLE_DPVS_VIRTUAL_PORTALS

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

int gnDPVSDebugTextureID = INVALID_ID;

#ifdef DPVS_DEBUG_OCCLUDER
	Vector3 *  gpDPVSDebugVerts = NULL;
	Vector3i * gpDPVSDebugTris  = NULL;
	int gnDPVSDebugTris = 0;
#endif

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#ifdef ENABLE_DPVS
void DPVSCommander::command(Command command)
{
	FSSE::Viewer * pViewer = dxC_ViewerGetCurrent_Private();

	switch (command)
    {
    case QUERY_BEGIN:
	{
		ASSERT_BREAK( pViewer );
		V( pViewer->tCommands.Clear() );
		pViewer->nCurEnvironmentDef = e_GetCurrentEnvironmentDefID();
		//pViewer->nCurRegion = e_GetCurrentRegionID();
		// force next view parameters changed to go through
		pViewer->nCurViewParams = -1;
	}
	break;

    case QUERY_END:
	{
		ASSERT_BREAK( pViewer );
		V( pViewer->tCommands.Commit() );
	}
    break;

    case PORTAL_ENTER:
    {
		ASSERT_BREAK( pViewer );
		DPVSTRACE( "Cmd: PORTAL_ENTER" );
		pViewer->nCurSection++;

		const Instance * pInstance = getInstance();
		ASSERT_BREAK( pInstance );
		const Object * pObject = pInstance->getObject();
		ASSERT_BREAK( pObject );
		const Cell * pCell = pObject->getCell();
		ASSERT_BREAK( pCell );


		int nRegion = (int)pCell->getUserPointer();
		const REGION * pRegion = e_RegionGet( nRegion );
		ASSERT_BREAK( pRegion );

		// force next view parameters changed to go through
		pViewer->nCurViewParams = -1;

		pViewer->nCurEnvironmentDef = pRegion->nEnvironmentDef;
		//pViewer->nCurRegion = nRegion;

		// check if it exists (asserts internally)
		void * pDef = DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pViewer->nCurEnvironmentDef );
    }
    break;

    case PORTAL_EXIT:
    {
		ASSERT_BREAK( pViewer );
		DPVSTRACE( "Cmd: PORTAL_EXIT" );
		pViewer->nCurSection++;
		V( dxC_ViewParametersStackPop() );

		const Instance * pInstance = getInstance();
		ASSERT_BREAK( pInstance );
		const Object * pObject = pInstance->getObject();
		ASSERT_BREAK( pObject );
		const Cell * pCell = pObject->getCell();
		ASSERT_BREAK( pCell );

		int nRegion = (int)pCell->getUserPointer();
		const REGION * pRegion = e_RegionGet( nRegion );
		ASSERT_BREAK( pRegion );

		// force next view parameters changed to go through
		pViewer->nCurViewParams = -1;

		pViewer->nCurEnvironmentDef = pRegion->nEnvironmentDef;
		//pViewer->nCurRegion = nRegion;

		// check if it exists (asserts internally)
		void * pDef = DefinitionGetById( DEFINITION_GROUP_ENVIRONMENT, pViewer->nCurEnvironmentDef );
	}
    break;

    case VIEW_PARAMETERS_CHANGED:
    {
		ASSERT_BREAK( pViewer );

        // View parameters have changed. This is signaled at
        // the beginning of the query and each time the query
        // passes through a portal.
		//DPVSTRACE( "Cmd: VIEW_PARAMETERS_CHANGED" );


		// VIEW_PARAMETERS_CHANGED is signaled for physical portals, too, but we only care about it after virtual portals.
		if ( pViewer->nCurViewParams != -1 )
			break;


		int nViewParams;
		nViewParams = INVALID_ID;
		V_BREAK( dxC_ViewParametersStackPush( nViewParams ) );
		ViewParameters * pViewParams = NULL;
		V_BREAK( dxC_ViewParametersGet( nViewParams, pViewParams ) );

		pViewer->nCurViewParams = nViewParams;

        const Viewer* viewer = getViewer(); 
        ASSERT_BREAK(viewer);


		// Environment
		SETBIT( pViewParams->dwFlagBits, ViewParameters::ENV_DEF, TRUE );
		pViewParams->nEnvDefID = pViewer->nCurEnvironmentDef;
		ASSERT( pViewParams->nEnvDefID != INVALID_ID );


		// Projection matrix
        //MATRIX matrix;
        //viewer->getProjectionMatrix(matrix, Viewer::LEFT_HANDED);
		//pViewParams->dwFlags |= ViewParameters::PROJ_MATRIX;
		//pViewParams->mProj = *(MATRIX*)&matrix;
		//e_ScreenShotGetProjectionOverride( &pViewParams->mProj, pViewer->tCameraInfo.fVerticalFOV, e_GetNearClipPlane(), e_GetFarClipPlane() );


		// View matrix
		//viewer->getCameraToWorldMatrix(*(Matrix4x4*)&matrix);
		//MatrixInverse( &pViewParams->mView, (const MATRIX*)&matrix );
		//SETBIT( pViewParams->dwFlagBits, ViewParameters::VIEW_MATRIX, TRUE );


		// Eye position
		//pViewParams->vEyePos = *(VECTOR*)&pViewParams->mView._41;
		//SETBIT( pViewParams->dwFlagBits, ViewParameters::EYE_POS, TRUE );


		//Frustum tFrustum;
		//viewer->getFrustum( tFrustum );
		//keytrace( "### %6.5f\n", tFrustum.zNear );


		//Vector4 plane;
		//viewer->getFrustumPlane( Instance::BACK, plane);
		//PLANE tPlane_w;
		//PlaneNormalize( (PLANE*)&plane, (const PLANE*)&plane );
		//MATRIX mView, mViewTran;
		//MatrixInverse( &mView, &matrix );
		//VECTOR vEyePos;
		//vEyePos = *(VECTOR*)&mView._41;
		//MatrixTranspose( &mViewTran, &mView );
		//PlaneTransform( &tPlane_w, (const PLANE*)&plane, &mViewTran );
		//VECTOR vPoP;
		//NearestPointOnPlane( &tPlane_w, &vEyePos, &vPoP );
		//float fDist = VectorDistanceSquared( vEyePos, vPoP );
		//fDist = SQRT_SAFE(fDist);



		// Scissor rect
		int left,right,top,bottom;
		viewer->getScissor(left,top,right,bottom);
		SETBIT( pViewParams->dwFlagBits, ViewParameters::SCISSOR_RECT, TRUE );
		pViewParams->tScissor.Set( left, top, right, bottom );


		// Inverted culling
        if ( viewer->isMirrored() )
			SETBIT( pViewParams->dwFlagBits, ViewParameters::INVERT_CULL, TRUE );


		// Clip plane
        if (viewer->getFrustumPlaneCount() > 6)
        {
            Vector4 plane;
            viewer->getFrustumPlane(6, plane);

			SETBIT( pViewParams->dwFlagBits, ViewParameters::CLIP_PLANE, TRUE );
			pViewParams->tClip = *(PLANE*)&plane;
        }
    }
    break;

    case INSTANCE_VISIBLE:
    {
		ASSERT_BREAK( pViewer );
		int nViewParams = pViewer->nCurViewParams;
		ASSERT( pViewer->nCurViewParams != INVALID_ID );
		//V_BREAK( dxC_ViewParametersGetCurrent_Private( nViewParams ) );

		const Instance* instance = getInstance();
        ASSERT_BREAK(instance);

        Object* object = instance->getObject();
        ASSERT_BREAK(object);

		const Viewer* viewer = getViewer();
		ASSERT( viewer );
		int left,right,top,bottom;
		viewer->getScissor(left,top,right,bottom);
		E_RECT tScissor;
		tScissor.Set( left, top, right, bottom );

        D3D_MODEL * pModel = (D3D_MODEL*)object->getUserPointer();
		ASSERT_BREAK( pModel );

		V_BREAK( dxC_ModelLazyUpdate( pModel ) );

		// add as a visible model for the later traversal & split
		V( dxC_ViewerAddModelToVisibleList_Private( pViewer, pModel, &tScissor, nViewParams ) );
    }
    break;

    case INSTANCE_IMMEDIATE_REPORT:
    {
        // An object is signaled as visible (during the query). 
        // Here we may want to change the write model of the 
        // object (in case we're using a LOD scheme or 
        // something).
		DPVSTRACE( "Cmd: INSTANCE_IMMEDIATE_REPORT" );

        const Instance* instance = getInstance();
        ASSERT_BREAK(instance);

        Object* object = instance->getObject();
        ASSERT_BREAK(object);

        // object->setWriteModel(foo);

    }
    break;

    case REMOVAL_SUGGESTED:
    {
        // This event can only happen if we have called
        // Library::suggestGarbageCollect().
		DPVSTRACE( "Cmd: REMOVAL_SUGGESTED" );

        const Instance* instance = getInstance();
        ASSERT_BREAK(instance);

        Object* object = instance->getObject();
        ASSERT_BREAK(object);

        // maybe delete the object here?
        // object->release();
    }
    break;

    case REGION_OF_INFLUENCE_ACTIVE:
    {
		DPVSTRACE( "Cmd: REGION_OF_INFLUENCE_ACTIVE" );

        //const Instance* instance = getInstance();
        //ASSERT_BREAK(instance);

        //Object* object = instance->getObject();
        //ASSERT_BREAK(object);

        // 'object' points now to the RegionOfInfluence.
        // get user point and turn the light on..

		pViewer->tCurNoScissor.AddRef();
	}
    break;

    case REGION_OF_INFLUENCE_INACTIVE:
    {
        // A previously activated light source has become
        // inactive. Disable it in your rendering engine.
		DPVSTRACE( "Cmd: REGION_OF_INFLUENCE_INACTIVE" );

        //const Instance* instance = getInstance();
        //assert (instance);

        //Object* object = instance->getObject();
        //assert (object);

        // 'object' points now to the RegionOfInfluence.
        // get user point and turn the light off.

		pViewer->tCurNoScissor.Release();
	}
    break;

    case STENCIL_VALUES_CHANGED:
    {
        // Stencil test/write values have changed. This
        // event can only happen when using floating
        // portals.
		DPVSTRACE( "Cmd: STENCIL_VALUES_CHANGED" );

        int test,write;

        getStencilValues(test,write);

        // here apply the new test / write values to the
        // rendering engine.
    }
    break;

    case STENCIL_MASK:
		DPVSTRACE( "Cmd: STENCIL_MASK" );

    // Called whenever there are changes to be made to the 
    // stencil buffer. The commands are either INCREMENT or 
    // DECREMENT commands. In absence of stencil buffer,
    // a destination alpha test can be used for performing 
    // this action.

    // Recommended action:
    // -------------------

    // Step 1:
    //      - disable all color channels. 
    //        (glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE))
    //      - getStencilValues(). INC = true, if write>test
    //      - enable stencil test (glEnable(GL_STENCIL_TEST))
    //      - set stencil func (glStencilFunc(GL_EQUAL, test, 0xff))
    //      - if(INC) glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    //        else    glStencilOp(GL_KEEP, GL_DECR, GL_DECR);
    //      - optionally force polygon rendering mode to filled
    //      - render instance
    // 
    //  Step 2 (only INC):
    //      - set depth value to far plane  
    //      - disable depth test            
    //      - set stencil function          
    //      - disable stencil operations    
    //      - enable all color channels     

    //  glDepthRange(1,1);
    //  glDepthFunc(GL_ALWAYS);
    //  glStencilFunc(GL_EQUAL, write, 0xff);
    //  glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
    //  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // 
    //  Step 3:
    //      - enable all color channels     
    //      - restore depth func            
    //      - restore depth range           
    //      - set stencil func              
    //      - disable stencil operations    
    //      - optionally restore polygon fill mode (wireframe)
    // 
    //  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    //  glDepthFunc(GL_LEQUAL);
    //  glDepthRange(0,1);
    //  glStencilFunc(GL_EQUAL, write, 0xff);
    //  glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

    //  Considerations:
    //  ---------------

    //  - additional clip plane has been enabled/disabled in 
    //    VIEW_PARAMETERS_CHANGED
    //  - clears color buffer in INC mode
    //  - leaves OpenGL color/depth states intact
    //  - adjusts OpenGL stencil state for object rendering
    // 
    //  Valid methods:
    //  --------------
    //      - getInstance(), do not use getClipMask() here
    //      - getStencilValues()

    break;

    case TEXT_MESSAGE:
    {
        // [debug information] dPVS has a text message to the user. 
        const char* s = getTextMessage();
        if (s)
		{
			DPVSTRACE( "text message: %s", s );
		}
    }
    break;

    case DRAW_LINE_2D:
    {
        Vector2 start,end;
        Vector4 color;

        Library::LineType type = getLine2D(start,end,color);

		e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, (const VECTOR2*)&start, (const VECTOR2*)&end, ARGB_MAKE_FROM_FLOAT( color.v[0], color.v[1], color.v[2], color.v[3] ) );
    }
    break;

    case DRAW_LINE_3D:
    {   
        Vector3 start,end;
        Vector4 color;

        Library::LineType type = getLine3D(start,end,color);

		e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, (const VECTOR3*)&start, (const VECTOR3*)&end, ARGB_MAKE_FROM_FLOAT( color.v[0], color.v[1], color.v[2], color.v[3] ) );
    }
    break;

    case DRAW_BUFFER:
    {
        const unsigned char* data;
        int                  width,height;

        Library::BufferType type = getBuffer(data,width,height);

		e_DPVS_DebugUpdateTexture( data, width, height );
    }
    break;

    default:

        // In debug build, perform an assertion
		DPVSTRACE( "ERROR: unrecognized command enum: %d", command );
		assert (!"Unrecognized command enumeration");
        break;
    }
}
#endif // ENABLE_DPVS





//----------------------------------------------------------------------------
// DEBUG-ONLY commander
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#ifdef ENABLE_DPVS
void DPVSDebugCommander::command(Command command)
{
	switch (command)
	{
	case QUERY_BEGIN:
	case QUERY_END:
	case VIEW_PARAMETERS_CHANGED:
		break;

	case TEXT_MESSAGE:
		{
			// [debug information] dPVS has a text message to the user. 
			const char* s = getTextMessage();
			if (s)
			{
				trace( "dPVS text message: %s", s );
			}
		}
		break;

	case DRAW_LINE_2D:
		{
			Vector2 start,end;
			Vector4 color;

			Library::LineType type = getLine2D(start,end,color);

			e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, (const VECTOR2*)&start, (const VECTOR2*)&end, ARGB_MAKE_FROM_FLOAT( color.v[0], color.v[1], color.v[2], color.v[3] ) );
		}
		break;

	case DRAW_LINE_3D:
		{   
			Vector3 start,end;
			Vector4 color;

			Library::LineType type = getLine3D(start,end,color);

			e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, (const VECTOR3*)&start, (const VECTOR3*)&end, ARGB_MAKE_FROM_FLOAT( color.v[0], color.v[1], color.v[2], color.v[3] ) );
		}
		break;

	case DRAW_BUFFER:
		{
			const unsigned char* data;
			int                  width,height;

			Library::BufferType type = getBuffer(data,width,height);

			e_DPVS_DebugUpdateTexture( data, width, height );
		}
		break;

	default:

		// In debug build, perform an assertion
		DPVSTRACE( "ERROR: unrecognized command enum: %d", command );
		assert (!"Unrecognized command enumeration");
		break;
	}
}
#endif // ENABLE_DPVS



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CellSetWorldMatrix( CULLING_CELL & tCell, const MATRIX & mWorld )
{
#ifdef ENABLE_DPVS
	if ( ! tCell.pInternalCell )
		return S_FALSE;

	//MATRIX mWorldInv;
	tCell.mWorld = mWorld;
	//MatrixInverse( &mWorldInv, &tCell.mWorld );

	// ensure that the fourth column is proper identity (avoid rounding error)
	tCell.mWorld.m[0][3] = 0.f;
	tCell.mWorld.m[1][3] = 0.f;
	tCell.mWorld.m[2][3] = 0.f;
	tCell.mWorld.m[3][3] = 1.f;

	tCell.pInternalCell->setCellToWorldMatrix( (const Matrix4x4&)tCell.mWorld );
#endif // ENABLE_DPVS

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CellSetRegion( CULLING_CELL & tCell, int nRegion )
{
#ifdef ENABLE_DPVS
	if ( ! tCell.pInternalCell )
		return S_FALSE;

	ASSERT( nRegion != INVALID_ID );

	tCell.nRegion = nRegion;
	tCell.pInternalCell->setUserPointer( (void*)tCell.nRegion );
#endif // ENABLE_DPVS

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CreateCullingCell( CULLING_CELL & tCell )
{
	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS
	ASSERT_RETINVALIDARG( NULL == tCell.pInternalCell );

	tCell.pInternalCell = (LP_INTERNAL_CELL)Cell::create();
	ASSERT_RETFAIL( tCell.pInternalCell );

	e_DPVS_CellSetWorldMatrix( tCell, tCell.mWorld );
	e_DPVS_CellSetRegion( tCell, tCell.nRegion );
#endif // ENABLE_DPVS
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetNearestCornerMappings(
	const VECTOR * pvCorners1,
	const VECTOR * pvCorners2,
	int nCornerCount,
	int * pnMapping )
{
	ASSERT_RETINVALIDARG( pvCorners1 );
	ASSERT_RETINVALIDARG( pvCorners2 );
	ASSERT_RETINVALIDARG( pnMapping );

	for ( int i = 0; i < nCornerCount; ++i )
		pnMapping[i] = -1;

	// this function assumes that the corners match pretty well to start with

	for ( int i = 0; i < nCornerCount; ++i )
	{
		float fMin = 0.f;
		int nMin = -1;
		for ( int j = 0; j < nCornerCount; ++j )
		{
			float fDist = VectorDistanceSquared( pvCorners1[i], pvCorners2[j] );
			if ( fDist >= fMin && nMin != -1 )
				continue;
			for ( int k = 0; k < nCornerCount; ++k )
			{
				// see if this corner was already "claimed"
				//if ( k == i )
				//	continue;
				if ( pnMapping[k] == j )
					continue;
			}
			// found a potential match
			fMin = fDist;
			nMin = j;
		}
		ASSERT( nMin != INVALID_ID );
		pnMapping[i] = nMin;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetOppositeCorner(
	const VECTOR * pvSrcCorners,
	int nCornerCount,
	int nFromIndex,
	int & nOppositeIndex
	)
{
	float fMax = 0.f;
	int nMax = -1;
	for ( int i = 0; i < nCornerCount; ++i )
	{
		if ( i == nFromIndex )
			continue;
		float fDist = VectorDistanceSquared( pvSrcCorners[nFromIndex], pvSrcCorners[i] );
		if ( fDist < fMax )
			continue;
		fMax = fDist;
		nMax = i;
	}

	nOppositeIndex = nMax;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetQuadWithCWWinding(
	const VECTOR * pvSrcCorners,
	int nCornerCount,
	const VECTOR & vNormal,
	int * pnInds,
	int & nPrimCount
	)
{
	// Desired order:
	//
	//  L--O
	//  | /|
	//  |/ |
	//  B--R

	const int CORNERS = 4;

	ASSERT_RETINVALIDARG( pvSrcCorners );
	ASSERTV_RETINVALIDARG( nCornerCount == CORNERS, "Only %d-vertex portal geometry supported!", CORNERS );
	ASSERT_RETINVALIDARG( pnInds );

	// find opposing verts
	int nBase = 0;
	int nOpp = -1;
	V_RETURN( sGetOppositeCorner( pvSrcCorners, nCornerCount, nBase, nOpp ) );
	ASSERT_RETFAIL( IS_VALID_INDEX( nOpp, CORNERS ) );


	// using normal as up vector, find which vert is on the right
	VECTOR N = vNormal;
	VectorNormalize( N );
	VECTOR BO = pvSrcCorners[nOpp] - pvSrcCorners[nBase];
	VectorNormalize( BO );
	VECTOR vRight;
	VectorCross( vRight, BO, N );
	VectorNormalize( vRight );
	int nRight = -1;
	int nLeft = -1;
	for ( int i = 0; i < CORNERS; ++i )
	{
		if ( i == nBase || i == nOpp )
			continue;
		VECTOR BV = pvSrcCorners[i] - pvSrcCorners[nBase];
		VectorNormalize( BV ); // shouldn't be necessary, but more readable in debugger
		float fDot = VectorDot( vRight, BV );
		if ( fDot < 0.f )
		{
			ASSERTX( nRight == -1, "Found a second corner on the right, while only one was expected." );
			nRight = i;
		} else if ( fDot > 0.f )
		{
			ASSERTX( nLeft == -1, "Found a second corner on the left, while only one was expected." );
			nLeft = i;
		} else
		{
			ASSERT_MSG( "Found a 3rd co-linear corner in the quad, while only two are expected." )
		}
	}
	ASSERT_RETFAIL( IS_VALID_INDEX( nRight, CORNERS ) );
	ASSERT_RETFAIL( IS_VALID_INDEX( nLeft, CORNERS ) );


	// lay out triangles using clockwise winding order
	pnInds[0] = nBase;
	pnInds[1] = nOpp;
	pnInds[2] = nRight;
	pnInds[3] = nOpp;
	pnInds[4] = nBase;
	pnInds[5] = nLeft;

	nPrimCount = 2;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CreateCullingPhysicalPortal(
	CULLING_PHYSICAL_PORTAL & tPortal,
	const VECTOR * pvBorders,
	int nBorderCount,
	const VECTOR & vNormal,
	CULLING_CELL* pFromCell,
	CULLING_CELL* pToCell,
	const MATRIX * pmWorldInv /*= NULL*/ )
{
	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS
	ASSERT_RETINVALIDARG( NULL == tPortal.pInternalPortal );
	ASSERT_RETINVALIDARG( pvBorders );
	ASSERT_RETINVALIDARG( nBorderCount > 0 );
	ASSERT_RETINVALIDARG( pFromCell );
	ASSERT_RETINVALIDARG( pFromCell->pInternalCell );

	ASSERTX_RETFAIL( nBorderCount == 4, "Only 4-vertex portal geometry supported!" );

	const float cfPushOutFudge = 1.0f;		// 2 cm

	LP_INTERNAL_CELL pToInternalCell = pToCell ? pToCell->pInternalCell : NULL;

	MATRIX mIdent;
	MatrixIdentity( &mIdent );
	if ( ! pmWorldInv )
		pmWorldInv = &mIdent;

	VECTOR vBorder[4];
	for ( int i = 0; i < nBorderCount; ++i )
	{
		MatrixMultiply( &vBorder[i], &pvBorders[i], pmWorldInv );
	}
	VECTOR No;
	MatrixMultiplyNormal( &No, &vNormal, pmWorldInv );

	Vector3i vTris[2];
	int nTris = 0;

	V_RETURN( sGetQuadWithCWWinding(
		vBorder,
		4,
		No,
		(int*)vTris,
		nTris ) );


	//VECTOR vFudge = No * -cfPushOutFudge;
	//for ( int i = 0; i < nBorderCount; ++i )
	//	vBorder[i] += vFudge;


	Model * pCullTestModel = MeshModel::create( (const Vector3 *)vBorder, vTris, 4, nTris, true );

	//Model * pCullTestModel = OBBModel::create( (const Vector3&)tBBox.vMin, (const Vector3&)tBBox.vMax );
	ASSERT_RETFAIL( pCullTestModel );
	pCullTestModel->autoRelease();
	pCullTestModel->set( MeshModel::BACKFACE_CULLABLE, true );

	tPortal.pInternalPortal = (LP_INTERNAL_PHYSICAL_PORTAL)PhysicalPortal::create( pCullTestModel, pToInternalCell );
	ASSERT_RETFAIL( tPortal.pInternalPortal );

	tPortal.pInternalPortal->setCell( pFromCell->pInternalCell );
	tPortal.pInternalPortal->set( Object::ENABLED, true );
	//tPortal.pInternalPortal->set( Object::INFORM_PORTAL_ENTER, true );
	//tPortal.pInternalPortal->set( Object::INFORM_PORTAL_EXIT, true );
	tPortal.pInternalPortal->setObjectToCellMatrix( (Matrix4x4&)mIdent );

	VECTOR vCenter(0,0,0);
	for ( int i = 0; i < nBorderCount; ++i )
	{
		vCenter += vBorder[i];
	}
	vCenter /= (float)nBorderCount;
	float fRad = 0.f;
	for ( int i = 0; i < nBorderCount; ++i )
	{
		float fTest = VectorDistanceSquared( vCenter, vBorder[i] );
		fRad = MAX( fRad, fTest );
	}
	fRad = SQRT_SAFE( fRad );
	// safety fudge
	fRad *= 2.f;

	Model * pPortalRegionModel = SphereModel::create( (Vector3&)vCenter, fRad );
	ASSERT_RETFAIL( pPortalRegionModel );

#if ISVERSION(DEBUG_VERSION)
	char szName[256];
	PStrPrintf( szName, 256, "%s-%d", __FILE__, __LINE__ );
	pPortalRegionModel->setName( szName );
#endif

	pPortalRegionModel->autoRelease();
	RegionOfInfluence * pRoI = RegionOfInfluence::create( pPortalRegionModel );
	ASSERT_RETFAIL( pRoI );
	pRoI->autoRelease();
	pRoI->set( RegionOfInfluence::ENABLED, true );
	pRoI->setCell( pFromCell->pInternalCell );
	pRoI->setObjectToCellMatrix( (Matrix4x4&)mIdent );

#endif // ENABLE_DPVS
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CreateCullingPhysicalPortalPair(
	CULLING_PHYSICAL_PORTAL & tPortal1,
	CULLING_PHYSICAL_PORTAL & tPortal2,
	const VECTOR * pvBorders1,
	const VECTOR * pvBorders2,
	int nBorderCount,
	const VECTOR & vNormal1,
	const VECTOR & vNormal2,
	CULLING_CELL* pFromCell,
	CULLING_CELL* pToCell,
	const MATRIX * pmWorldInv1 /*= NULL*/,
	const MATRIX * pmWorldInv2 /*= NULL*/ )
{
	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS
	ASSERT_RETINVALIDARG( NULL == tPortal1.pInternalPortal );
	ASSERT_RETINVALIDARG( NULL == tPortal2.pInternalPortal );
	ASSERT_RETINVALIDARG( pvBorders1 );
	ASSERT_RETINVALIDARG( pvBorders2 );
	ASSERT_RETINVALIDARG( nBorderCount > 0 );
	ASSERT_RETINVALIDARG( pFromCell );
	ASSERT_RETINVALIDARG( pFromCell->pInternalCell );
	ASSERT_RETINVALIDARG( pToCell );
	ASSERT_RETINVALIDARG( pToCell->pInternalCell );

	ASSERTX_RETFAIL( nBorderCount == 4, "Only 4-vertex portal geometry supported!" );

	CULLING_PHYSICAL_PORTAL * ptPortal[2] = { &tPortal1, &tPortal2 };
	const VECTOR * pvBorders[2] = { pvBorders1, pvBorders2 };
	const VECTOR * pvNormal[2] = { &vNormal1, &vNormal2 };
	const MATRIX * pmWorldInv[2] = { pmWorldInv1, pmWorldInv2 };

	const float cfPushOutFudge = 1.0f;		// 2 cm

	//LP_INTERNAL_CELL pToInternalCell = pToCell ? pToCell->pInternalCell : NULL;
	LP_INTERNAL_CELL pFromInternalCells[2]   = { pFromCell->pInternalCell, pToCell->pInternalCell };
	LP_INTERNAL_CELL pTargetInternalCells[2] = { pToCell->pInternalCell, pFromCell->pInternalCell };

	MATRIX mIdent;
	MatrixIdentity( &mIdent );
	for ( int i = 0; i < 2; ++i )
		if ( ! pmWorldInv[i] )
			pmWorldInv[i] = &mIdent;

	VECTOR vBorder[2][4];
	VECTOR No[2];
	for ( int n = 0; n < 2; ++n )
	{
		for ( int i = 0; i < nBorderCount; ++i )
			MatrixMultiply( &vBorder[n][i], &pvBorders[n][i], pmWorldInv[n] );
		MatrixMultiplyNormal( &No[n], pvNormal[n], pmWorldInv[n] );
	}

	int nMapping[ 4 ];
	V( sGetNearestCornerMappings( vBorder[0], vBorder[1], 4, nMapping ) );

	// find average of both borders
	VECTOR vFinalBorder[4];
	for ( int i = 0; i < nBorderCount; ++i )
	{
		ASSERT_CONTINUE( IS_VALID_INDEX( nMapping[i], nBorderCount ) );
		vFinalBorder[i] = ( vBorder[0][i] + vBorder[1][nMapping[i]] ) * 0.5f;
	}

	Vector3i vTris[2][2];
	int nTris = 0;

	for ( int i = 0; i < 2; ++i )
	{
		V_RETURN( sGetQuadWithCWWinding(
			vFinalBorder,
			4,
			No[i],
			(int*)vTris[i],
			nTris ) );
	}


	//VECTOR vFudge = No * -cfPushOutFudge;
	//for ( int i = 0; i < nBorderCount; ++i )
	//	vBorder[i] += vFudge;


	for ( int i = 0; i < 2; ++i )
	{
		Model * pCullTestModel = MeshModel::create( (const Vector3 *)vFinalBorder, vTris[i], 4, nTris, true );
		ASSERT_RETFAIL( pCullTestModel );
		pCullTestModel->set( MeshModel::BACKFACE_CULLABLE, true );

		ptPortal[i]->pInternalPortal = (LP_INTERNAL_PHYSICAL_PORTAL)PhysicalPortal::create( pCullTestModel, pTargetInternalCells[i] );
		ASSERT_RETFAIL( ptPortal[i]->pInternalPortal );

		pCullTestModel->autoRelease();
		ptPortal[i]->pInternalPortal->setCell( pFromInternalCells[i] );
		ptPortal[i]->pInternalPortal->set( Object::ENABLED, true );
		//ptPortal[i]->pInternalPortal->set( Object::INFORM_PORTAL_ENTER, true );
		//ptPortal[i]->pInternalPortal->set( Object::INFORM_PORTAL_EXIT, true );
		ptPortal[i]->pInternalPortal->setObjectToCellMatrix( (Matrix4x4&)mIdent );
	}

	VECTOR vCenter(0,0,0);
	for ( int i = 0; i < nBorderCount; ++i )
	{
		vCenter += vFinalBorder[i];
	}
	vCenter /= (float)nBorderCount;

	float fRad = 0.f;
	for ( int i = 0; i < nBorderCount; ++i )
	{
		float fTest = VectorDistanceSquared( vCenter, vFinalBorder[i] );
		fRad = MAX( fRad, fTest );
	}

	// Construct plane equation of the portal surface.
	for ( int i = 0; i < 2; ++i )
	{
		PlaneFromPointNormal( &ptPortal[i]->tPlane, &vCenter, &No[i] );
		ptPortal[i]->vCenter = vCenter;
		ptPortal[i]->fRadiusSq = fRad * 1.1f; // smaller safety fudge
	}

	// bigger safety fudge
	fRad = SQRT_SAFE( fRad );
	fRad *= 2.f;

	for ( int i = 0; i < 2; ++i )
	{
		Model * pPortalRegionModel = SphereModel::create( (Vector3&)vCenter, fRad );
		ASSERT_RETFAIL( pPortalRegionModel );

#if ISVERSION(DEBUG_VERSION)
		char szName[256];
		PStrPrintf( szName, 256, "%s-%d", __FILE__, __LINE__ );
		pPortalRegionModel->setName( szName );
#endif

		RegionOfInfluence * pRoI = RegionOfInfluence::create( pPortalRegionModel );
		ASSERT_RETFAIL( pRoI );
		pPortalRegionModel->autoRelease();
		pRoI->autoRelease();
		pRoI->set( RegionOfInfluence::ENABLED, true );
		pRoI->setCell( pFromInternalCells[i] );
		pRoI->setObjectToCellMatrix( (Matrix4x4&)mIdent );
	}

#endif // ENABLE_DPVS
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_VirtualPortalSetWarpMatrix(
	CULLING_VIRTUAL_PORTAL & tFromPortal )
{
	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS
#ifdef ENABLE_DPVS_VIRTUAL_PORTALS
	ASSERT_RETFAIL( tFromPortal.pInternalPortal );

	MATRIX mWarp;
	mWarp.Identity();

	VECTOR vWorldX(1,0,0);
	VECTOR vWorldY(0,1,0);
	VECTOR vWorldZ(0,0,1);
	VECTOR vForward = tFromPortal.tPlane.N();
	VECTOR vRight;
	VectorCross( vRight, vWorldZ, vForward );
	VectorNormalize( vRight );
	VECTOR vUp;
	VectorCross( vUp, vForward, vRight );
	VectorNormalize( vUp );

	mWarp.SetRight( vRight );
	mWarp.SetForward( vUp );
	mWarp.SetUp( vForward );
	mWarp.SetTranslate( tFromPortal.vCenter );

	tFromPortal.pInternalPortal->setWarpMatrix( (const Matrix4x4&) mWarp );

#endif // ENABLE_DPVS_VIRTUAL_PORTALS
#endif // ENABLE_DPVS
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_VirtualPortalSetTarget(
	CULLING_VIRTUAL_PORTAL & tFromPortal,
	CULLING_VIRTUAL_PORTAL & tToPortal )
{
	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS
#ifdef ENABLE_DPVS_VIRTUAL_PORTALS
	ASSERT_RETFAIL( tFromPortal.pInternalPortal );

	tFromPortal.pInternalPortal->setTargetPortal( tToPortal.pInternalPortal );

#endif // ENABLE_DPVS_VIRTUAL_PORTALS
#endif // ENABLE_DPVS
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_CreateCullingVirtualPortal(
	CULLING_VIRTUAL_PORTAL & tPortal,
	const VECTOR * pvBorders,
	int nBorderCount,
	const VECTOR & vNormal,
	CULLING_CELL* pFromCell,
	const MATRIX * pmWorldInv /*= NULL*/ )
{
	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS
#ifdef ENABLE_DPVS_VIRTUAL_PORTALS
	ASSERT_RETINVALIDARG( NULL == tPortal.pInternalPortal );
	ASSERT_RETINVALIDARG( pvBorders );
	ASSERT_RETINVALIDARG( nBorderCount > 0 );
	ASSERT_RETINVALIDARG( pFromCell );
	ASSERT_RETINVALIDARG( pFromCell->pInternalCell );

	ASSERTX_RETFAIL( nBorderCount == 4, "Only 4-vertex portal geometry supported!" );

	const float cfPushOutFudge = 1.0f;		// 2 cm

	//LP_INTERNAL_CELL pToInternalCell = pToCell ? pToCell->pInternalCell : NULL;

	MATRIX mIdent;
	MatrixIdentity( &mIdent );
	if ( ! pmWorldInv )
		pmWorldInv = &mIdent;

	VECTOR vBorder[4];
	for ( int i = 0; i < nBorderCount; ++i )
	{
		MatrixMultiply( &vBorder[i], &pvBorders[i], pmWorldInv );
	}
	VECTOR No;
	MatrixMultiplyNormal( &No, &vNormal, pmWorldInv );

	Vector3i vTris[2];
	int nTris = 0;

	V_RETURN( sGetQuadWithCWWinding(
		vBorder,
		4,
		No,
		(int*)vTris,
		nTris ) );


	//VECTOR vFudge = No * -cfPushOutFudge;
	//for ( int i = 0; i < nBorderCount; ++i )
	//	vBorder[i] += vFudge;


	Model * pCullTestModel = MeshModel::create( (const Vector3 *)vBorder, vTris, 4, nTris, true );

	//Model * pCullTestModel = OBBModel::create( (const Vector3&)tBBox.vMin, (const Vector3&)tBBox.vMax );
	ASSERT_RETFAIL( pCullTestModel );
	pCullTestModel->autoRelease();
	pCullTestModel->set( MeshModel::BACKFACE_CULLABLE, true );

	tPortal.pInternalPortal = (LP_INTERNAL_VIRTUAL_PORTAL)VirtualPortal::create( pCullTestModel, NULL );
	ASSERT_RETFAIL( tPortal.pInternalPortal );

	tPortal.pInternalPortal->setCell( pFromCell->pInternalCell );
	tPortal.pInternalPortal->set( Object::ENABLED, true );
	tPortal.pInternalPortal->set( Object::INFORM_PORTAL_ENTER, true );
	tPortal.pInternalPortal->set( Object::INFORM_PORTAL_EXIT, true );
	//tPortal.pInternalPortal->set( Object::FLOATING_PORTAL, true );
	tPortal.pInternalPortal->setObjectToCellMatrix( (Matrix4x4&)mIdent );

	VECTOR vCenter(0,0,0);
	for ( int i = 0; i < nBorderCount; ++i )
	{
		vCenter += vBorder[i];
	}
	vCenter /= (float)nBorderCount;

	// Construct plane equation of the portal surface.
	PlaneFromPointNormal( &tPortal.tPlane, &vCenter, &No );
	tPortal.vCenter = vCenter;

	float fRad = 0.f;
	for ( int i = 0; i < nBorderCount; ++i )
	{
		float fTest = VectorDistanceSquared( vCenter, vBorder[i] );
		fRad = MAX( fRad, fTest );
	}
	fRad = SQRT_SAFE( fRad );
	// safety fudge
	fRad *= 2.f;

	Model * pPortalRegionModel = SphereModel::create( (Vector3&)vCenter, fRad );
	ASSERT_RETFAIL( pPortalRegionModel );

#if ISVERSION(DEBUG_VERSION)
	char szName[256];
	PStrPrintf( szName, 256, "%s-%d", __FILE__, __LINE__ );
	pPortalRegionModel->setName( szName );
#endif

	pPortalRegionModel->autoRelease();
	RegionOfInfluence * pRoI = RegionOfInfluence::create( pPortalRegionModel );
	ASSERT_RETFAIL( pRoI );
	pRoI->autoRelease();
	pRoI->set( RegionOfInfluence::ENABLED, true );
	pRoI->setCell( pFromCell->pInternalCell );
	pRoI->setObjectToCellMatrix( (Matrix4x4&)mIdent );

#endif // ENABLE_DPVS_VIRTUAL_PORTALS
#endif // ENABLE_DPVS
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_PhysicalPortalGetOBB( CULLING_PHYSICAL_PORTAL * pPortal, MATRIX & mOBB )
{
	ASSERT_RETINVALIDARG( pPortal );

#if ENABLE_DPVS
	pPortal->pInternalPortal->getOBB( (Matrix4x4&)mOBB );
	Cell * pCell = pPortal->pInternalPortal->getCell();
	ASSERT_RETFAIL( pCell );
	MATRIX mToWorld;
	pCell->getCellToWorldMatrix( (Matrix4x4&)mToWorld );
	MatrixMultiply( &mOBB, &mOBB, &mToWorld );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DPVS_VirtualPortalGetOBB( CULLING_VIRTUAL_PORTAL * pPortal, MATRIX & mOBB )
{
	ASSERT_RETINVALIDARG( pPortal );

#if ENABLE_DPVS
	pPortal->pInternalPortal->getOBB( (Matrix4x4&)mOBB );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_DPVS_ModelGetVisible( int nModelID )
{
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( ! pModel )
		return FALSE;
	return dxC_DPVS_ModelGetVisible( pModel );
}

BOOL dxC_DPVS_ModelGetVisible( const D3D_MODEL * pModel )
{
	return dxC_ModelCurrentVisibilityToken( pModel );
}

//----------------------------------------------------------------------------

PRESULT e_DPVS_ModelSetEnabled( int nModelID, BOOL bEnabled )
{
	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel || !pModel->pCullObject )
		return S_FALSE;

	BOOL bActive = bEnabled;
	if ( pModel->nModelDefinitionId == INVALID_ID )
		bActive = FALSE;
	else if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_NODRAW ) )
		bActive = FALSE;
	else if ( dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_MODEL_DEF_NOT_APPLIED ) )
		bActive = FALSE;

	dxC_DPVS_SetEnabled( pModel->pCullObject, bActive );
#endif // ENABLE_DPVS

	return S_OK;
}

void dxC_DPVS_SetEnabled( Object * pObject, BOOL bEnabled )
{
	if ( c_GetToolMode() )
		return;
	ASSERT_RETURN( pObject );

#ifdef ENABLE_DPVS
	pObject->set( Object::ENABLED, bEnabled == TRUE );
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------

void e_DPVS_ModelSetCell( int nModelID, LP_INTERNAL_CELL pCell )
{
	if ( c_GetToolMode() )
		return;

#ifdef ENABLE_DPVS
	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel || !pModel->pCullObject )
		return;
	dxC_DPVS_SetCell( pModel->pCullObject, pCell );
#endif // ENABLE_DPVS
}

void dxC_DPVS_SetCell( Object * pObject, LP_INTERNAL_CELL pCell )
{
	if ( c_GetToolMode() )
		return;
	ASSERT_RETURN( pObject );

#ifdef ENABLE_DPVS
	pObject->setCell( pCell );
#endif // ENABLE_DPVS
}

LP_INTERNAL_CELL dxC_DPVS_GetCell( Object * pObject )
{
	if ( c_GetToolMode() )
		return NULL;
	ASSERT_RETNULL( pObject );

#ifdef ENABLE_DPVS
	return pObject->getCell();
#endif // ENABLE_DPVS
	return NULL;
}

//----------------------------------------------------------------------------

void dxC_DPVS_ModelSetCell( D3D_MODEL * pModel, LP_INTERNAL_CELL pNewCell )
{
	if ( c_GetToolMode() )
		return;

#ifdef ENABLE_DPVS
	ASSERT_RETURN( pModel );
	if ( ! pModel->pCullObject )
		return;

	//ASSERT( pNewCell != NULL );
	LP_INTERNAL_CELL pCurrentCell = dxC_DPVS_GetCell( pModel->pCullObject );
	if ( pCurrentCell == pNewCell )
		return;
	dxC_DPVS_SetCell( pModel->pCullObject, pNewCell );
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------

void e_DPVS_ModelSetObjToCellMatrix( int nModelID, MATRIX & tMatrix )
{
	if ( c_GetToolMode() )
		return;

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel || !pModel->pCullObject )
		return;
	dxC_DPVS_SetObjToCellMatrix( pModel->pCullObject, tMatrix );
}

void dxC_DPVS_SetObjToCellMatrix( Object * pObject, MATRIX & tMatrix )
{
	if ( c_GetToolMode() )
		return;
	ASSERT_RETURN( pObject );

	//MATRIX mMat = (MATRIX)tMatrix;
	//Cell * pCell = pObject->getCell();
	//if ( pCell )
	//{
	//	MATRIX mToCell;
	//	pCell->getWorldToCellMatrix( (Matrix4x4)mToCell );
	//	MatrixMultiply( &mMat, (MATRIX*)&tMatrix, &mToCell );
	//}

#ifdef ENABLE_DPVS
	pObject->setObjectToCellMatrix( (const Matrix4x4&)tMatrix );
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------

void e_DPVS_ModelSetTestModel( int nModelID, Model * pCullModel )
{
	if ( c_GetToolMode() )
		return;

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel || !pModel->pCullObject )
		return;
	dxC_DPVS_SetTestModel( pModel->pCullObject, pCullModel );
}

void dxC_DPVS_SetTestModel( Object * pObject, Model * pCullModel )
{
	if ( c_GetToolMode() )
		return;
	ASSERT_RETURN( pObject );

#ifdef ENABLE_DPVS
	pObject->setTestModel( pCullModel );
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------

void e_DPVS_ModelSetWriteModel( int nModelID, Model * pWriteModel )
{
	if ( c_GetToolMode() )
		return;

	D3D_MODEL * pModel = dx9_ModelGet( nModelID );
	if ( !pModel || !pModel->pCullObject )
		return;
	dxC_DPVS_SetWriteModel( pModel->pCullObject, pWriteModel );
}

void dxC_DPVS_SetWriteModel( Object * pObject, Model * pWriteModel )
{
	if ( c_GetToolMode() )
		return;
	ASSERT_RETURN( pObject );

#ifdef ENABLE_DPVS
	pObject->setWriteModel( pWriteModel );
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------

void e_DPVS_ModelSetVisibilityParent( int nChildModelID, int nParentModelID )
{
	if ( c_GetToolMode() )
		return;

	D3D_MODEL * pChildModel = dx9_ModelGet( nChildModelID );
	if ( !pChildModel || !pChildModel->pCullObject )
		return;
	D3D_MODEL * pParentModel = dx9_ModelGet( nParentModelID );
	if ( !pParentModel || !pParentModel->pCullObject )
		return;
	dxC_DPVS_SetVisibilityParent( pChildModel->pCullObject, pParentModel->pCullObject );
}

void dxC_DPVS_SetVisibilityParent( Object * pObject, Object * pParent )
{
	if ( c_GetToolMode() )
		return;
	ASSERT_RETURN( pObject );

#ifdef ENABLE_DPVS
	pObject->setVisibilityParent( pParent );
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_DPVS_ModelDefCreateTestModel( D3D_MODEL_DEFINITION * pModelDef )
{
	if ( c_GetToolMode() )
		return;

#ifdef ENABLE_DPVS
	// tester
	// use render bounding box

	ASSERT_RETURN( pModelDef );
	if ( pModelDef->pCullTestModel )
		return;

	pModelDef->pCullTestModel = OBBModel::create( (const Vector3&)pModelDef->tRenderBoundingBox.vMin, (const Vector3&)pModelDef->tRenderBoundingBox.vMax );
	ASSERT_RETURN( pModelDef->pCullTestModel );

#if ISVERSION(DEBUG_VERSION)
	char szName[256];
	PStrPrintf( szName, 256, "%d %s", pModelDef->tHeader.nId, pModelDef->tHeader.pszName );
	pModelDef->pCullTestModel->setName( szName );
#endif

	//pModelDef->pCullTestModel->autoRelease();
#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static float sGetTriangleArea( VECTOR & v1, VECTOR & v2, VECTOR & v3 )
{
	VECTOR vEdge1 = v1 - v2;
	VECTOR vEdge2 = v2 - v3;
	VECTOR vCross;
	VectorCross( vCross, (const VECTOR&)vEdge1, (const VECTOR&)vEdge2 );
	return VectorLength( vCross );
}

static float sGetTriangleMinAxis( VECTOR & v1, VECTOR & v2, VECTOR & v3 )
{
	BOUNDING_BOX tBBox;
	tBBox.vMin = v1;
	tBBox.vMax = v1;
	BoundingBoxExpandByPoint( &tBBox, &v2 );
	BoundingBoxExpandByPoint( &tBBox, &v3 );
	VECTOR vD = tBBox.vMax - tBBox.vMin;

	// since triangles are 2D, return the 2nd smallest axis
	if ( vD.x <= vD.y && vD.x <= vD.z )
		return MIN( vD.y, vD.z );
	if ( vD.y <= vD.x && vD.y <= vD.z )
		return MIN( vD.x, vD.z );
	if ( vD.z <= vD.y && vD.z <= vD.x )
		return MIN( vD.y, vD.x );

	ASSERTX_RETZERO( 0, "Shouldn't have gotten here!" );
}


//----------------------------------------------------------------------------

void dxC_DPVS_ModelDefCreateWriteModel( D3D_MODEL_DEFINITION * pModelDef )
{
	if ( c_GetToolMode() )
		return;

#ifdef ENABLE_DPVS
	// occluder
	// use collision mesh, if present

	ASSERT_RETURN( pModelDef );
	if ( pModelDef->dwFlags & MODELDEF_FLAG_ANIMATED )
		return;
	if ( pModelDef->pCullWriteModel )
		return;
	if ( ! e_BackgroundModelFileOccludesVisibility( pModelDef->tHeader.pszName ) )
		return;


#ifdef DPVS_DEBUG_OCCLUDER
	Vector3  *& pvVerts = gpDPVSDebugVerts;
	Vector3i *& pvTris  = gpDPVSDebugTris;
	int & nTris = gnDPVSDebugTris;
#else
	Vector3  * pvVerts = NULL;
	Vector3i * pvTris = NULL;
	int nTris;
#endif
	nTris = 0;
	int nVerts = 0;
	int nCurVBuffer = -1;

	BOOL bCreateBackfaces = TRUE;

	MLI_DEFINITION* pSharedModelDef = dxC_SharedModelDefinitionGet( pModelDef->tHeader.nId );
	if ( pSharedModelDef && pSharedModelDef->pOccluderDef )
	{
		OCCLUDER_DEFINITION* pOccluderDef = pSharedModelDef->pOccluderDef;
		nVerts = pOccluderDef->nVertexCount;
		pvVerts = MALLOC_TYPE( Vector3, NULL, nVerts );

		ASSERT_RETURN( pOccluderDef->nIndexCount % 3 == 0 );
		nTris = pOccluderDef->nIndexCount / 3;
		if ( bCreateBackfaces )
			nTris *= 2;
		pvTris = MALLOC_TYPE( Vector3i, NULL, nTris );

		ASSERT( sizeof( Vector3 ) == sizeof( VECTOR ) );
		DWORD dwVectorCopySize = sizeof( Vector3 ) * nVerts;
		MemoryCopy( pvVerts, dwVectorCopySize, pOccluderDef->pVertices, dwVectorCopySize );

		int nIndexCount = pOccluderDef->nIndexCount;
		for ( int nIndex = 0, t = 0; nIndex < nIndexCount; nIndex += 3 )
		{
			pvTris[ t ].i = pOccluderDef->pIndices[ nIndex ];
			pvTris[ t ].j = pOccluderDef->pIndices[ nIndex + 1 ];
			pvTris[ t ].k = pOccluderDef->pIndices[ nIndex + 2 ];
			t++;
			if ( bCreateBackfaces )
			{
				pvTris[ t ].i = pOccluderDef->pIndices[ nIndex ];
				pvTris[ t ].j = pOccluderDef->pIndices[ nIndex + 2 ];
				pvTris[ t ].k = pOccluderDef->pIndices[ nIndex + 1 ];
				t++;
			}
		}
	}
	
	if ( nVerts > 0 && nTris > 0 && pvVerts && pvTris )
	{
		DPVSTRACE( "Allocating write model for Def %d(%08p)", pModelDef->tHeader.nId, pModelDef );

		pModelDef->pCullWriteModel = MeshModel::create(
			pvVerts,
			pvTris,
			nVerts,
			nTris,
			true );

		ASSERT( pModelDef->pCullWriteModel );
		//pModelDef->pCullWriteModel->autoRelease();
	}

#ifndef DPVS_DEBUG_OCCLUDER
	if ( pvVerts )
		FREE( NULL, pvVerts );
	if ( pvTris )
		FREE( NULL, pvTris );
#endif

#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_DPVS_ModelCreateCullObject( D3D_MODEL * pModel, D3D_MODEL_DEFINITION * pModelDef )
{
	if ( c_GetToolMode() )
		return;

#ifdef ENABLE_DPVS

	// make model cull object from modeldef cull models
	ASSERT_RETURN( pModel );
	if ( pModel->pCullObject )
		return;
	if ( ! pModelDef || ! pModelDef->pCullTestModel )
		return;

	ASSERT_RETURN( pModel->nModelDefinitionId == pModelDef->tHeader.nId );
	//ASSERT_RETURN( gpDPVSCell );

	pModel->pCullObject = Object::create( pModelDef->pCullTestModel );
	ASSERT_RETURN( pModel->pCullObject );

	if ( pModelDef->pCullWriteModel )
	{
		pModel->pCullObject->setWriteModel( pModelDef->pCullWriteModel );
	}
	//else
	//	pModel->pCullObject->setCost( 1, 1, 100.f );

	pModel->pCullObject->setUserPointer( pModel );

	//e_ModelSetRegionPrivate( pModel->nId, pModel->nRegion );
	dxC_DPVS_ModelUpdate( pModel );

	//pModel->pCullObject->autoRelease();

#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_DPVS_ModelUpdate( D3D_MODEL * pModel )
{
#if !ISVERSION(SERVER_VERSION)

	if ( c_GetToolMode() )
		return S_OK;

#ifdef ENABLE_DPVS

	ASSERT_RETINVALIDARG( pModel );
	if ( ! pModel->pCullObject )
		return S_FALSE;

	if ( pModel->pCullObject->getCell() == NULL && pModel->nCellID != INVALID_ID )
	{
		CULLING_CELL * pCell = e_CellGet( pModel->nCellID );
		if ( pCell && pCell->pInternalCell )
		{
			dxC_DPVS_ModelSetCell( pModel, pCell->pInternalCell );
		}
	}
	else if ( pModel->nCellID == INVALID_ID )
	{
		//REGION * pRegion = e_RegionGet( pModel->nRegion );
		//if ( pRegion && pRegion->nCullCell != INVALID_ID )
		//{
		//	CULLING_CELL * pCell = e_CellGet( pRegion->nCullCell );
		//	if ( pCell && pCell->pInternalCell )
		//	{
		//		dxC_DPVS_ModelSetCell( pModel, pCell->pInternalCell );
		//	}
		//}
	}

	MATRIX mWorld = pModel->matScaledWorld;

	// update with ragdoll position, if necessary
	if ( AppIsHellgate() && dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) )
	{
		VECTOR vRagPos;
		BOOL bRagPos = e_ModelGetRagdollPosition( pModel->nId, vRagPos );
		if ( bRagPos )
		{
			const float cfScaleUp = 2.0f;

			BOUNDING_BOX tBBox;
			tBBox.vMin = VECTOR(0,0,0);
			tBBox.vMax = VECTOR(0,0,0);
			V( dxC_GetModelRenderAABBInObject( pModel, &tBBox ) );

			// Scale up the matrix to the biggest extent of the original in each direction.
			VECTOR vSize = tBBox.vMax - tBBox.vMin;
			float fMax = MAX( vSize.x, MAX( vSize.y, vSize.z ) );
			// Plus the model scale and a fudge amount to handle ragdoll skew.
			fMax *= cfScaleUp * pModel->vScale.fX;
			VECTOR vScale = VECTOR(fMax / vSize.x, fMax / vSize.y, fMax / vSize.z);
			// This blows away the existing scale/rotation.
			MatrixScale( &mWorld, &vScale );

			// Set the translation;
			*(VECTOR*)(&mWorld.m[3][0]) = vRagPos;
		}
	}

	LP_INTERNAL_CELL pCell = dxC_DPVS_GetCell( pModel->pCullObject );
	if ( ! pCell )
		return S_FALSE;
	MATRIX & mO2Cell = mWorld;

	// ensure that the fourth column is proper identity (avoid rounding error)
	mO2Cell.m[0][3] = 0.f;
	mO2Cell.m[1][3] = 0.f;
	mO2Cell.m[2][3] = 0.f;
	mO2Cell.m[3][3] = 1.f;

	pModel->pCullObject->setObjectToCellMatrix( (const Matrix4x4&)mO2Cell );

#endif // ENABLE_DPVS


#endif // !ISVERSION(SERVER_VERSION)

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DPVS_DebugRestoreTexture()
{
	if ( c_GetToolMode() )
		return;

#ifdef ENABLE_DPVS

	if ( e_TextureIsValidID( gnDPVSDebugTextureID ) )
		return;

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	V( dx9_TextureNewEmpty(
		&gnDPVSDebugTextureID,
		nWidth,
		nHeight,
		1,
		D3DFMT_A8R8G8B8,
		INVALID_ID,
		INVALID_ID,
		D3DC_USAGE_2DTEX_DEFAULT ) );

	ASSERT_RETURN( e_TextureIsValidID( gnDPVSDebugTextureID ) );

#endif // ENABLE_DPVS
}

//----------------------------------------------------------------------------

PRESULT e_DPVS_DebugUpdateTexture( const unsigned char * pbyData, int nWidth, int nHeight )
{
	if ( c_GetToolMode() )
		return S_FALSE;

#ifdef ENABLE_DPVS

	if ( ! e_GetRenderFlag( RENDER_FLAG_DBG_TEXTURE_ENABLED ) )
		return S_FALSE;

	e_DPVS_DebugRestoreTexture();
	if ( ! e_TextureIsValidID( gnDPVSDebugTextureID ) )
		return S_FALSE;

	D3D_TEXTURE * pTexture = dxC_TextureGet( gnDPVSDebugTextureID );
	ASSERT_RETFAIL( pTexture && pTexture->pD3DTexture );

	nWidth  = min( nWidth,  pTexture->nWidthInPixels );
	nHeight = min( nHeight, pTexture->nHeight );
	RECT tSrcRect;
	SetRect( &tSrcRect, 0, 0, nWidth, nHeight );

#ifdef ENGINE_TARGET_DX9
	SPDIRECT3DSURFACE9 pDestSurface;
	V_RETURN( pTexture->pD3DTexture->GetSurfaceLevel( 0, &pDestSurface ) );
	V_RETURN( D3DXLoadSurfaceFromMemory( pDestSurface, NULL, &tSrcRect, pbyData, D3DFMT_L8, nWidth * 1, NULL, &tSrcRect, D3DX_DEFAULT, 0 ) );
	//D3DLOCKED_RECT tLockRect;
	//dxC_MapTexture( pTexture->pD3DTexture, 0, &tLockRect );
	//for ( int y = 0; y < pTexture->nHeight; y++ )
	//{
	//	for ( int x = 0; x < nWidth; x++ )
	//	{
	//		//BYTE val = ((BYTE*)tLockRect.pBits)[ y * tLockRect.Pitch + x ];
	//		BYTE val = pbyData[ y * nWidth + x ];
	//		if ( val > 0 )
	//		{
	//			int a=0;
	//		}
	//	}
	//}
	//dxC_UnmapTexture( pTexture->pD3DTexture, 0 );
#endif

	dxC_DebugTextureGetShaderResource() = pTexture->pD3DTexture;

#endif // ENABLE_DPVS
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//void e_DPVS_MakeRenderList()
//{
//#ifdef ENABLE_DPVS
//	if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
//		return;
//
//	e_DPVS_Update();
//
//	// currently only supports the "normal" pass (no portals)
//
//	RL_PASS_DESC tDesc;
//	tDesc.eType = RL_PASS_NORMAL;
//	if ( AppIsHellgate() )
//		tDesc.dwFlags = RL_PASS_FLAG_CLEAR_BACKGROUND | RL_PASS_FLAG_RENDER_PARTICLES | RL_PASS_FLAG_RENDER_SKYBOX | RL_PASS_FLAG_GENERATE_SHADOWS;
//	else
//		tDesc.dwFlags = RL_PASS_FLAG_RENDER_PARTICLES | RL_PASS_FLAG_GENERATE_SHADOWS;	
//	PASSID nTopPass = e_RenderListBeginPass( tDesc, e_GetCurrentRegionID() );
//
//	if ( nTopPass == INVALID_ID )
//		return;
//
//	for ( D3D_MODEL * pModel = dx9_ModelGetFirst();
//		pModel;
//		pModel = dx9_ModelGetNext( pModel ) )
//	{
//		if ( ! dxC_DPVS_ModelGetVisible( pModel ) )
//			continue;
//
//		int nRootIndex = e_RenderListAddModel( nTopPass, pModel->nId, 0 );
//		//e_RenderListSetBBox( nTopPass, nRootIndex, &tCommand.ClipBox );
//	}
//
//	e_RenderListEndPass( nTopPass );
//
//#endif // ENABLE_DPVS
//}
