
#include <renderWindow.h>
#include <renderMemoryMacros.h>
#include "gearsPlatform.h"
#include <stdio.h>

_NAMESPACE_BEGIN

RenderWindow::RenderWindow(void)
{
	m_platform = GearPlatform::getSingleton();
	if (!m_platform)
	{
		m_platform = new GearPlatform(this);
	}

	m_isOpen = false;
}

RenderWindow::~RenderWindow(void)
{
	DELETESINGLE(m_platform);
}

bool RenderWindow::open(uint32 width, uint32 height, const char *title, bool fullscreen)
{
	bool ok = false;
	ph_assert2(width && height, "Attempting to open a window with invalid width and/or height.");
	if(width && height)
	{
		ok = m_platform->openWindow(width, height, title, fullscreen);
	}
	return ok;
}

void RenderWindow::close(void)
{
	m_platform->closeWindow();
}

bool RenderWindow::isOpen(void) const
{
	bool open = m_platform->isOpen();
	return open;
}

// update the window's state... handle messages, etc.
void RenderWindow::update(void)
{
	m_platform->update();

	if(isOpen())
	{
		onDraw();
	}
}

// get/set the size of the window...
void RenderWindow::getSize(uint32 &width, uint32 &height) const
{
	m_platform->getWindowSize(width, height);
}

void RenderWindow::setSize(uint32 width, uint32 height)
{
	m_platform->setWindowSize(width, height);
}

// get the window's title...
void RenderWindow::getTitle(char *title, uint32 maxLength) const
{
	m_platform->getTitle(title, maxLength);
}

// set the window's title...
void RenderWindow::setTitle(const char *title)
{
	m_platform->setTitle(title);
}

_NAMESPACE_END