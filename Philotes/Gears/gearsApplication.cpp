
#include "gearsApplication.h"
#include "gearsPlatform.h"
#include "gearsAssetManager.h"
#include "gearsComandLine.h"

#include "render.h"
#include "renderCamera.h"
#include "renderDesc.h"
#include "renderMemoryMacros.h"

_NAMESPACE_BEGIN

char gShadersDir[1024];
char gAssetDir[1024];

GearApplication::GearApplication(const GearCommandLine &cmdline, const char *assetPathPrefix, MouseButton camMoveButton) :
	m_cmdline(cmdline)
{
	m_platform->correctCurrentDir();

	if (assetPathPrefix)
	{
		if (!GearAssetManager::searchForPath(assetPathPrefix, m_assetPathPrefix, sizeof(m_assetPathPrefix)/sizeof(m_assetPathPrefix[0]), 20))
		{
			ph_assert2(false, "assetPathPrefix could not be found in any of the parent directories!");
			m_assetPathPrefix[0] = 0;
		}
	}
	else
	{
		ph_assert2(assetPathPrefix, "assetPathPrefix must not be NULL (try \"media\" instead)");
		m_assetPathPrefix[0] = 0;
	}

	m_renderer     = 0;
	m_sceneSize    = 1.0f;
	m_assetManager = 0;
	m_mouseX       = 0;
	m_mouseY       = 0;
	m_timeCounter  = 0;
	memset(m_mouseButtonState, 0, sizeof(m_mouseButtonState));
	memset(m_keyState,         0, sizeof(m_keyState));
	m_camMoveButton = camMoveButton;

	m_camera = new RenderCamera("maincam");
}

GearApplication::~GearApplication(void)
{
	delete m_camera;
	ph_assert2(!m_renderer, "Render was not released prior to window closure.");
	ph_assert2(!m_assetManager, "Asset Manager was not released prior to window closure.");
}

void GearApplication::onOpen( void )
{
	m_platform->preRenderSetup();

	memset(m_mouseButtonState, 0, sizeof(m_mouseButtonState));
	memset(m_keyState,         0, sizeof(m_keyState));

	char rendererdir[1024];
	strcpy_s(rendererdir, sizeof(rendererdir), m_assetPathPrefix);
	strcat_s(rendererdir, sizeof(rendererdir), "/");
	strcpy_s(gAssetDir, sizeof(gAssetDir), rendererdir);

	strcpy_s(gShadersDir, sizeof(gShadersDir), rendererdir);
	strcat_s(gShadersDir, sizeof(gShadersDir), "shaders/");

	m_renderer = Render::createRender(m_platform->getWindowHandle());
	m_platform->postRenderSetup();

	m_timeCounter = m_time.getCurrentCounterValue();

	m_assetManager = new GearAssetManager(*m_renderer);
	m_assetManager->addSearchPath(m_assetPathPrefix);
	m_assetManager->addSearchPath(rendererdir);

	onInit();
}

bool GearApplication::onClose( void )
{
	onShutdown();
	DELETESINGLE(m_assetManager);

	SAFE_RELEASE(m_renderer);
	m_platform->postRenderRelease();

	return true;
}

void GearApplication::onDraw( void )
{
	uint64 qpc = m_time.getCurrentCounterValue();
	float dtime = static_cast<float>((double)(qpc - m_timeCounter) / (double)m_time.sCounterFreq.mDenominator);
	m_timeCounter = qpc;
	ph_assert(dtime >= 0);
	if(dtime > 0)
	{
		onTickPreRender(dtime);
		
		if(m_renderer)
		{
			onRender();
		}
		
		onTickPostRender(dtime);

		// update scene...
		if(m_keyState[KEY_W]) 
		{
			m_camera->moveRelative(Vector3(0,0,0.01f));
		}
		if(m_keyState[KEY_A]) 
		{
			m_camera->moveRelative(Vector3(-0.01f,0,0));
		}
		if(m_keyState[KEY_S]) 
		{
			m_camera->moveRelative(Vector3(0,0,-0.01f));
		}
		if(m_keyState[KEY_D]) 
		{
			m_camera->moveRelative(Vector3(0.01f,0,0));
		}
	}
}

void GearApplication::rotateCamera( scalar dx, scalar dy )
{
	const float eyeCap      = 1.5f;

	const float eyeRotSpeed = 0.005f;
	m_eyeRot.x -= dy * eyeRotSpeed;
	m_eyeRot.y += dx * eyeRotSpeed;
	if(m_eyeRot.x >  eyeCap) 
	{
		m_eyeRot.x =  eyeCap;
	}
	if(m_eyeRot.x < -eyeCap)
	{
		m_eyeRot.x = -eyeCap;
	}
}

void GearApplication::doInput( void )
{
	m_platform->doInput();
}

void GearApplication::onMouseMove( uint32 x, uint32 y )
{
	if(m_mouseButtonState[m_camMoveButton])
	{
		uint32 dx = ((uint32)x)-(uint32)m_mouseX;
		uint32 dy = ((uint32)y)-(uint32)m_mouseY;
		rotateCamera((scalar)dx, (scalar)dy);
	}
	m_mouseX = x;
	m_mouseY = y;
}

void GearApplication::onMouseDown( uint32 /*x*/, uint32 /*y*/, MouseButton button )
{
	m_mouseButtonState[button] = true;
}

void GearApplication::onMouseUp( uint32 /*x*/, uint32 /*y*/, MouseButton button )
{
	m_mouseButtonState[button] = false;
}

void GearApplication::onKeyDown( KeyCode keyCode )
{
	m_keyState[keyCode] = true;
}

void GearApplication::onKeyUp( KeyCode keyCode )
{
	m_keyState[keyCode] = false;
}

_NAMESPACE_END