
#pragma once

typedef D3DXCOLOR ColorValue;

namespace ColorUtil
{
	ColorValue	ColorFromHSV(float h, float s, float v);

	UINT		ColorPackBGR888(const ColorValue& color);

}