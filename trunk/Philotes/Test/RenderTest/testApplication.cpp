
#include "testApplication.h"
#include "render.h"
#include "renderCamera.h"

#include "debug/renderDebugLine.h"
#include "debug/renderDebugGrid.h"

#include "testCamera.h"

_NAMESPACE_BEGIN

TestApplication::TestApplication( const GearCommandLine& cmdline )
	: GearApplication(cmdline),
	m_debugLine(NULL),
	m_debugGrid(NULL)
{

}

TestApplication::~TestApplication()
{

}

void TestApplication::onInit( void )
{	
	srand(::GetTickCount());

	m_cameraMgr = new TestCameraManager(m_camera);

	// ����line����
	m_debugLine = new RenderDebugLine(*m_renderer,*m_assetManager);

	m_debugLine->clearLine();
	for (size_t i=0; i<100; i++)
	{
		Vector3 vec0 = Vector3(-(float)(rand()%10),-(float)(rand()%10),-(float)(rand()%10));
		Vector3 vec1 = Vector3((float)(rand()%10),(float)(rand()%10),(float)(rand()%10));

		m_debugLine->addLine(vec0,vec1,Colour(1,0,0,1));
	}

	m_debugGrid = new RenderGridShape(*m_renderer,100,40);

	m_camera->setNearClipDistance(0.1f);
	m_camera->setFarClipDistance(10000.0f);
}

void TestApplication::onShutdown( void )
{
	delete m_cameraMgr;
	m_cameraMgr = NULL;

	delete m_debugLine;
	m_debugLine = NULL;

	delete m_debugGrid;
	m_debugGrid = NULL;
}

void TestApplication::onTickPreRender( float dtime )
{

}

void TestApplication::onRender( void )
{
	Render* renderer = getRender();

	if (renderer)
	{
		uint32 windowWidth  = 0;
		uint32 windowHeight = 0;
		getSize(windowWidth, windowHeight);
		if (windowWidth > 0 && windowHeight > 0)
		{
			renderer->clearBuffers();

			Math::makeProjectionMatrix(Radian(Degree(45)),windowWidth / (float)windowHeight, 0.1f, 10000.0f,m_projection);

			m_debugLine->queueForRenderLine();

			// ��Ⱦ����
			renderer->render(m_camera);

			m_rewriteBuffers = renderer->swapBuffers();
		}
	}
}

void TestApplication::onTickPostRender( float dtime )
{

}

int TestApplication::getDisplayWidth( void )
{
	return 800;
}

int TestApplication::getDisplayHeight( void )
{
	return 600;
}

bool TestApplication::keyPressed( const KeyEvent &arg )
{
	m_cameraMgr->injectKeyDown(arg);
	return true;
}

bool TestApplication::keyReleased( const KeyEvent &arg )
{
	m_cameraMgr->injectKeyUp(arg);
	return true;
}

bool TestApplication::mouseMoved( const MouseEvent &arg )
{
	m_cameraMgr->injectMouseMove(arg);
	return true;
}

bool TestApplication::mousePressed( const MouseEvent &arg, MouseButtonID id )
{
	m_cameraMgr->injectMouseDown(arg,id);
	return true;
}

bool TestApplication::mouseReleased( const MouseEvent &arg, MouseButtonID id )
{
	m_cameraMgr->injectMouseUp(arg,id);
	return true;
}

_NAMESPACE_END