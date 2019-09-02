#pragma once

#include "Renderable.h"
#include "Light.h"
#include "Entity.h"
#include "View.h"
#include "Graphics.h"
#include "RenderTechnique.h"
#include "WorldManager.h"

#include <string>
#include <memory>
#include <vector>
#include <atlstr.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <DirectXMath.h>

using std::string;
using std::shared_ptr;
using std::make_shared;
using std::vector;
using glm::ivec4;

namespace Jarvis
{
  class Graphics;
  class Renderable;
  class Light;
  class Mesh;
  class WorldManager;

  class RenderTechnique
  {
  public:
    RenderTechnique(string name, WorldManager* worldManager, HINSTANCE hinstance, HWND window, shared_ptr<Graphics> graphics);
    ~RenderTechnique();

    void addRenderable(shared_ptr<Renderable> renderable, shared_ptr<Entity> entity);
    void removeRenderable(shared_ptr<Renderable> renderable, shared_ptr<Entity> entity);
    void addLight(shared_ptr<Light> light, shared_ptr<Entity> entity);
    void removeLight(shared_ptr<Light> light, shared_ptr<Entity> entity);
    size_t getNumLights();
    shared_ptr<Light> getLight(uint32_t index);
    void addView(shared_ptr<View> view);
    void removeView(shared_ptr<View> view);
    void updateWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void setClusterEntityFreeze(bool freeze);

    virtual void build();
    virtual void update();
    virtual void render();

  protected:
    string                                m_name;
    HINSTANCE                             m_hinstance;
    HWND                                  m_window;
    shared_ptr<Graphics>                  m_graphics;

    WorldManager*                         m_worldManager;
    vector<shared_ptr<Renderable>>        m_renderables;
    vector<shared_ptr<Entity>>            m_renderEntities;
    vector<shared_ptr<Light>>             m_lights;
    vector<shared_ptr<Entity>>            m_lightEntities;
    vector<shared_ptr<View>>              m_views;
    shared_ptr<View>                      m_onscreenView;

  private:
    void updateLightClusters(shared_ptr<View> view);
    void buildLightClusters(shared_ptr<View> view);
    vec4 planeEquation(DirectX::XMVECTOR p1, DirectX::XMVECTOR p2, DirectX::XMVECTOR p3);
    float updatePlaneD(vec4 plane, vec3 p);
    bool intersectsCluster(uint32_t clusterIndex, vec3 lightViewPosition, float radius);
    void renderMeshes(shared_ptr<View> view);

    struct LightData
    {
      mat4 light_view_projections[6];
      vec4 light_position;
      vec4 light_color;
    };

    struct FrameShaderParamBlock {
      ivec4 lightInfo;
      vec4 viewPosition;
      LightData lights[6];
    };

    struct ObjectShaderParamBlock {
      mat4 model;
      mat4 view_projection;
      vec4 albedoColor;
      vec4 emmisiveColor;
      vec4 metallicRoughness;
      vec4 flags;
    };

    struct Cluster {
      uint32_t                            m_verts[8];
	    vec4	                              m_planes[6];
      vector<shared_ptr<Light>>*          m_lights;
    };

    struct ClusterData {
      uint32_t  m_numClusterVerts;
      DirectX::XMVECTOR*     m_clusterVerts;
      float*    m_localVerts;
	    uint32_t  m_numXSegments;
	    uint32_t  m_numYSegments;
	    uint32_t  m_numZSegments;
      Cluster*  m_clusters;
    };

    size_t                                m_frameDataAlignedSize;
    size_t                                m_objectDataAlignedSize;
    size_t                                m_numMeshes;
    uint32_t*                             m_meshOffsets;
    int                                   m_currentLight;
    bool                                  m_depthPrepass;
    ClusterData*                          m_clusterData;
    shared_ptr<Entity>                    m_clusterEntity;
    bool                                  m_freezeClusterEntity;
  };
}
