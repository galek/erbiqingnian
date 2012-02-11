
#include "testApplication.h"
#include "render.h"

#include "debug/renderDebugLine.h"
#include "debug/renderDebugGrid.h"

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

	// »æÖÆline²âÊÔ
	m_debugLine = new RenderDebugLine(*m_renderer,*m_assetManager);

	m_debugLine->clearLine();
	for (size_t i=0; i<100; i++)
	{
		Vector3 vec0 = Vector3(-(float)(rand()%10),-(float)(rand()%10),-(float)(rand()%10));
		Vector3 vec1 = Vector3((float)(rand()%10),(float)(rand()%10),(float)(rand()%10));

		m_debugLine->addLine(vec0,vec1,Colour(1,0,0,1));
	}

	m_debugGrid = new RenderGridShape(*m_renderer,100,40);
}

void TestApplication::onShutdown( void )
{
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

			//renderer->queueMeshForRender(m_debugGrid->getMesh()->)

			// äÖÈ¾³¡¾°
			renderer->render(getEyeTransform(), m_projection);

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

_NAMESPACE_END
