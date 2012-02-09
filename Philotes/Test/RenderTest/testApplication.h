
#include "common.h"
#include "renderPrerequisites.h"
#include "gearsPrerequisites.h"
#include "renderWindow.h"
#include "gearsApplication.h"
#include "gearsComandLine.h"

_NAMESPACE_BEGIN

class TestApplication : public GearApplication
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

protected:

	bool				m_rewriteBuffers;    // flag for rewriting static buffers on device reset
	Matrix4				m_projection;

	RendererDebugLine*	m_debugLine;
	RendererGridShape*	m_debugGrid;

};

_NAMESPACE_END