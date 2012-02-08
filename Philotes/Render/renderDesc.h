
#ifndef RENDERER_DESC_H
#define RENDERER_DESC_H

#include <renderConfig.h>
#include <render.h>

_NAMESPACE_BEGIN

class RendererWindow;

class RendererDesc
{
public:
	Renderer::DriverType driver;

	uint64	windowHandle;

public:
	RendererDesc(void);
	
	bool isValid(void) const;
};

_NAMESPACE_END

#endif
