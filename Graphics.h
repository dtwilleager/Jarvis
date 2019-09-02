#pragma once
#include "stdafx.h"

//#include "Mesh.h"
//#include "View.h"

#include <string>
#include <memory>
#include <vector>

using std::string;
using std::shared_ptr;
using std::vector;

namespace Jarvis
{
  class RenderTechnique;
  class Renderable;
  class Light;
  class Mesh;
  class View;
  class Material;

  class Graphics
  {
  public:
    Graphics(string name, HINSTANCE hinstance, HWND window);
    ~Graphics();

    virtual void                createDevice(uint32_t numFrames);

    virtual void                createView(shared_ptr<View> view);
    virtual void                resize(uint32_t width, uint32_t height);

    virtual void                buildBuffers(vector<shared_ptr<Renderable>>& renderables);

    virtual void                beginGBufferPass(shared_ptr<View> view);
    virtual void                drawMesh(shared_ptr<View> view, shared_ptr<Mesh>, shared_ptr<Material>);
    virtual void                compute(shared_ptr<View> view);
    virtual void                trace(shared_ptr<View> view);
    virtual void                endGBufferPass(shared_ptr<View> view);
    virtual void                beginLightingPass(shared_ptr<View> view);
    virtual void                endLightingPass(shared_ptr<View> view);

    virtual void                beginFrame(shared_ptr<View> view);
    virtual void                endFrame(shared_ptr<View> view);

    virtual void                executeCommands(shared_ptr<View> view);
    virtual void                present(shared_ptr<View> view);

    virtual void                updateFrameData(shared_ptr<View> view, vector<shared_ptr<Light>>& lights);
    virtual void                updateMaterialData(shared_ptr<View> view);
    virtual void                updateObjectData(shared_ptr<View> view, vector<shared_ptr<Renderable>>& renderables);


  protected:
    HINSTANCE                     m_hinstance;
    HWND                          m_window;
    uint32_t                      m_width;
    uint32_t                      m_height;

  private:
    string                        m_name;
  };
}
