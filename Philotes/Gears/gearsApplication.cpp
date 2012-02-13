
#include "gearsApplication.h"
#include "gearsPlatformUtil.h"
#include "gearsAssetManager.h"
#include "gearsComandLine.h"

#include "render.h"
#include "renderCamera.h"
#include "renderMemoryMacros.h"

_NAMESPACE_BEGIN

char gShadersDir[1024];
char gAssetDir[1024];

GearApplication* GearApplication::m_sApp = NULL;

GearApplication::GearApplication(const GearCommandLine &cmdline, const char *assetPathPrefix) :
	m_cmdline(cmdline)
{
	ph_assert2(!m_sApp,"m_sApp != NULL");
	m_sApp = this;

	GearPlatformUtil::getSingleton()->correctCurrentDir();

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
	m_timeCounter  = 0;
	m_camera = new RenderCamera("maincam");

	m_inputManager = 0;
	m_keyboard = 0;
	m_mouse = 0;
}

GearApplication::~GearApplication(void)
{
	delete m_camera;
	ph_assert2(!m_renderer, "Render was not released prior to window closure.");
	ph_assert2(!m_assetManager, "Asset Manager was not released prior to window closure.");
}

void GearApplication::onOpen( void )
{
	GearPlatformUtil::getSingleton()->preRenderSetup();

	char rendererdir[1024];
	strcpy_s(rendererdir, sizeof(rendererdir), m_assetPathPrefix);
	strcat_s(rendererdir, sizeof(rendererdir), "/");
	strcpy_s(gAssetDir, sizeof(gAssetDir), rendererdir);

	strcpy_s(gShadersDir, sizeof(gShadersDir), rendererdir);
	strcat_s(gShadersDir, sizeof(gShadersDir), "shaders/");

	m_renderer = Render::createRender(GearPlatformUtil::getSingleton()->getWindowHandle());
	GearPlatformUtil::getSingleton()->postRenderSetup();

	m_timeCounter = m_time.getCurrentCounterValue();

	m_assetManager = new GearAssetManager(*m_renderer);
	m_assetManager->addSearchPath(m_assetPathPrefix);
	m_assetManager->addSearchPath(rendererdir);

	setupInput();

	onInit();
}

bool GearApplication::onClose( void )
{
	onShutdown();

	shutdownInput();

	DELETESINGLE(m_assetManager);
	SAFE_RELEASE(m_renderer);

	GearPlatformUtil::getSingleton()->postRenderRelease();

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
		// ¸üÐÂÊäÈë
		captureInput();

		onTickPreRender(dtime);
		
		if(m_renderer)
		{
			onRender();
		}
		
		onTickPostRender(dtime);
	}
}

void GearApplication::setupInput()
{
	ParamList pl;
	size_t winHandle = 0;

	String hwnd;
	hwnd.SetInt64(GearPlatformUtil::getSingleton()->getWindowHandle());
	pl.insert(std::make_pair("WINDOW", hwnd.c_str()));
	pl.insert(std::make_pair("x11_keyboard_grab", "false"));
	pl.insert(std::make_pair("x11_mouse_grab", "false"));
	pl.insert(std::make_pair("w32_mouse", "DISCL_FOREGROUND"));
	pl.insert(std::make_pair("w32_mouse", "DISCL_NONEXCLUSIVE"));
	pl.insert(std::make_pair("w32_keyboard", "DISCL_FOREGROUND"));
	pl.insert(std::make_pair("w32_keyboard", "DISCL_NONEXCLUSIVE"));

	m_inputManager = GearInputManager::createInputSystem(pl);

	GearInputObject* obj = m_inputManager->createInputObject(PI_Keyboard, true);
	m_keyboard = static_cast<GearKeyboard*>(obj);
	m_mouse = static_cast<GearMouse*>(m_inputManager->createInputObject(PI_Mouse, true));

	m_keyboard->setEventCallback(this);
	m_mouse->setEventCallback(this);
}

void GearApplication::shutdownInput()
{
	if (m_inputManager)
	{
		m_inputManager->destroyInputObject(m_keyboard);
		m_inputManager->destroyInputObject(m_mouse);

		GearInputManager::destroyInputSystem(m_inputManager);
		m_inputManager = NULL;
	}
}

void GearApplication::captureInput()
{
	m_mouse->capture();
	m_keyboard->capture();
}

GearApplication* GearApplication::getApp()
{
	return m_sApp;
}


_NAMESPACE_END