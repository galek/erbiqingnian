//----------------------------------------------------------------------------
// dxC_rendercommand_def.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef RCD_TYPE
#	define RCD_TYPE(name)					RCHDATA_##name
#endif

#ifndef RCD_ARRAY_TYPE
#	define RCD_ARRAY_TYPE(name)				RCD_TYPE(name)_ARRAYTYPE
#endif

#ifndef RCD_ARRAY
#	define RCD_ARRAY(name)					gpt_RCHDATA_##name
#endif

#ifndef RCD_ARRAY_NEW
#	define RCD_ARRAY_NEW(name, nID)			ONCE { ASSERT_BREAK( RCD_ARRAY(name) );		RCD_TYPE(name) tLocal;	RCD_ARRAY(name)->push_back( tLocal );	nID = RCD_ARRAY(name)->size() - 1;	}
#endif

#ifndef RCD_ARRAY_GET
#	define RCD_ARRAY_GET(name, nID, pData)	ONCE { ASSERT_BREAK( RCD_ARRAY(name) );		ASSERT_BREAK( IS_VALID_INDEX( nID, (int)RCD_ARRAY(name)->size() ) );		pData = &(*RCD_ARRAY(name))[ nID ];	}
#endif

//----------------------------------------------------------------------------
// Render command definitions
//----------------------------------------------------------------------------

#ifdef INCLUDE_RCD_FUNC_ARRAY
#	define _RCD( name, pfn )			pfn,
#	undef INCLUDE_RCD_FUNC_ARRAY
#elif defined(INCLUDE_RCD_ENUM)
#	define _RCD( name, pfn )			name,
#	undef INCLUDE_RCD_ENUM
#elif defined(INCLUDE_RCD_DATA_ARRAY_TYPEDEF)
#	define _RCD( name, pfn )			typedef safe_vector< RCD_TYPE(name) > RCD_TYPE(name)##_ARRAYTYPE;
#	undef INCLUDE_RCD_DATA_ARRAY_TYPEDEF
#elif defined(INCLUDE_RCD_DATA_VECTOR)
#	define _RCD( name, pfn )			RCD_ARRAY_TYPE(name) * RCD_ARRAY(name) = NULL;
#	undef INCLUDE_RCD_DATA_VECTOR
#elif defined(INCLUDE_RCD_DATA_EXTERN)
#	define _RCD( name, pfn )			extern RCD_ARRAY_TYPE(name) * RCD_ARRAY(name);
#	undef INCLUDE_RCD_DATA_EXTERN
#elif defined(INCLUDE_RCD_DATA_ARRAY_INIT)
#	define _RCD( name, pfn )			ONCE { ASSERT_BREAK( RCD_ARRAY(name) == NULL );		RCD_ARRAY(name) = MALLOC_NEW( NULL, RCD_TYPE(name)##_ARRAYTYPE );	ASSERT_BREAK( RCD_ARRAY(name) );	};
#	undef INCLUDE_RCD_DATA_ARRAY_INIT
#elif defined(INCLUDE_RCD_DATA_ARRAY_DESTROY)
#	define _RCD( name, pfn )			ONCE { if ( RCD_ARRAY(name) != NULL )	{	FREE_DELETE( NULL, RCD_ARRAY(name), RCD_TYPE(name)##_ARRAYTYPE );	RCD_ARRAY(name) = NULL;		} }
#	undef INCLUDE_RCD_DATA_ARRAY_DESTROY
#elif defined(INCLUDE_RCD_DATA_ARRAY_CLEAR)
#	define _RCD( name, pfn )			ONCE { if ( RCD_ARRAY(name) == NULL ) break;	RCD_ARRAY(name)->clear();	}
#	undef INCLUDE_RCD_DATA_ARRAY_CLEAR
#elif defined(INCLUDE_RCD_ENUM_STRING)
#	define _RCD( name, pfn )			#name,
#	undef INCLUDE_RCD_ENUM_STRING
#else
#	error "Missing define!"
#endif


//	  enum/RCHDATA_struct			handler func
_RCD( DRAW_MESH_LIST,				dxC_RCH_DrawMeshList		)
_RCD( SET_TARGET,					dxC_RCH_SetTarget			)
_RCD( CLEAR_TARGET,					dxC_RCH_ClearTarget			)
_RCD( COPY_BUFFER,					dxC_RCH_CopyBuffer			)
_RCD( SET_VIEWPORT,					dxC_RCH_SetViewport			)
_RCD( SET_CLIP,						dxC_RCH_SetClip				)
_RCD( SET_SCISSOR,					dxC_RCH_SetScissor			)
_RCD( SET_FOG_ENABLED,				dxC_RCH_SetFogEnabled		)
_RCD( CALLBACK_FUNC,				dxC_RCH_Callback			)
_RCD( DRAW_PARTICLES,				dxC_RCH_DrawParticles		)
_RCD( SET_COLORWRITE,				dxC_RCH_SetColorWrite		)
_RCD( SET_DEPTHWRITE,				dxC_RCH_SetDepthWrite		)
_RCD( DEBUG_TEXT,					dxC_RCH_DebugText			)


#undef _RCD
