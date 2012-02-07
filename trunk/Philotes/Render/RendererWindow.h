
#ifndef RENDERER_WINDOW_H
#define RENDERER_WINDOW_H

#include <RendererConfig.h>

class SamplePlatform;

class RendererWindow
{
	public:
		typedef enum MouseButton
		{
			MOUSE_LEFT = 0,
			MOUSE_RIGHT,
			MOUSE_CENTER,
			
			NUM_MOUSE_BUTTONS,
		};
		
		typedef enum KeyCode
		{
			KEY_UNKNOWN = 0,

			KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
			KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
			KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
			KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

			KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, 
			KEY_7, KEY_8, KEY_9,

			KEY_SPACE, KEY_RETURN, KEY_SHIFT, KEY_CONTROL, KEY_ESCAPE, KEY_COMMA, 
			KEY_NUMPAD0, KEY_NUMPAD1, KEY_NUMPAD2, KEY_NUMPAD3, KEY_NUMPAD4, KEY_NUMPAD5, KEY_NUMPAD6, KEY_NUMPAD7, KEY_NUMPAD8, KEY_NUMPAD9,
			KEY_MULTIPLY, KEY_ADD, KEY_SEPARATOR, KEY_SUBTRACT, KEY_DECIMAL, KEY_DIVIDE,

			KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
			KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

			KEY_TAB, KEY_PRIOR, KEY_NEXT,
			KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

			NUM_KEY_CODES,
		};

		typedef enum GamepadAxis
		{
			AXIS_UNKNOWN = 0,
			AXIS_RIGHT_HORIZONTAL,
			AXIS_RIGHT_VERTICAL,
			AXIS_LEFT_HORIZONTAL,
			AXIS_LEFT_VERTICAL,

			NUM_GAMEPAD_AXIS,
		};

		bool keyCodeToASCII( KeyCode code, char& c )
		{
			if( code >= KEY_A && code <= KEY_Z )
			{
				c = (char)code + 'A' - 1;
			}
			else if( code >= KEY_0 && code <= KEY_9 )
			{
				c = (char)code + '0' - 1;
			}
			else if( code == KEY_SPACE )
			{
				c = ' ';
			}
			else
			{
				return false;
			}
			return true;
		}
		
	public:
		RendererWindow(void);
		virtual ~RendererWindow(void);
		
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
		
		virtual void onMouseMove(uint32 /*x*/, uint32 /*y*/) {}
		
		virtual void onMouseDown(uint32 /*x*/, uint32 /*y*/, MouseButton /*button*/) {}
		virtual void onMouseUp(  uint32 /*x*/, uint32 /*y*/, MouseButton /*button*/) {}
		
		virtual void onKeyDown(KeyCode /*keyCode*/) {}
		virtual void onKeyUp(  KeyCode /*keyCode*/) {}
		
		// called when the window's contents needs to be redrawn.
		virtual void onDraw(void) = 0;

	protected:
		SamplePlatform*				m_platform;
	private:
		bool						m_isOpen;
};

#endif
