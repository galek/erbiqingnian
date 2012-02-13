#pragma once

#include "renderWindow.h"
#include "util/timer.h"
#include "input/gearsInput.h"

_NAMESPACE_BEGIN

class GearApplication : public	RenderWindow,
								GearKeyListener,
								GearMouseListener
{
public:

	GearApplication(const GearCommandLine& cmdline, const char* assetPathPrefix="media");

	virtual						~GearApplication(void);

	Render*						getRender(void)									{ return m_renderer; }
	GearAssetManager*			getAssetManager(void)							{ return m_assetManager; }
	inline const char*			getAssetPathPrefix(void)				const	{ return m_assetPathPrefix; }

	virtual	void				onInit(void)					= 0;
	virtual	void				onShutdown(void)				= 0;
	virtual	void				onTickPreRender(float dtime)	= 0;
	virtual	void				onRender(void)					= 0;
	virtual	void				onTickPostRender(float dtime)	= 0;

	virtual	void				onOpen(void);
	virtual	bool				onClose(void);
	virtual	void				onDraw(void);

	void						setupInput();
	void						shutdownInput();
	void						captureInput();

	virtual bool				keyPressed( const KeyEvent &arg )							= 0;
	virtual bool				keyReleased( const KeyEvent &arg )							= 0;
	virtual bool 				mouseMoved( const MouseEvent &arg )							= 0;
	virtual bool 				mousePressed( const MouseEvent &arg, MouseButtonID id )		= 0;
	virtual bool 				mouseReleased( const MouseEvent &arg, MouseButtonID id )	= 0;

protected:

	const GearCommandLine&		m_cmdline;

	scalar						m_sceneSize;

	Render*						m_renderer;

	RenderCamera*				m_camera;

	char						m_assetPathPrefix[256];

	GearAssetManager*			m_assetManager;

	GearInputManager*			m_inputManager;

	GearKeyboard*				m_keyboard;

	GearMouse*					m_mouse;

private:

	uint64						m_timeCounter;
	Timer						m_time;

	// get rid of stupid C4512
	void						operator=(const GearApplication&);

};	

_NAMESPACE_END