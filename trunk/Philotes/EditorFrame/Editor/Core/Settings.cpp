
#include "Settings.h"

_NAMESPACE_BEGIN

EditorSettings::EditorSettings()
{
	OSVERSIONINFO OSVerInfo;
	OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OSVerInfo);
	Gui.bWindowsVista = OSVerInfo.dwMajorVersion >= 6;
	Gui.bSkining = FALSE;

	int lfHeight = -MulDiv(8, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
	Gui.nDefaultFontHieght = lfHeight;
	Gui.hSystemFont = ::CreateFont(lfHeight,0,0,0,FW_NORMAL,0,0,0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, "Ms Shell Dlg 2");

	Gui.hSystemFontBold = ::CreateFont(lfHeight,0,0,0,FW_BOLD,0,0,0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, "Ms Shell Dlg 2");

	Gui.hSystemFontItalic = ::CreateFont(lfHeight,0,0,0,FW_NORMAL,TRUE,0,0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH, "Ms Shell Dlg 2");

	cameraMoveSpeed = 1;
	cameraFastMoveSpeed = 2;
	wheelZoomSpeed = 1;

	Load();
}

BOOL EditorSettings::Save()
{
	return FALSE;
}

BOOL EditorSettings::Load()
{
	return FALSE;
}

EditorSettings& EditorSettings::Get()
{
	static EditorSettings set;
	return set;
}

_NAMESPACE_END