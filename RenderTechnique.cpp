#include "stdafx.h"
#include "RenderTechnique.h"

namespace Jarvis
{
  RenderTechnique::RenderTechnique(string name, WorldManager* worldManager, HINSTANCE hinstance, HWND window, shared_ptr<Graphics> graphics):
    m_name(name),
    m_worldManager(worldManager),
    m_hinstance(hinstance),
    m_window(window),
    m_graphics(graphics),
    m_currentLight(0),
    m_depthPrepass(false),
    m_clusterData(nullptr),
    m_freezeClusterEntity(false)
  {
  }


  RenderTechnique::~RenderTechnique()
  {
  }

  void RenderTechnique::addRenderable(shared_ptr<Renderable> renderable, shared_ptr<Entity> entity)
  {
    m_renderables.push_back(renderable);
    m_renderEntities.push_back(entity);
  }

  void RenderTechnique::removeRenderable(shared_ptr<Renderable> renderable, shared_ptr<Entity> entity)
  {
    for (vector<shared_ptr<Renderable>>::iterator it = m_renderables.begin(); it != m_renderables.end(); ++it)
    {
      if (*it == renderable)
      {
        m_renderables.erase(it);
        return;
      }
    }

    for (vector<shared_ptr<Entity>>::iterator it = m_renderEntities.begin(); it != m_renderEntities.end(); ++it)
    {
      if (*it == entity)
      {
        m_renderEntities.erase(it);
        return;
      }
    }
  }

  void RenderTechnique::addLight(shared_ptr<Light> light, shared_ptr<Entity> entity)
  {
    m_lights.push_back(light);
    m_lightEntities.push_back(entity);
  }

  void RenderTechnique::removeLight(shared_ptr<Light> light, shared_ptr<Entity> entity)
  {
    for (vector<shared_ptr<Light>>::iterator it = m_lights.begin(); it != m_lights.end(); ++it)
    {
      if (*it == light)
      {
        m_lights.erase(it);
        return;
      }
    }

    for (vector<shared_ptr<Entity>>::iterator it = m_lightEntities.begin(); it != m_lightEntities.end(); ++it)
    {
      if (*it == entity)
      {
        m_lightEntities.erase(it);
        return;
      }
    }
  }

  size_t RenderTechnique::getNumLights()
  {
    return m_lights.size();
  }

  shared_ptr<Light> RenderTechnique::getLight(uint32_t index)
  {
    return m_lights[index];
  }

  void RenderTechnique::addView(shared_ptr<View> view)
  {
    m_views.push_back(view);
    m_onscreenView = view;
  }

  void RenderTechnique::removeView(shared_ptr<View> view)
  {
    for (vector<shared_ptr<View>>::iterator it = m_views.begin(); it != m_views.end(); ++it)
    {
      if (*it == view)
      {
        m_views.erase(it);
        return;
      }
    }
  }

  void RenderTechnique::updateWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
  {
    vec2 position(x, y);
    vec2 size(width, height);
    m_onscreenView->setViewportPosition(position);
    m_onscreenView->setViewportSize(size);
    m_graphics->resize(width, height);
  }

  void RenderTechnique::buildLightClusters(shared_ptr<View> view)
  {
    m_clusterEntity = make_shared<Entity>("Frustum Lines");
    shared_ptr<Renderable> renderable = make_shared<Renderable>("Frustum Lines");
    vec2 viewportSize;
    view->getViewportSize(viewportSize);

    // Determine the number of X and Y tiles
    uint32_t segmentWidth = 64;
    uint32_t segmentHeight = 64;
    uint32_t numXSegments = (uint32_t)viewportSize.x / segmentWidth;
    uint32_t numYSegments = (uint32_t)viewportSize.y / segmentHeight;
    uint32_t numZSegments = 10;

    if ((uint32_t)viewportSize.x % segmentWidth != 0)
    {
      numXSegments++;
    }
    if ((uint32_t)viewportSize.y % segmentHeight != 0)
    {
      numYSegments++;
    }
    uint32_t numVerts = (numXSegments + 1) * (numYSegments + 1) * numZSegments;
    uint32_t numClusters = numXSegments * numYSegments * (numZSegments - 1);

    shared_ptr<Mesh> mesh = make_shared<Mesh>("Frustum Lines", Mesh::LINES, numVerts, 2);

    float* vertexBuffer = (float*)malloc(numVerts * 3 * sizeof(float));
    float* normalBuffer = (float*)malloc(numVerts * 3 * sizeof(float));

    uint32_t* indexBuffer = (uint32_t*)malloc(numClusters * 12 * 2 * sizeof(uint32_t));

    m_clusterData = (ClusterData*)malloc(sizeof(ClusterData));
    m_clusterData->m_clusters = (Cluster*)malloc(numClusters*sizeof(Cluster));
    m_clusterData->m_numClusterVerts = numVerts;
    m_clusterData->m_localVerts = vertexBuffer;
	  m_clusterData->m_numXSegments = numXSegments;
	  m_clusterData->m_numYSegments = numYSegments;
	  m_clusterData->m_numZSegments = numZSegments;
    m_clusterData->m_clusterVerts = (DirectX::XMVECTOR*)malloc(numVerts*sizeof(DirectX::XMVECTOR));

    // Get the eye and direction in world space
    mat4 viewTransform;
    view->getViewTransform(viewTransform);
    mat4 invViewTransform = glm::inverse(viewTransform);

    vec3 worldDirectionX = vec3(1.0f, 0.0f, 0.0f);
    vec3 worldDirectionY = vec3(0.0f, 1.0f, 0.0f);
    vec3 worldDirectionZ = vec3(0.0f, 0.0f, -1.0f);
    vec3 worldEye = vec3();

    float hFov = view->getFieldOfView();
    float nearClip = view->getNearClip();
    float farClip = 100.0f;// view->getFarClip();

    vec3 nearZ = worldEye + worldDirectionZ * nearClip;
    vec3 farZ = worldEye + worldDirectionZ * farClip;

    float zInc = (farClip - nearClip) / numZSegments;
    
    uint32_t vindex = 0;
    uint32_t nindex = 0;

    vec3 currentZ = nearZ;
    float currentZD = nearClip;

    for (uint32_t k = 0; k < numZSegments; k++)
    {
      float farD = tan(hFov*0.5f * (float)M_PI / 180.0f) * currentZD;
      float currentYD = farD;
      float xInc = farD * 2.0f / numXSegments;
      float yInc = -farD * 2.0f / numYSegments;

      for (uint32_t j = 0; j < numYSegments + 1; j++)
      {
        float currentXD = -farD;
        for (uint32_t i = 0; i < numXSegments + 1; i++)
        {
          vec3 currentPoint = currentZ + worldDirectionX * currentXD + worldDirectionY * currentYD;
          vertexBuffer[vindex++] = currentPoint.x;
          vertexBuffer[vindex++] = currentPoint.y;
          vertexBuffer[vindex++] = currentPoint.z;
          normalBuffer[nindex++] = 0.0f;
          normalBuffer[nindex++] = 0.0f;
          normalBuffer[nindex++] = 1.0f;
          currentXD += xInc;
        }
        currentYD += yInc;
      }
      currentZD += zInc;
      currentZ = worldEye + worldDirectionZ * currentZD;
    }

    uint32_t iindex = 0;
    uint32_t bvindex = 0;
    uint32_t clusterIndex = 0;
    for (uint32_t k = 0; k < numZSegments-1; k++)
    {
      vindex = k * (numYSegments + 1) * (numXSegments + 1);
      bvindex = (k+1) * (numYSegments + 1) * (numXSegments + 1);
      for (uint32_t j = 0; j < numYSegments; j++)
      {    
        for (uint32_t i = 0; i < numXSegments; i++)
        {
          indexBuffer[iindex++] = vindex;
          indexBuffer[iindex++] = vindex+1;
          indexBuffer[iindex++] = vindex + 1;
          indexBuffer[iindex++] = vindex + 1 + numXSegments + 1;
          indexBuffer[iindex++] = vindex + 1 + numXSegments + 1;
          indexBuffer[iindex++] = vindex + numXSegments + 1;
          indexBuffer[iindex++] = vindex + numXSegments + 1;
          indexBuffer[iindex++] = vindex;

          indexBuffer[iindex++] = vindex;
          indexBuffer[iindex++] = bvindex;
          indexBuffer[iindex++] = vindex + 1;
          indexBuffer[iindex++] = bvindex + 1;
          indexBuffer[iindex++] = vindex + 1 + numXSegments + 1;
          indexBuffer[iindex++] = bvindex + 1 + numXSegments + 1;
          indexBuffer[iindex++] = vindex + numXSegments + 1;
          indexBuffer[iindex++] = bvindex + numXSegments + 1;

          indexBuffer[iindex++] = bvindex;
          indexBuffer[iindex++] = bvindex + 1;
          indexBuffer[iindex++] = bvindex + 1;
          indexBuffer[iindex++] = bvindex + 1 + numXSegments + 1;
          indexBuffer[iindex++] = bvindex + 1 + numXSegments + 1;
          indexBuffer[iindex++] = bvindex + numXSegments + 1;
          indexBuffer[iindex++] = bvindex + numXSegments + 1;
          indexBuffer[iindex++] = bvindex;

          m_clusterData->m_clusters[clusterIndex].m_verts[0] = vindex;
          m_clusterData->m_clusters[clusterIndex].m_verts[1] = vindex + 1;
          m_clusterData->m_clusters[clusterIndex].m_verts[2] = vindex + 1 + numXSegments + 1;
          m_clusterData->m_clusters[clusterIndex].m_verts[3] = vindex + numXSegments + 1;
          m_clusterData->m_clusters[clusterIndex].m_verts[4] = bvindex;
          m_clusterData->m_clusters[clusterIndex].m_verts[5] = bvindex + 1;
          m_clusterData->m_clusters[clusterIndex].m_verts[6] = bvindex + 1 + numXSegments + 1;
          m_clusterData->m_clusters[clusterIndex].m_verts[7] = bvindex + numXSegments + 1;

          DirectX::XMVECTOR p[8];
          for (int i = 0; i < 8; i++)
          {
            uint32_t vertexIndex = m_clusterData->m_clusters[clusterIndex].m_verts[i] * 3;
            p[i].m128_f32[0] = m_clusterData->m_localVerts[vertexIndex];
            p[i].m128_f32[1] = m_clusterData->m_localVerts[vertexIndex +1];
            p[i].m128_f32[2] = m_clusterData->m_localVerts[vertexIndex +2];
          }

          m_clusterData->m_clusters[clusterIndex].m_planes[0] = planeEquation(p[0], p[3], p[2]);
          m_clusterData->m_clusters[clusterIndex].m_planes[1] = planeEquation(p[1], p[2], p[6]);
          m_clusterData->m_clusters[clusterIndex].m_planes[2] = planeEquation(p[5], p[6], p[7]);
          m_clusterData->m_clusters[clusterIndex].m_planes[3] = planeEquation(p[4], p[7], p[3]);
          m_clusterData->m_clusters[clusterIndex].m_planes[4] = planeEquation(p[1], p[5], p[4]);
          m_clusterData->m_clusters[clusterIndex].m_planes[5] = planeEquation(p[3], p[7], p[6]);

          m_clusterData->m_clusters[clusterIndex].m_lights = new vector<shared_ptr<Light>>;
          vindex++;
          bvindex++;
          clusterIndex++;
        }
        vindex++;
        bvindex++;
      }
    }

    mesh->addVertexBuffer(0, 3, numVerts * 3 * sizeof(float), vertexBuffer);
    mesh->addVertexBuffer(1, 3, numVerts * 3 * sizeof(float), normalBuffer);
    mesh->addIndexBuffer(numClusters * 12 * 2, indexBuffer);

    shared_ptr<Material>material = make_shared<Material>("Frustum Lines", Material::LIT);
    material->setAlbedoColor(vec4(1.0f, 1.0f, 1.0f, 1.0f));
    mesh->setMaterial(material);

    free(normalBuffer);
    free(indexBuffer);
    renderable->addGeometry(mesh);
    m_clusterEntity->addComponent(renderable);
    m_clusterEntity->setTransform(invViewTransform);
    m_clusterEntity->updateCompositeTransform(mat4());
    //addRenderable(renderable, m_clusterEntity);
  }

  void RenderTechnique::setClusterEntityFreeze(bool freeze)
  {
    m_freezeClusterEntity = freeze;
  }

  void RenderTechnique::build()
  {
    for (size_t i = 0; i < m_views.size(); ++i)
    {
      m_graphics->createView(m_views[i]);
    }

    buildLightClusters(m_onscreenView);
    m_graphics->buildBuffers(m_renderables);
  }


  void RenderTechnique::update()
  {
    updateLightClusters(m_onscreenView);
    m_graphics->updateFrameData(m_onscreenView, m_lights);
    m_graphics->updateMaterialData(m_onscreenView);
    m_graphics->updateObjectData(m_onscreenView, m_renderables);
  }

  void RenderTechnique::render()
  {
    m_graphics->beginFrame(m_onscreenView);

    update();

    m_graphics->beginGBufferPass(m_onscreenView);
    renderMeshes(m_onscreenView);
    m_graphics->endGBufferPass(m_onscreenView);

    m_graphics->beginLightingPass(m_onscreenView);
    m_graphics->endLightingPass(m_onscreenView);

    renderVolumes(m_onscreenView);

    m_graphics->endFrame(m_onscreenView);

    m_graphics->executeCommands(m_onscreenView);

    // Swap the buffers
    m_graphics->present(m_onscreenView);
  }

  void RenderTechnique::renderMeshes(shared_ptr<View> view)
  {
    uint32_t currentMeshIndex = 0;
    for (size_t i = 0; i < m_renderables.size(); i++)
    {

      for (size_t j = 0; j < m_renderables[i]->numGeometries(); j++, currentMeshIndex++)
      {
        shared_ptr<Geometry> geometry = m_renderables[i]->getGeometry(j);
        if (geometry->getGeometryType() == Geometry::GEOMETRY_MESH)
        {
          shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(geometry);
          m_graphics->drawMesh(view, mesh, mesh->getMaterial());
        }
      }
    }
  }

  void RenderTechnique::renderVolumes(shared_ptr<View> view)
  {
    uint32_t currentMeshIndex = 0;
    for (size_t i = 0; i < m_renderables.size(); i++)
    {

      for (size_t j = 0; j < m_renderables[i]->numGeometries(); j++, currentMeshIndex++)
      {
        shared_ptr<Geometry> geometry = m_renderables[i]->getGeometry(j);
        if (geometry->getGeometryType() == Geometry::GEOMETRY_VOLUME)
        {
          shared_ptr<Volume> volume = std::static_pointer_cast<Volume>(geometry);
          m_graphics->drawVolume(view, volume, volume->getMaterial());
        }
      }
    }
  }

  void RenderTechnique::updateLightClusters(shared_ptr<View> view)
  {
    mat4 viewTransform;
    mat4 invViewTransform;

    view->getViewTransform(viewTransform);
    invViewTransform = glm::inverse(viewTransform);

    vec3 eye = view->getEye();
    vec3 up = view->getUp();
    vec3 target = view->getTarget();

    DirectX::XMVECTOR pos = DirectX::XMVectorSet(eye.x, eye.y, eye.z, 1.0f);
    DirectX::XMVECTOR targetp = DirectX::XMVectorSet(target.x, target.y, target.z, 1.0f);
    DirectX::XMVECTOR upv = DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f);

    DirectX::XMMATRIX viewm = DirectX::XMMatrixLookAtLH(pos, targetp, upv);

    uint32_t vindex = 0;
    for (uint32_t i = 0; i < m_clusterData->m_numClusterVerts; i++)
    {
      DirectX::XMVECTOR localPoint = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
      localPoint.m128_f32[0] = m_clusterData->m_localVerts[vindex++];
      localPoint.m128_f32[1] = m_clusterData->m_localVerts[vindex++];
      localPoint.m128_f32[2] = m_clusterData->m_localVerts[vindex++];
      m_clusterData->m_clusterVerts[i] = DirectX::XMVECTOR(DirectX::XMVector3Transform(localPoint, viewm));
    }

    for (size_t i = 0; i < m_lights.size(); ++i)
    {
      vec3 position;
      mat4 transform;
      m_lights[i]->getPosition(position);
      m_lights[i]->getEntity(0)->getCompositeTransform(transform);
      vec3 lightViewPosition = vec3(transform * vec4(position, 1.0f));
      m_lights[i]->setViewPosition(lightViewPosition);
    }

    size_t maxLights = 0;
    size_t minLights = 100000;
    size_t numZero = 0;
    size_t totalLights = 0;

    uint32_t clusterIndex = 0;
	  for (uint32_t k = 0; k < m_clusterData->m_numZSegments-1; k++)
	  {
		  for (uint32_t j = 0; j < m_clusterData->m_numYSegments; j++)
		  {
			  for (uint32_t i = 0; i < m_clusterData->m_numXSegments; i++)
			  {
          DirectX::XMVECTOR p1 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[0]];
          DirectX::XMVECTOR p2 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[1]];
          DirectX::XMVECTOR p3 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[2]];
          DirectX::XMVECTOR p4 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[3]];
          DirectX::XMVECTOR p5 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[4]];
          DirectX::XMVECTOR p6 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[5]];
          DirectX::XMVECTOR p7 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[6]];
          DirectX::XMVECTOR p8 = m_clusterData->m_clusterVerts[m_clusterData->m_clusters[clusterIndex].m_verts[7]];

          //m_clusterData->m_clusters[clusterIndex].m_planes[0].w = updatePlaneD(m_clusterData->m_clusters[clusterIndex].m_planes[0], p1);
          //m_clusterData->m_clusters[clusterIndex].m_planes[1].w = updatePlaneD(m_clusterData->m_clusters[clusterIndex].m_planes[1], p2);
          //m_clusterData->m_clusters[clusterIndex].m_planes[2].w = updatePlaneD(m_clusterData->m_clusters[clusterIndex].m_planes[2], p6);
          //m_clusterData->m_clusters[clusterIndex].m_planes[3].w = updatePlaneD(m_clusterData->m_clusters[clusterIndex].m_planes[3], p5);
          //m_clusterData->m_clusters[clusterIndex].m_planes[4].w = updatePlaneD(m_clusterData->m_clusters[clusterIndex].m_planes[4], p2);
          //m_clusterData->m_clusters[clusterIndex].m_planes[5].w = updatePlaneD(m_clusterData->m_clusters[clusterIndex].m_planes[5], p4);

          m_clusterData->m_clusters[clusterIndex].m_planes[0] = planeEquation(p1, p4, p3);
          m_clusterData->m_clusters[clusterIndex].m_planes[1] = planeEquation(p2, p3, p7);
          m_clusterData->m_clusters[clusterIndex].m_planes[2] = planeEquation(p6, p7, p8);
          m_clusterData->m_clusters[clusterIndex].m_planes[3] = planeEquation(p5, p8, p4);
          m_clusterData->m_clusters[clusterIndex].m_planes[4] = planeEquation(p2, p6, p5);
          m_clusterData->m_clusters[clusterIndex].m_planes[5] = planeEquation(p4, p8, p7);

          m_clusterData->m_clusters[clusterIndex].m_lights->clear();
          for (uint32_t l = 0; l < m_lights.size(); l++)
          {
            vec3 lightViewPosition;
            m_lights[l]->getPosition(lightViewPosition);
            if (intersectsCluster(clusterIndex, lightViewPosition, 25.0f))
            {
              m_clusterData->m_clusters[clusterIndex].m_lights->push_back(m_lights[l]);
              totalLights++;
            }
          }

          size_t numLights = m_clusterData->m_clusters[clusterIndex].m_lights->size();
          if (numLights == 0)
          {
            numZero++;
          }
          if (numLights > maxLights)
          {
            maxLights = numLights;
          }
          if (numLights < minLights)
          {
            minLights = numLights;
          }
          clusterIndex++;
			  }
		  }
	  }

    //m_worldManager->printLog("MaxLights: " + std::to_string(maxLights) + ", totalLights: " + std::to_string(totalLights) + ", zeros: " + std::to_string(numZero));
    if (!m_freezeClusterEntity)
    {
      m_clusterEntity->setTransform(invViewTransform);
      m_clusterEntity->updateCompositeTransform(mat4());
    }
  }

  vec4 RenderTechnique::planeEquation(DirectX::XMVECTOR p1, DirectX::XMVECTOR p2, DirectX::XMVECTOR p3)
  {
    vec4 plane;
    DirectX::FXMVECTOR p3p2 = DirectX::XMVectorSubtract(p3, p2);
    DirectX::FXMVECTOR v1 = DirectX::XMVector3Normalize(p3p2);
    DirectX::FXMVECTOR p1p2 = DirectX::XMVectorSubtract(p1, p2);
    DirectX::FXMVECTOR v2 = DirectX::XMVector3Normalize(p1p2);

    DirectX::FXMVECTOR normal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(v2, v1));
    
    plane.x = -normal.m128_f32[0];
    plane.y = -normal.m128_f32[1];
    plane.z = -normal.m128_f32[2];
    plane.w = -(normal.m128_f32[0] * p1.m128_f32[0] + normal.m128_f32[1] * p1.m128_f32[1] + normal.m128_f32[2] * p1.m128_f32[2]);
    return plane;
  }

  float RenderTechnique::updatePlaneD(vec4 plane, vec3 p)
  {
    return -(plane.x * p.x + plane.y * p.y + plane.z * p.z);
  }

  bool RenderTechnique::intersectsCluster(uint32_t clusterIndex, vec3 lightViewPosition, float radius)
  {
    bool intersects = true;
    for (int i = 0; i < 6; i++)
    {
      vec4 plane = m_clusterData->m_clusters[clusterIndex].m_planes[i];
      float denom = 1.0f / sqrt(plane.x * plane.x + plane.y*plane.y + plane.z*plane.z);
      float d = (plane.x * lightViewPosition.x + plane.y * lightViewPosition.y + plane.z * lightViewPosition.z + plane.w) * denom;
      if ((d + radius) < 0.0f)
      {
        return false;
      }
    }

    return intersects;
  }
}