
#include <renderWindow.h>
#include <renderMemoryMacros.h>
#include "gearsPlatformUtil.h"
#include <stdio.h>

_NAMESPACE_BEGIN

RenderWindow::RenderWindow(void)
{
	GearPlatformUtil* platform = GearPlatformUtil::getSingleton();
	if (!platform)
	{
		platform = new GearPlatformUtil(this);
	}

	m_isOpen = false;
}

RenderWindow::~RenderWindow(void)
{
	GearPlatformUtil* platform = GearPlatformUtil::getSingleton();
	if (platform)
	{
		delete platform;
	}
}

bool RenderWindow::open(uint32 width, uint32 height, const char *title, bool fullscreen)
{
	bool ok = false;
	ph_assert2(width && height, "Attempting to open a window with invalid width and/or height.");
	if(width && height)
	{
		ok = GearPlatformUtil::getSingleton()->openWindow(width, height, title, fullscreen);
	}
	return ok;
}

void RenderWindow::close(void)
{
	GearPlatformUtil::getSingleton()->closeWindow();
}

bool RenderWindow::isOpen(void) const
{
	bool open = GearPlatformUtil::getSingleton()->isOpen();
	return open;
}

// update the window's state... handle messages, etc.
void RenderWindow::update(void)
{
	GearPlatformUtil::getSingleton()->update();

	if(isOpen())
	{
		onDraw();
	}
}

// get/set the size of the window...
void RenderWindow::getSize(uint32 &width, uint32 &height) const
{
	GearPlatformUtil::getSingleton()->getWindowSize(width, height);
}

void RenderWindow::setSize(uint32 width, uint32 height)
{
	GearPlatformUtil::getSingleton()->setWindowSize(width, height);
}

// get the window's title...
void RenderWindow::getTitle(char *title, uint32 maxLength) const
{
	GearPlatformUtil::getSingleton()->getTitle(title, maxLength);
}

// set the window's title...
void RenderWindow::setTitle(const char *title)
{
	GearPlatformUtil::getSingleton()->setTitle(title);
}

_NAMESPACE_END