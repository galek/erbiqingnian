
#include "common.h"
#include "renderPrerequisites.h"
#include "gearsPrerequisites.h"
#include "renderWindow.h"
#include "gearsApplication.h"
#include "gearsComandLine.h"

#include "input/gearsInput.h"

_NAMESPACE_BEGIN

class TestCameraManager;

class TestApplication : public	GearApplication
{
public:

	TestApplication(const GearCommandLine& cmdline);

	virtual		~TestApplication();

public:

	virtual	void		onInit(void);
	virtual	void		onShutdown(void);
	virtual	void		onTickPreRender(float dtime);
	virtual	void		onRender(void);
	virtual	void		onTickPostRender(float dtime);

	virtual int			getDisplayWidth(void);
	virtual int			getDisplayHeight(void);

	virtual bool		keyPressed( const KeyEvent &arg );
	virtual bool		keyReleased( const KeyEvent &arg );
	virtual bool 		mouseMoved( const MouseEvent &arg );
	virtual bool 		mousePressed( const MouseEvent &arg, MouseButtonID id );
	virtual bool 		mouseReleased( const MouseEvent &arg, MouseButtonID id );


protected:

	bool				m_rewriteBuffers;    // flag for rewriting static buffers on device reset
	Matrix4				m_projection;

	RenderDebugLine*	m_debugLine;
	RenderGridShape*	m_debugGrid;
	
	TestCameraManager*	m_cameraMgr;
};

_NAMESPACE_END