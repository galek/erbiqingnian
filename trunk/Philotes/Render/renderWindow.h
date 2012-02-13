
#ifndef RENDERER_WINDOW_H
#define RENDERER_WINDOW_H

_NAMESPACE_BEGIN

class GearPlatformUtil;

class RenderWindow
{
	public:
		RenderWindow(void);

		virtual ~RenderWindow(void);
		
		bool open(uint32 width, uint32 height, const char *title, bool fullscreen=false);

		void close(void);
		
		bool isOpen(void) const;
		
		// update the window's state... handle messages, etc.
		void update(void);
		
		// get/set the size of the window...
		void getSize(uint32 &width, uint32 &height) const;

		void setSize(uint32 width, uint32 height);
		
		// get/set the window's title...
		void getTitle(char *title, uint32 maxLength) const;

		void setTitle(const char *title);
		
	public:
		// called just AFTER the window opens.
		virtual void onOpen(void) {}
		
		// called just BEFORE the window closes. return 'true' to confirm the window closure.
		virtual bool onClose(void) { return true; }
		
		// called just AFTER the window is resized.
		virtual void onResize(uint32 /*width*/, uint32 /*height*/) {}
		
		// called when the window's contents needs to be redrawn.
		virtual void onDraw(void) = 0;

	private:
		bool					m_isOpen;
};

_NAMESPACE_END

#endif
