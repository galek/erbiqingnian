
#ifndef RENDERER_DESC_H
#define RENDERER_DESC_H

#include <renderConfig.h>
#include <render.h>

_NAMESPACE_BEGIN

class RenderWindow;

class RenderDesc
{
public:
	Render::DriverType driver;

	uint64	windowHandle;

public:
	RenderDesc(void);
	
	bool isValid(void) const;
};

_NAMESPACE_END

#endif
