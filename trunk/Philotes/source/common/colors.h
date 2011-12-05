//----------------------------------------------------------------------------
// colors.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _COLORS_H_
#define _COLORS_H_


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define GFXCOLOR_BLACK			0xff000000
#define GFXCOLOR_WHITE			0xffffffff

#define GFXCOLOR_LTRED			0xffff4040

#define GFXCOLOR_RED			0xffff0000
#define GFXCOLOR_GREEN			0xff00ff00
#define GFXCOLOR_BLUE			0xff0000ff
#define GFXCOLOR_YELLOW			0xffffff00
#define GFXCOLOR_CYAN			0xff00ffff
#define GFXCOLOR_PURPLE			0xffff00ff
#define GFXCOLOR_GRAY			0xffc0c0c0

#define GFXCOLOR_DKRED			0xff800000
#define GFXCOLOR_DKGREEN		0xff008000
#define GFXCOLOR_DKBLUE			0xff000080
#define GFXCOLOR_DKYELLOW		0xff808000
#define GFXCOLOR_DKCYAN			0xff008080
#define GFXCOLOR_DKPURPLE		0xff800080
#define GFXCOLOR_DKGRAY			0xff808080

#define GFXCOLOR_VDKRED			0xff400000
#define GFXCOLOR_VDKGREEN		0xff004000
#define GFXCOLOR_VDKBLUE		0xff000040
#define GFXCOLOR_VDKYELLOW		0xff404000
#define GFXCOLOR_VDKCYAN		0xff004040
#define GFXCOLOR_VDKPURPLE		0xff400040
#define GFXCOLOR_VDKGRAY		0xff404040

#define GFXCOLOR_HOTPINK		0xffff69cd

#define GFXCOLOR_STD_UI_OUTLINE	0xfff78e1e
#define GFXCOLOR_STD_UI_BKGRD	0xff1c6497

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define GFXCOLOR_GET_RED(dwColor)   ((dwColor & 0x00ff0000) >> 16)
#define GFXCOLOR_GET_GREEN(dwColor) ((dwColor & 0x0000ff00) >> 8)
#define GFXCOLOR_GET_BLUE(dwColor)  ((dwColor & 0x000000ff))
#define GFXCOLOR_GET_ALPHA(dwColor) ((dwColor & 0xff000000) >> 24)
#define GFXCOLOR_MAKE(r, g, b, a) (((a & 0xff) << 24) + ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff))


#endif // _COLORS_H_
