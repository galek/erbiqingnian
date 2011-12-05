//----------------------------------------------------------------------------
// uix_xml.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_XML_H_
#define _UIX_XML_H_

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "markup.h"


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define XML_LOAD(filename, key)			ASSERT(filename); \
										char fullfilename[MAX_XML_STRING_LENGTH]; \
										sprintf(fullfilename, "%s%s", FILE_PATH_UI_XML, filename); \
										PStrReplaceExtension(fullfilename, MAX_XML_STRING_LENGTH, fullfilename, DEFINITION_FILE_EXTENSION); \
										bool bResult = false; \
										int size; \
										char* buf = (char *)FileLoad(NULL, fullfilename, &size); \
										CMarkup xml; \
										if (!buf) goto _stop; \
										xml.SetDoc(buf); \
										bool found = xml.FindElem(key); \
										if (!found) goto _stop;

#define XML_END							bResult = true; \
										_stop: \
										if (buf) FREE(NULL, buf);

#define XML_LOADNAME()						{xml.ResetChildPos(); if (xml.FindChildElem("Name")) { strncpy(m_Name, xml.GetChildData(), UI_NAME_MAXLEN - 5); m_Name[UI_NAME_MAXLEN - 5] = 0; NameTableMakeUniqueName(g_UIX.m_pNameTable, m_Name, UI_NAME_MAXLEN); NameTableRegister(g_UIX.m_pNameTable, m_Name, NAMETYPE_UI, (void *)GetId()); }}
#define XML_LOADSTRING(key, var, def, size)	{xml.ResetChildPos(); if (xml.FindChildElem(key)) { strncpy(var, xml.GetChildData(), size); var[size-1] = 0; } else { if (def) { strncpy(var, def, size); var[size-1] = 0; } else { var[0] = 0; } }}
#define XML_LOADCSTRING(key, var, def)		{xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = xml.GetChildData(); } else { if (def) { var = def; } else { var = L""; } }}
#define XML_LOADFLOAT(key, var, def)		{xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (float)atof(xml.GetChildData()); } else { var = def; }}
#define XML_LOADFLOATF(key, F, def)			{xml.ResetChildPos(); if (xml.FindChildElem(key)) { F((float)atof(xml.GetChildData())); } else { F(def); }}
#define XML_LOADINT(key, var, def)			{xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (int)atoi(xml.GetChildData()); } else { var = def; }}
#define XML_LOADINTF(key, F, def)			{xml.ResetChildPos(); if (xml.FindChildElem(key)) { F((int)atoi(xml.GetChildData())); } else { F(def); }}
#define XML_LOADBOOL(key, var, def)			{xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (BOOL)atoi(xml.GetChildData()); } else { var = def; }}
#define XML_LOADBOOLF(key, F, def)			{xml.ResetChildPos(); if (xml.FindChildElem(key)) { F((BOOL)atoi(xml.GetChildData())); } else { F(def); }}
#define XML_LOADHDWORD(key, var, def)		{xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (DWORD)atoh(xml.GetChildData()); } else { var = def; }}
#define XML_LOADHDWORDF(key, F, def)		{xml.ResetChildPos(); if (xml.FindChildElem(key)) { F((DWORD)atoh(xml.GetChildData())); } else { F(def); }}
#define XML_LOADENUM(key, L, var, def)		{xml.ResetChildPos(); if (xml.FindChildElem(key)) { int ii = 0; do { if (L[ii] == NULL) { var = def; break; } if (stricmp(xml.GetChildData(), L[ii]) == 0) { var = ii; break; } ii++; } while (1); } else { var = def; }}
#define XML_LOADENUMF(key, L, F, def)		{xml.ResetChildPos(); if (xml.FindChildElem(key)) { int ii = 0; do { if (L[ii] == NULL) { F(def); break; } if (stricmp(xml.GetChildData(), L[ii]) == 0) { F(ii); break; } ii++; } while (1); } else { F(def); }}
#define XML_LOADTEXTURE(key, var)			{xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = c_TextureNewFromFile(xml.GetChildData(), TEXTURE_GROUP_UI, 0 ); } else { var = INVALID_ID; }}
#define XML_LOADTEXTUREF(key, F)			{xml.ResetChildPos(); if (xml.FindChildElem(key)) { F(c_TextureNewFromFile(xml.GetChildData(), TEXTURE_GROUP_UI, 0)); } else { F(INVALID_ID); }}
#define XML_LOADFONT(key, var)				{xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = GetFontIndex(xml.GetChildData()); } else { var = INVALID_ID; }}
#define XML_LOADFONTF(key, F)				{xml.ResetChildPos(); if (xml.FindChildElem(key)) { F(GetFontIndex(xml.GetChildData())); } else { F(INVALID_ID); }}

#define XML_LOADCHILDCONTROLS			{xml.ResetChildPos(); \
										while (xml.FindChildElem()) { \
											LoadUIControl(xml, this); }}


//----------------------------------------------------------------------------
// EXPORTED VARIABLES
//----------------------------------------------------------------------------
extern const char* UI_XML_ALIGNMENT[];
extern const char* UI_XML_ORIENTATION[];


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
DWORD atoh(
	const char* str);


#endif // _UIX_XML_H_

