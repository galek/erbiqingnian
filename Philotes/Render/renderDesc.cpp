
#include <renderDesc.h>

_NAMESPACE_BEGIN

RenderDesc::RenderDesc(void)
{
#if defined(RENDERER_ENABLE_OPENGL)
	driver = Render::DRIVER_OPENGL;
#elif defined(RENDERER_ENABLE_DIRECT3D9)
	driver = Render::DRIVER_DIRECT3D9;
#elif defined(RENDERER_ENABLE_DIRECT3D10)
	driver = Render::DRIVER_DIRECT3D10;
#elif defined(RENDERER_ENABLE_LIBGCM)
	driver = Render::DRIVER_LIBGCM;
#else
	#error "No Render Drivers support!"
#endif
	windowHandle   = 0;
}
		
bool RenderDesc::isValid(void) const
{
	bool ok = true;
#if defined(RENDERER_WINDOWS)
	if(!windowHandle) ok = false;
#endif
	return ok;
}

_NAMESPACE_END