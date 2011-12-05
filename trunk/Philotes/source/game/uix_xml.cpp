//----------------------------------------------------------------------------
// uix_xml.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "prime.h"
#include "excel.h"
#include "c_d3d_texture.h"
#include "c_font.h"
#include "uix_xml.h"
#include "uix.h"
#include "uix_texture.h"
#include "uix_control.h"
#include "uix_imagecontrol.h"
#include "uix_background.h"
#include "uix_border.h"
#include "uix_titlebar.h"
#include "uix_window.h"
#include "uix_label.h"
#include "uix_button.h"
#include "uix_checkbox.h"
#include "uix_editbox.h"
#include "uix_slider.h"
#include "uix_groupbox.h"
#include "uix_radiobutton.h"
#include "uix_scrollbar.h"
#include "uix_listbox.h"
#include "uix_combobox.h"
#include "uix_progressbar.h"
#include "uix_tabcontrol.h"
#include "uix_menu.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
const char* UI_XML_ALIGNMENT[] =
{
	"none",
	"upper left",
	"upper",
	"upper right",
	"right",
	"lower right",
	"lower",
	"lower left",
	"left",
	"center",
	"client",
	"client top",
	"client right",
	"client bottom",
	"client left",
	NULL
};

const char* UI_XML_ORIENTATION[] =
{
	"horizontal",
	"vertical",
	NULL
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD atoh(
	const char* str)
{
	while (*str && *str < ' ')
	{
		str++;
	}
	if (strncmp(str, "0x", 2) != 0)
	{
		return atoi(str);
	}
	str += 2;
	
	DWORD value = 0;
	while (*str)
	{
		value *= 16;
		switch (*str)
		{
		case '0': value += 0; break;
		case '1': value += 1; break;
		case '2': value += 2; break;
		case '3': value += 3; break;
		case '4': value += 4; break;
		case '5': value += 5; break;
		case '6': value += 6; break;
		case '7': value += 7; break;
		case '8': value += 8; break;
		case '9': value += 9; break;
		case 'A':
		case 'a': value += 10; break;
		case 'B':
		case 'b': value += 11; break;
		case 'C':
		case 'c': value += 12; break;
		case 'D':
		case 'd': value += 13; break;
		case 'E':
		case 'e': value += 14; break;
		case 'F':
		case 'f': value += 15; break;
		default:
			return 0;
		}
		str++;
	}
	return value;
}
