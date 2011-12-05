//----------------------------------------------------------------------------
// uix_messages.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef UIX_GRAPHIC_ENUM
	#define UIMSG(m, c, pi, fp)					m,
#else 
	#define UIMSG(m, c, pi, fp)					{ m, #m, c, pi, fp },
#endif

	  //message				//always pass to children	// process when inactive	//default handler
UIMSG(UIMSG_PAINT,				TRUE,					TRUE,						UIComponentOnPaint)								
UIMSG(UIMSG_MOUSEMOVE,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_MOUSEOVER,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_MOUSELEAVE,			FALSE,					TRUE,						UIComponentOnMouseLeave)						
UIMSG(UIMSG_LBUTTONDOWN,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_MBUTTONDOWN,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_RBUTTONDOWN,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_LBUTTONUP,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_MBUTTONUP,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_RBUTTONUP,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_LBUTTONDBLCLK,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_RBUTTONDBLCLK,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_LBUTTONCLICK,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_RBUTTONCLICK,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_MBUTTONCLICK,		FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_MOUSEWHEEL,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_KEYDOWN,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_KEYUP,				FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_KEYCHAR,			FALSE,					FALSE,						NULL)											
UIMSG(UIMSG_INVENTORYCHANGE,	FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_SETHOVERUNIT,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_ACTIVATE,			FALSE,					TRUE,						UIComponentOnActivate)							
UIMSG(UIMSG_INACTIVATE,			FALSE,					TRUE,						UIComponentOnInactivate)						
UIMSG(UIMSG_SETCONTROLUNIT,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_SETTARGETUNIT,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_CONTROLUNITGETHIT,	FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_SETCONTROLSTAT,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_SETTARGETSTAT,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_TOGGLEDEBUGDISPLAY, FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_PLAYERDEATH,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_RESTART,			FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_POSTACTIVATE,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_POSTINACTIVATE,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_POSTVISIBLE,		FALSE,					TRUE,						NULL)											
UIMSG(UIMSG_POSTINVISIBLE,		FALSE,					TRUE,						NULL)
UIMSG(UIMSG_MOUSEHOVER,			FALSE,					TRUE,						UIComponentMouseHover)
UIMSG(UIMSG_MOUSEHOVERLONG,		FALSE,					TRUE,						UIComponentMouseHoverLong)
UIMSG(UIMSG_SETCONTROLSTATE,	FALSE,					TRUE,						NULL)
UIMSG(UIMSG_SETTARGETSTATE,		FALSE,					TRUE,						NULL)
UIMSG(UIMSG_SETFOCUSSTAT,		FALSE,					TRUE,						NULL)
UIMSG(UIMSG_SCROLL,				FALSE,					TRUE,						NULL) 
UIMSG(UIMSG_FREEUNIT,			TRUE,					TRUE,						UIComponentOnFreeUnit) 
UIMSG(UIMSG_SETFOCUS,			FALSE,					TRUE,						NULL) 
UIMSG(UIMSG_PARTYCHANGE,		FALSE,					TRUE,						NULL) 
UIMSG(UIMSG_ANIMATE,			FALSE,					TRUE,						NULL) 
UIMSG(UIMSG_REFRESH_TEXT_KEY,	TRUE,					TRUE,						UIComponentRefreshTextKey) 
UIMSG(UIMSG_LB_ITEMSEL,			FALSE,					TRUE,						NULL)
UIMSG(UIMSG_LB_ITEMHOVER,		FALSE,					TRUE,						NULL)
UIMSG(UIMSG_POSTCREATE,			FALSE,					TRUE,						NULL)
UIMSG(UIMSG_SETFOCUSUNIT,		TRUE,					TRUE,						NULL)
UIMSG(UIMSG_CURSORINACTIVATE,	FALSE,					TRUE,						NULL)
UIMSG(UIMSG_CURSORACTIVATE,		FALSE,					TRUE,						NULL)
UIMSG(UIMSG_WARDROBECHANGE,		FALSE,					TRUE,						NULL)
UIMSG(UIMSG_FULLTEXTREVEAL,		FALSE,					TRUE,						NULL)
UIMSG(UIMSG_INPUTLANGCHANGE,	TRUE,					TRUE,						UIComponentLanguageChanged)
UIMSG(UIMSG_RESIZE,				FALSE,					TRUE,						UIComponentResize)

#undef UIMSG
