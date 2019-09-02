#include "stdafx.h"
#include "Geometry.h"

#include <cstring>

using std::memcpy;

namespace Jarvis
{
  Geometry::Geometry(string name, GeometryType geometryType):
    m_name(name),
    m_geometryType(geometryType),
    m_graphicsData(nullptr),
    m_dirty(true)
  {
  }

  Geometry::~Geometry()
  {
  }

  Geometry::GeometryType Geometry::getGeometryType()
  {
    return m_geometryType;
  }

  void Geometry::setRenderable(shared_ptr<Renderable> renderable)
  {
    m_renderable = renderable;
  }

  shared_ptr<Renderable> Geometry::getRenderable()
  {
    return m_renderable;
  }

  void Geometry::setMaterial(shared_ptr<Material> material)
  {
    m_material = material;
  }

  shared_ptr<Material> Geometry::getMaterial()
  {
    return m_material;
  }

  void Geometry::setGraphicsData(void * graphicsData)
  {
    m_graphicsData = graphicsData;
  }

  void* Geometry::getGraphicsData()
  {
    return m_graphicsData;
  }

  void Geometry::setDirty(bool dirty)
  {
    m_dirty  = dirty;
  }

  bool Geometry::isDirty()
  {
    return m_dirty;
  }
}