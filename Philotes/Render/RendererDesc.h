
#ifndef RENDERER_DESC_H
#define RENDERER_DESC_H

#include <RendererConfig.h>
#include <Renderer.h>

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

#endif
