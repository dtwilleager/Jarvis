#include "stdafx.h"
#include "Renderable.h"

namespace Jarvis
{
  Renderable::Renderable(string name) : Component(name, Component::RENDER)
  {
  }


  Renderable::~Renderable()
  {
  }

  void Renderable::addGeometry(shared_ptr<Geometry> geometry)
  {
    m_geometries.push_back(geometry);
    geometry->setRenderable(shared_from_this());
  }

  shared_ptr<Geometry> Renderable::getGeometry(size_t index)
  {
    return m_geometries[index];
  }

  size_t Renderable::numGeometries()
  {
    return m_geometries.size();
  }

  void Renderable::setVisible(bool visible)
  {
    m_visible = visible;
  }

  bool Renderable::IsVisible()
  {
    return m_visible;
  }
}
