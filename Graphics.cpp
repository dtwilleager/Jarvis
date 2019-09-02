#include "stdafx.h"
#include "Graphics.h"

#include "Mesh.h"
#include "View.h"

namespace Jarvis
{
  Graphics::Graphics(string name, HINSTANCE hinstance, HWND window) :
    m_name(name),
    m_hinstance(hinstance),
    m_window(window)
  {
  }


  Graphics::~Graphics()
  {
  }

  void Graphics::createDevice(uint32_t numFrames)
  {
  }

  void Graphics::createView(shared_ptr<View> view)
  {
  }

  void Graphics::resize(uint32_t width, uint32_t height)
  {
  }

  void Graphics::buildBuffers(vector<shared_ptr<Renderable>>& renderables)
  {
  }

  void Graphics::beginGBufferPass(shared_ptr<View> view)
  {
  }

  void Graphics::drawMesh(shared_ptr<View> view, shared_ptr<Mesh>, shared_ptr<Material> material)
  {
  }

  void Graphics::compute(shared_ptr<View> view)
  {
  }

  void Graphics::trace(shared_ptr<View> view)
  {
  }

  void Graphics::endGBufferPass(shared_ptr<View> view)
  {
  }

  void Graphics::beginLightingPass(shared_ptr<View> view)
  {
  }

  void Graphics::endLightingPass(shared_ptr<View> view)
  {
  }

  void Graphics::beginFrame(shared_ptr<View> view)
  {
  }
  void Graphics::endFrame(shared_ptr<View> view)
  {
  }

  void Graphics::executeCommands(shared_ptr<View> view)
  {
  }

  void Graphics::present(shared_ptr<View> view)
  {
  }

  void Graphics::updateFrameData(shared_ptr<View> view, vector<shared_ptr<Light>>& lights)
  {

  }

  void Graphics::updateMaterialData(shared_ptr<View> view)
  {

  }

  void Graphics::updateObjectData(shared_ptr<View> view, vector<shared_ptr<Renderable>>& renderables)
  {

  }
}