
#include "renderSceneManager.h"
#include "gearsApplication.h"
#include "renderCamera.h"
#include "renderCellNode.h"

_NAMESPACE_BEGIN

RenderSceneManager::RenderSceneManager()
{
	m_renderer = GearApplication::getApp()->getRender();
	m_camera = ph_new(RenderCamera)("main_cam");
	m_camera->setNearClipDistance(0.1f);
	m_camera->setFarClipDistance(10000.0f);

	m_rootNode = NULL;
}

RenderSceneManager::~RenderSceneManager()
{
	ph_delete(m_camera);
}

void RenderSceneManager::tickVisible( const RenderCamera* camera )
{
	// 不进行裁剪，全部渲染
	if (m_rootNode)
	{
		m_rootNode->tickVisible(camera);
	}
}

RenderCellNode* RenderSceneManager::getRootCellNode()
{
	if (!m_rootNode)
	{
		m_rootNode = _createCellNodeImpl("_root_cn");
	}
	return m_rootNode;
}

RenderCellNode* RenderSceneManager::createCellNode( const String& name )
{
	if (m_cells.Contains(name))
	{
		PH_EXCEPT(ERR_RENDER,"A cell node with the name " + name + " already exists");
	}
	RenderCellNode* cn = _createCellNodeImpl(name);
	m_cells[name] = cn;
	return cn;
}

RenderCellNode* RenderSceneManager::createCellNode()
{
	RenderCellNode* cn = _createCellNodeImpl();
	ph_assert(!m_cells.Contains(cn->getName()));
	m_cells[cn->getName()] = cn;
	return cn;
}

RenderCellNode* RenderSceneManager::_createCellNodeImpl( const String& name )
{
	return ph_new(RenderCellNode(this,name));
}

RenderCellNode* RenderSceneManager::_createCellNodeImpl()
{
	return ph_new(RenderCellNode(this));
}

void RenderSceneManager::destroyCellNode( const String& name )
{
	IndexT i = m_cells.FindIndex(name);
	if (i == InvalidIndex)
	{
		PH_EXCEPT(ERR_RENDER,"Cell node not found.: RenderSceneManager::destroyCellNode");
	}

	RenderCellNode* node = m_cells.ValueAtIndex(i);
	RenderNode* parentNode = node->getParent();
	if (parentNode)
	{
		parentNode->removeChild(node);
	}
	
	ph_delete(node);

	m_cells.EraseAtIndex(i);
}

void RenderSceneManager::destroyCellNode( RenderCellNode* node )
{
	destroyCellNode(node->getName());
}

_NAMESPACE_END


