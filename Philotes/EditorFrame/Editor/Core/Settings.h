
/********************************************************************
	日期:		2012年2月20日
	文件名: 	Settings.h
	创建者:		王延兴
	描述:		编辑器全局设置面板	
*********************************************************************/

#pragma once

struct GuiSettings
{
	// Vista/Win7系统选择
	BOOL	bWindowsVista;

	// 开启皮肤
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