
#ifndef RENDERER_COLOR_H
#define RENDERER_COLOR_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class RenderColor
{
	public:
		uint8 b, g, r, a;
	
	public:
		RenderColor(void);
		RenderColor(uint8 r, uint8 g, uint8 b, uint8 a);
		RenderColor(uint32 rgba);
		
		// switches between RGBA and BGRA.
		inline void swizzleRB(void)
		{
			uint8 t = b;
			b = r;
			r = t;
		}
};

inline RenderColor lerp( const RenderColor& start, const RenderColor& end, float s )
{
    return RenderColor(
        start.r + uint8(( end.r - start.r ) * s),
        start.g + uint8(( end.g - start.g ) * s),
        start.b + uint8(( end.b - start.b ) * s),
        start.a + uint8(( end.a - start.a ) * s));
}

_NAMESPACE_END

#endif
