
#include "ColorUtil.h"

namespace ColorUtil
{
	ColorValue ColorFromHSV( float h, float s, float v )
	{
		ColorValue result(0,0,0,1);
		float r, g, b;
		if (v == 0.0f)
		{
			r = 0.0f; g = 0.0f; b = 0.0f;
		}
		else if (s == 0.0f)
		{
			r = v; g = v; b = v;
		}
		else
		{
			const float hi = h * 6.0f;
			const int i = (int)::floor(hi);
			const float f = hi - i;

			const float v0 = v * (1.0f - s);
			const float v1 = v * (1.0f - s * f);
			const float v2 = v * (1.0f - s * (1.0f - f));

			switch (i)
			{
			case 0: r = v; g = v2; b = v0; break;
			case 1: r = v1; g = v; b = v0; break;
			case 2: r = v0; g = v; b = v2; break;
			case 3: r = v0; g = v1; b = v; break;
			case 4: r = v2; g = v0; b = v; break;
			case 5: r = v; g = v0; b = v1; break;

			case 6: r = v; g = v2; b = v0; break;
			case -1: r = v; g = v0; b = v1; break;
			default: r = 0.0f; g = 0.0f; b = 0.0f; break;
			}
		}

		result.r = r;
		result.g = g;
		result.b = b;

		return result;
	}

	UINT ColorPackBGR888( const ColorValue& color )
	{
		unsigned char cr;
		unsigned char cg;
		unsigned char cb;
		if(sizeof(color.r) == 1)
		{
			cr = (unsigned char)color.r;
			cg = (unsigned char)color.g;
			cb = (unsigned char)color.b;
		}
		else if(sizeof(color.r) == 2)
		{
			cr = (unsigned short)(color.r)>>8;
			cg = (unsigned short)(color.g)>>8;
			cb = (unsigned short)(color.b)>>8;
		}
		else
		{
			cr = (unsigned char)(color.r * 255.0f);
			cg = (unsigned char)(color.g * 255.0f);
			cb = (unsigned char)(color.b * 255.0f);
		}
		return (cb << 16) | (cg << 8) | cr;
	}

}