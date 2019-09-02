#pragma once
#include "Component.h"
#include "Geometry.h"
#include "Graphics.h"
#include "View.h"
#include "Light.h"

using std::enable_shared_from_this;

namespace Jarvis 
{
  class Graphics;
  class Geometry;

  class Renderable :
    public Component, public enable_shared_from_this<Renderable>
  {
  public:
    Renderable(string name);
    ~Renderable();

    void                 addGeometry(shared_ptr<Geometry> geometry);
    shared_ptr<Geometry> getGeometry(size_t index);
    size_t               numGeometries();
    void                 setVisible(bool visible);
    bool                 IsVisible();

  private:
    vector<shared_ptr<Geometry>>  m_geometries;
    bool                          m_visible;
  };
}

