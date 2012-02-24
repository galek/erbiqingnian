
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