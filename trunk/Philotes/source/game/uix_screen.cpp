//----------------------------------------------------------------------------
// uix_screen.cpp
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
#include "uix_background.h"
#include "uix_screen.h"


//----------------------------------------------------------------------------
// UI_SCREEN
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_SCREEN::UI_SCREEN(
	const char* xml_file)
{
	m_fWidth = AppGetWindowWidth();
	m_fHeight = AppGetWindowHeight();

	ASSERT(xml_file);

	char fullfilename[MAX_XML_STRING_LENGTH];
	sprintf(fullfilename, "%s%s", FILE_PATH_UI_XML, xml_file);
	PStrReplaceExtension(fullfilename, MAX_XML_STRING_LENGTH, fullfilename, DEFINITION_FILE_EXTENSION);
	bool bResult = false;

	int size;
	char* buf = (char*)FileLoad(NULL, fullfilename, &size);

	CMarkup xml;
	if (!buf) 
	{
		goto _stop;
	}
	xml.SetDoc(buf);
	bool found = xml.FindElem("UI_SCREEN_DEFINITION");
	if (!found) 
	{
		goto _stop;
	}
	xml.ResetChildPos();
	LoadDefinition(xml, NULL);

	bResult = true;

_stop:
	if (buf) 
	{
		FREE(NULL, buf);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UI_SCREEN::LoadDefinition(
	CMarkup& xml,
	UI_CONTROL* parent)
{
	XML_LOADNAME();
	XML_LOADTEXTUREF("Texture", m_Background.SetTexture);
	XML_LOADHDWORDF("Color", m_Background.SetColor, 0xffffffff);
	XML_LOADINTF("Tile", m_Background.SetTile, FALSE);
	m_fWidth = AppGetWindowWidth();
	m_fHeight = AppGetWindowHeight();

	m_Background.SetRect(m_Position.x, m_Position.y, m_fWidth, m_fHeight);

	XML_LOADCHILDCONTROLS;

	return true;
}




