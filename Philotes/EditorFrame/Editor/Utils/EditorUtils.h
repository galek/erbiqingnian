
/********************************************************************
	����:		2012��2��20��
	�ļ���: 	EditorUtils.h
	������:		������
	����:		�༭�����õĹ��ߺ���	
*********************************************************************/

#pragma once

#define TOOLBAR_TRANSPARENT_COLOR RGB(0xC0,0xC0,0xC0)

struct CMFCUtils
{
	static BOOL LoadTrueColorImageList( CImageList &imageList,UINT nIDResource,int nIconWidth,COLORREF colMaskColor );

	static void TransparentBlt( HDC hdcDest, int nXDest, int nYDest, int nWidth,int nHeight, HBITMAP hBitmap, int nXSrc, int nYSrc,
		COLORREF colorTransparent );
};
