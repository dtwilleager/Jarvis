#pragma once
#include "Material.h"
#include "Renderable.h"

#include <string>
#include <memory>

using std::string;
using std::shared_ptr;

namespace Jarvis
{
  class Renderable;

  class Geometry
  {
  public:

    enum GeometryType {
      GEOMETRY_MESH,
      GEOMETRY_VOLUME
    };

    Geometry(string name, GeometryType geometryType);
    ~Geometry();

    void                  setMaterial(shared_ptr<Material> material);
    shared_ptr<Material>  getMaterial();
    void                  setRenderable(shared_ptr<Renderable> renderable);
    shared_ptr<Renderable> getRenderable();
    void                  setDirty(bool dirty);
    bool                  isDirty();
    void                  setGraphicsData(void * graphicsData);
    void*                 getGraphicsData();
    GeometryType          getGeometryType();

  protected:
 
    string                m_name;
    GeometryType          m_geometryType;
    shared_ptr<Material>  m_material;
    shared_ptr<Renderable> m_renderable;
    bool                  m_dirty;
    void*                 m_graphicsData;
  };
}

