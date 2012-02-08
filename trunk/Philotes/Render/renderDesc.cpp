
#include <renderDesc.h>

_NAMESPACE_BEGIN

RendererDesc::RendererDesc(void)
{
#if defined(RENDERER_ENABLE_OPENGL)
	driver = Renderer::DRIVER_OPENGL;
#elif defined(RENDERER_ENABLE_DIRECT3D9)
	driver = Renderer::DRIVER_DIRECT3D9;
#elif defined(RENDERER_ENABLE_DIRECT3D10)
	driver = Renderer::DRIVER_DIRECT3D10;
#elif defined(RENDERER_ENABLE_LIBGCM)
	driver = Renderer::DRIVER_LIBGCM;
#else
	#error "No Renderer Drivers support!"
#endif
	windowHandle   = 0;
}
		
bool RendererDesc::isValid(void) const
{
	bool ok = true;
#if defined(RENDERER_WINDOWS)
	if(!windowHandle) ok = false;
#endif
	return ok;
}

_NAMESPACE_END