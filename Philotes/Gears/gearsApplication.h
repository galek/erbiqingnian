#pragma once

#include "renderWindow.h"
#include "util/timer.h"

_NAMESPACE_BEGIN

class GearApplication : public RenderWindow
{
public:

	GearApplication(const GearCommandLine& cmdline, const char* assetPathPrefix="media", MouseButton camMoveButton = MOUSE_LEFT);

	virtual						~GearApplication(void);

	Render*						getRender(void)								{ return m_renderer; }
	GearAssetManager*			getAssetManager(void)							{ return m_assetManager; }
	inline const char*			getAssetPathPrefix(void)				const	{ return m_assetPathPrefix; }
	Matrix4						getEyeTransform(void)					const	{ return m_worldToView.inverse(); }
	void						setEyeTransform(const Matrix4& eyeTransform);
	void						setEyeTransform(const Vector3& pos, const Vector3& rot);
	void						setViewTransform(const Matrix4& viewTransform);
	const Matrix4&				getViewTransform(void)					const;
	bool						isKeyDown(KeyCode keyCode)				const	{ return m_keyState[keyCode];			}
	bool						isMouseButtonDown(MouseButton button)	const	{ return m_mouseButtonState[button];	}

	virtual	void				onInit(void)					= 0;
	virtual	void				onShutdown(void)				= 0;
	virtual	void				onTickPreRender(float dtime)	= 0;
	virtual	void				onRender(void)					= 0;
	virtual	void				onTickPostRender(float dtime)	= 0;

	virtual	void				onOpen(void);
	virtual	bool				onClose(void);
	virtual	void				onDraw(void);

	virtual	void				onMouseMove(uint32 x, uint32 y);
	virtual	void				onMouseDown(uint32 /*x*/, uint32 /*y*/, MouseButton button);
	virtual	void				onMouseUp(uint32 /*x*/, uint32 /*y*/, MouseButton button);

	virtual	void				onKeyDown(KeyCode keyCode);
	virtual	void				onKeyUp(KeyCode keyCode);

	void						rotateCamera(scalar dx, scalar dy);
	virtual	void				doInput(void);

protected:

	const GearCommandLine&		m_cmdline;

	scalar						m_sceneSize;

	Render*					m_renderer;

	char						m_assetPathPrefix[256];
	GearAssetManager*			m_assetManager;

	Vector3						m_eyeRot;
	Matrix4						m_worldToView;

	uint32						m_mouseX, m_mouseY;
	bool						m_mouseButtonState[NUM_MOUSE_BUTTONS];
	bool						m_keyState[NUM_KEY_CODES];

	MouseButton					m_camMoveButton;

private:

	uint64						m_timeCounter;
	Timer						m_time;

	// get rid of stupid C4512
	void						operator=(const GearApplication&);

};	

_NAMESPACE_END