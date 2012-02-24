
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	Settings.h
	������:		������
	����:		�༭��ȫ���������	
*********************************************************************/

#pragma once

_NAMESPACE_BEGIN

struct GuiSettings
{
	// Vista/Win7ϵͳѡ��
	BOOL	bWindowsVista;

	// ����Ƥ��
	BOOL	bSkining;

	HFONT	hSystemFont;

	HFONT	hSystemFontBold;

	HFONT	hSystemFontItalic;

	int		nDefaultFontHieght; 
};

//////////////////////////////////////////////////////////////////////////

struct EditorSettings
{
	EditorSettings();

	static EditorSettings& Get();

	BOOL Save();

	BOOL Load();

	GuiSettings	Gui;

	float		cameraMoveSpeed;
	float		cameraFastMoveSpeed;
	float		wheelZoomSpeed;
};

_NAMESPACE_END