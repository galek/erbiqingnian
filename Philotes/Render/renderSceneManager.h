
#pragma once

_NAMESPACE_BEGIN

class RenderSceneManager
{
public:

	RenderSceneManager();

	virtual ~RenderSceneManager();

public:

	// 场景管理，裁剪查找出可见物体
	virtual void tickVisible(const RenderCamera* camera);

	RenderCamera* getCamera(){return m_camera;}

	RenderCellNode* getRootCellNode();

	virtual RenderCellNode* createCellNode(const String& name);

	virtual RenderCellNode* createCellNode();

	virtual void destroyCellNode(const String& name);

	virtual void destroyCellNode(RenderCellNode* node);

	typedef Dictionary<String,RenderCellNode*> CellMap;

protected:

	virtual RenderCellNode* _createCellNodeImpl(const String& name);

	virtual RenderCellNode* _createCellNodeImpl();

protected:

	Render* m_renderer;

	RenderCamera* m_camera;

	RenderCellNode* m_rootNode;

	CellMap m_cells;
};

_NAMESPACE_END