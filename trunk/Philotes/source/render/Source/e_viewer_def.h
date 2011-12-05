//----------------------------------------------------------------------------
// e_viewer_def.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Viewer render functions
//----------------------------------------------------------------------------

#ifdef INCLUDE_VR_ARRAY
#	define _VR_START()			PFN_VIEWER_RENDER	gpfn_ViewerRender[ NUM_VIEWER_RENDER_TYPES ] = {
#	define _VR( name, pfn )		pfn,
#	define _VR_END()			};
#	undef INCLUDE_VR_ARRAY
#elif defined(INCLUDE_VR_ENUM)
#	define _VR_START()
#	define _VR( name, pfn )		name,
#	define _VR_END()
#	undef INCLUDE_VR_ENUM
#endif


_VR_START()
_VR( VIEWER_RENDER_HELLGATE,				e_ViewerRender_Common		)
_VR( VIEWER_RENDER_MYTHOS,					e_ViewerRender_Common		)
_VR_END()


#undef _VR_START
#undef _VR
#undef _VR_END