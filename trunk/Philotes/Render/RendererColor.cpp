
#include <RendererColor.h>

RendererColor::RendererColor(void)
{

}

RendererColor::RendererColor(uint8 _r, uint8 _g, uint8 _b, uint8 _a)
{
	r=_r;
	g=_g;
	b=_b;
	a=_a;
}

RendererColor::RendererColor(uint32 rgba)
{
	r = (uint8)((rgba>>16) & 0xff);
	g = (uint8)((rgba>>8)  & 0xff);
	b = (uint8)((rgba)     & 0xff);
	a = 255;
}
