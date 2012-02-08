
#include <renderWindow.h>
#include <renderMemoryMacros.h>
#include <stdio.h>

_NAMESPACE_BEGIN

RendererWindow::RendererWindow(void)
{
	m_platform = createPlatform(this);
	m_isOpen = false;
}

RendererWindow::~RendererWindow(void)
{
	DELETESINGLE(m_platform);
}

bool RendererWindow::open(uint32 width, uint32 height, const char *title, bool fullscreen)
{
	bool ok         = false;
	ph_assert2(width && height, "Attempting to open a window with invalid width and/or height.");
	if(width && height)
	{
		ok = m_platform->openWindow(width, height, title, fullscreen);
	}
	return ok;
}

void RendererWindow::close(void)
{
	m_platform->closeWindow();
}

bool RendererWindow::isOpen(void) const
{
	bool open = m_platform->isOpen();
	return open;
}

// update the window's state... handle messages, etc.
void RendererWindow::update(void)
{
	m_platform->update();

	if(isOpen())
	{
		onDraw();
	}
}

// get/set the size of the window...
void RendererWindow::getSize(uint32 &width, uint32 &height) const
{
	m_platform->getWindowSize(width, height);
}

void RendererWindow::setSize(uint32 width, uint32 height)
{
	m_platform->setWindowSize(width, height);
}

// get the window's title...
void RendererWindow::getTitle(char *title, uint32 maxLength) const
{
	m_platform->getTitle(title, maxLength);
}

// set the window's title...
void RendererWindow::setTitle(const char *title)
{
	m_platform->setTitle(title);
}

_NAMESPACE_END