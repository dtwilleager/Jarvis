#pragma once

#include "Entity.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "RenderScreenView.h"
#include "Processor.h"
#include "Renderable.h"
#include "RenderTechnique.h"
#include "Light.h"
#include "Graphics.h"
#include "CpuTimer.h"
#include "ModelLoader.h"

#include <string>
#include <vector>
#include <memory>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h> 
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

using std::string;
using std::vector;
using std::shared_ptr;
using std::map;

namespace Jarvis
{
  class RenderTechnique;
  class Graphics;
  class Renderable;
  class ModelLoader;

  class WorldManager
  {
  public:
    WorldManager(string name, HINSTANCE hinstance, HWND window);
    ~WorldManager();

    void                addEntity(shared_ptr<Entity> entity);
    void                removeEntity(shared_ptr<Entity> entity);
    shared_ptr<Entity>  getEntity(unsigned int index);
    size_t              numEntities();

    void                addView(shared_ptr<RenderScreenView> view);
    void                removeView(shared_ptr<RenderScreenView> view);
    shared_ptr<RenderScreenView>    getView(unsigned int index);
    size_t              numViews();

    void                handleKeyboard(MSG* event);
    void                handleMouse(MSG* event);

    shared_ptr<Entity>  loadAssimpModel(string filename);

    void                buildFrame();
    void                executeFrame();


    void                updateWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void                printLog(string s);

  private:
    string      m_name;

    vector<shared_ptr<Entity>>                m_entities;
    vector<shared_ptr<RenderScreenView>>      m_views;
    vector<shared_ptr<Processor>>             m_processors;
    shared_ptr<Graphics>                      m_graphics;
    shared_ptr<RenderTechnique>               m_renderTechnique;
    shared_ptr<ModelLoader>                   m_modelLoader;
    CpuTimer                                  m_timer;
    unsigned long long                        m_frameStartTime;
    unsigned long long                        m_lastFrameStartTime;
    unsigned long long                        m_totalRenderTime;
    unsigned long long                        m_totalProcessTime;
    uint32_t                                  m_frameCount;
    float                                     m_constantDepthBias;
    float                                     m_slopeDepthBias;
    bool                                      m_clusterEntityFreeze;

    void processAddEntity(shared_ptr<Entity> entity);
    void processRemoveEntity(shared_ptr<Entity> entity);
    void addProcessor(shared_ptr<Processor> processor);
    void addLight(shared_ptr<Light> light, shared_ptr<Entity> entity);
    void addRenderable(shared_ptr<Renderable> renderable, shared_ptr<Entity> entity);
    void removeProcessor(shared_ptr<Processor> processor);
    void removeLight(shared_ptr<Light> light, shared_ptr<Entity> entity);
    void removeRenderable(shared_ptr<Renderable> renderable, shared_ptr<Entity> entity);


    void updateTransforms();
    void updateTransform(shared_ptr<Entity> entity, mat4& parent);
  };
}

