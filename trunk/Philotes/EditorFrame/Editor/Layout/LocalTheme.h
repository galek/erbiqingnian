
/********************************************************************
	日期:		2012年2月22日
	文件名: 	LocalTheme.h
	创建者:		王延兴
	描述:		编辑器主题	
*********************************************************************/

#pragma once

class CLocalTheme : public CXTPNativeXPTheme
{
public:
	CLocalTheme();

	virtual ~CLocalTheme();

	virtual void DrawImage(CDC* pDC, CPoint pt, CSize sz, CXTPImageManagerIcon* pImage,
		BOOL bSelected, BOOL bPressed, BOOL bEnabled = TRUE,
		BOOL bChecked = FALSE, BOOL bPopuped = FALSE, BOOL bToolBarImage = TRUE);
};
