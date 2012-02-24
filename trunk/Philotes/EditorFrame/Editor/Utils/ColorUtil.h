
/********************************************************************
	日期:		2012年2月24日
	文件名: 	ColorUtil.h
	创建者:		王延兴
	描述:		颜色定义以及一些常用操作	
*********************************************************************/

#pragma once

_NAMESPACE_BEGIN

typedef D3DXCOLOR ColorValue;

namespace ColorUtil
{
	ColorValue			ColorFromHSV(float h, float s, float v);

	UINT				ColorPackBGR888(const ColorValue& color);

	COLORREF			LinearToGamma( ColorValue col );

	ColorValue			GammaToLinear( COLORREF col );
}

_NAMESPACE_END