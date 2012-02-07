
#ifndef RENDERER_COLOR_H
#define RENDERER_COLOR_H

#include <RendererConfig.h>

class RendererColor
{
	public:
		uint8 b, g, r, a;
	
	public:
		RendererColor(void);
		RendererColor(uint8 r, uint8 g, uint8 b, uint8 a);
		RendererColor(uint32 rgba);
		
		// switches between RGBA and BGRA.
		inline void swizzleRB(void)
		{
			uint8 t = b;
			b = r;
			r = t;
		}
};

inline RendererColor lerp( const RendererColor& start, const RendererColor& end, float s )
{
    return RendererColor(
        start.r + uint8(( end.r - start.r ) * s),
        start.g + uint8(( end.g - start.g ) * s),
        start.b + uint8(( end.b - start.b ) * s),
        start.a + uint8(( end.a - start.a ) * s));
}

#endif
