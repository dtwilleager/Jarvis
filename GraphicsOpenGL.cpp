#include "stdafx.h"
#include "GraphicsOpenGL.h"
#include "Light.h"

namespace Jarvis
{
  GraphicsOpenGL::GraphicsOpenGL(string name, HINSTANCE hinstance, HWND window) : Graphics(name, hinstance, window),
    m_frameIndex(0),
    m_hinstance(hinstance),
    m_window(window)
  {
  }


  GraphicsOpenGL::~GraphicsOpenGL()
  {
  }

  void GraphicsOpenGL::createDevice(uint32_t numFrames)
  {
    // Set the appropriate pixel format.
    PIXELFORMATDESCRIPTOR pfd =
    {
      sizeof(PIXELFORMATDESCRIPTOR),            // Size Of This Pixel Format Descriptor
      1,                                        // Version Number
      PFD_DRAW_TO_WINDOW |                      // Format Must Support Window
      PFD_SUPPORT_OPENGL |                      // Format Must Support OpenGL
      PFD_STEREO |                              // Format Must Support Quad-buffer Stereo
      PFD_DOUBLEBUFFER,                         // Must Support Double Buffering
      PFD_TYPE_RGBA,                            // Request An RGBA Format
      24,                                       // 24-bit color depth
      0, 0, 0, 0, 0, 0,                         // Color Bits Ignored
      0,                                        // No Alpha Buffer
      0,                                        // Shift Bit Ignored
      0,                                        // No Accumulation Buffer
      0, 0, 0, 0,                               // Accumulation Bits Ignored
      32,                                       // 32-bit Z-Buffer (Depth Buffer)
      0,                                        // No Stencil Buffer
      0,                                        // No Auxiliary Buffer
      PFD_MAIN_PLANE,                           // Main Drawing Layer
      0,                                        // Reserved
      0, 0, 0                                   // Layer Masks Ignored
    };

    m_deviceContext = GetDC(m_window);
    GLuint pixelFormat = ChoosePixelFormat(m_deviceContext, &pfd);
    SetPixelFormat(m_deviceContext, pixelFormat, &pfd);
    m_renderContext = wglCreateContext(m_deviceContext);
    wglMakeCurrent(m_deviceContext, m_renderContext);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
  }

  void GraphicsOpenGL::beginGBufferPass(shared_ptr<View> view)
  {
    glViewData* viewData = (glViewData*)view->getGraphicsData();

    glViewport(0, 0, viewData->m_currentWidth, viewData->m_currentHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void GraphicsOpenGL::drawMesh(shared_ptr<View> view, shared_ptr<Mesh> mesh, shared_ptr<Material> material)
  {
    update(view, material);

    glMaterialData* materialData = (glMaterialData*)mesh->getMaterial()->getGraphicsData();
    glMeshData* meshData = (glMeshData*)mesh->getGraphicsData();

    mat4 viewTransform;
    mat4 compositeModelTransform;
    GLint i = 0;

    view->getViewTransform(viewTransform);
    mesh->getRenderable()->getEntity(0)->getCompositeTransform(compositeModelTransform);

    glUniformMatrix4fv(materialData->m_modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewTransform*compositeModelTransform));

    glBindVertexArray(meshData->m_vertexArrayID);
    for (i = 0; i<mesh->getNumBuffers(); i++)
    {
      glEnableVertexAttribArray(i);
      glBindBuffer(GL_ARRAY_BUFFER, meshData->m_vertexArrayBufferIDs[i]);
      glVertexAttribPointer(i, (GLint)mesh->getVertexBufferSize(i), GL_FLOAT, GL_FALSE, 0, 0);
    }

    for (; i<8; i++)
    {
      glDisableVertexAttribArray(i);
    }

    if (mesh->getIndexBufferSize() != 0)
    {
      glDrawElements(GL_TRIANGLES, (GLsizei)mesh->getIndexBufferSize(), GL_UNSIGNED_INT, mesh->getIndexBuffer());
    }
    else
    {
      glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh->getNumVerts());
    }
  }


  void GraphicsOpenGL::drawVolume(shared_ptr<View> view, shared_ptr<Volume>, shared_ptr<Material> material)
  {
  }

  void GraphicsOpenGL::compute(shared_ptr<View> view)
  {
  }

  void GraphicsOpenGL::trace(shared_ptr<View> view)
  {
  }

  void GraphicsOpenGL::endGBufferPass(shared_ptr<View> view)
  {
  }

  void GraphicsOpenGL::beginLightingPass(shared_ptr<View> view)
  {
  }

  void GraphicsOpenGL::endLightingPass(shared_ptr<View> view)
  {
  }

  void GraphicsOpenGL::beginFrame(shared_ptr<View> view)
  {
  }
  void GraphicsOpenGL::endFrame(shared_ptr<View> view)
  {
  }

  void GraphicsOpenGL::executeCommands(shared_ptr<View> view)
  {
  }

  void GraphicsOpenGL::update(shared_ptr<View> view, shared_ptr<Material> material)
  {
    glMaterialData* materialData = (glMaterialData*)material->getGraphicsData();
    glUseProgram(materialData->m_shaderProgram);

    mat4 viewTransform;
    mat4 projectionTransform;
    mat4 worldToCamera;
    mat4 compositeModelTransform;
    vec3 boundsCenter;

    view->getProjectionTransform(projectionTransform);

    glUniformMatrix4fv(materialData->m_projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionTransform));

    view->getViewTransform(viewTransform);
    if (material->getBlendEnable())
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
      glDisable(GL_BLEND);
    }

    glDisable(GL_BLEND);

    vec4 diffuseColor;
    material->getAlbedoColor(diffuseColor);
    glUniform3fv(materialData->m_diffuseColorUniform, 1, glm::value_ptr(diffuseColor));

    if (material->getMaterialType() != Material::LIT_NOTEXTURE)
    {
      shared_ptr<Texture> texture = material->getAlbedoTexture();
      glTextureData* textureData = (glTextureData*)texture->getGraphicsData();
      glBindTexture(GL_TEXTURE_2D, textureData->m_diffuseTextureID);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  void GraphicsOpenGL::present(shared_ptr<View> view)
  {
    SwapBuffers(m_deviceContext);
  }

  void GraphicsOpenGL::buildBuffers(vector<shared_ptr<Renderable>>& renderables)
  {
    for (size_t i = 0; i < renderables.size(); ++i)
    {
      for (size_t j = 0; j < renderables[i]->numGeometries(); ++j)
      {
        shared_ptr<Geometry> geometry = renderables[i]->getGeometry(j);
        if (geometry->getGeometryType() == Geometry::GEOMETRY_MESH)
        {
          shared_ptr<Mesh> mesh = std::static_pointer_cast<Mesh>(geometry);
          createMesh(mesh);
          createMaterial(mesh->getMaterial());
        }
      }
    }
  }

  void GraphicsOpenGL::createMesh(shared_ptr<Mesh> mesh)
  {
    glMeshData* meshData = (glMeshData*)mesh->getGraphicsData();
    if (meshData == nullptr || mesh->isDirty())
    {
      if (meshData != nullptr)
      {
        // TODO: Free old resources
      }

      size_t numBuffers = mesh->getNumBuffers();

      meshData = new glMeshData();
      mesh->setGraphicsData(meshData);
      meshData->m_vertexArrayBufferIDs = new GLuint[numBuffers];

      if (meshData->m_vertexArrayID == 0)
      {
        glGenVertexArrays(1, &meshData->m_vertexArrayID);
      }

      for (unsigned int i = 0; i < numBuffers; i++)
      {
        glBindVertexArray(meshData->m_vertexArrayID);
        glEnableVertexAttribArray(i);
        glGenBuffers(1, &meshData->m_vertexArrayBufferIDs[i]);
        glBindBuffer(GL_ARRAY_BUFFER, meshData->m_vertexArrayBufferIDs[i]);
        glBufferData(GL_ARRAY_BUFFER, mesh->getVertexBufferNumBytes(i), mesh->getVertexBufferData(i), GL_STATIC_DRAW);
        glVertexAttribPointer(i, (GLint)mesh->getVertexBufferSize(i), GL_FLOAT, GL_FALSE, 0, 0);
      }
    }
  }

  void GraphicsOpenGL::createMaterial(shared_ptr<Material> material)
  {
    glMaterialData* materialData = (glMaterialData*)material->getGraphicsData();
    if (materialData == nullptr || material->getDirty())
    {
      if (materialData != nullptr)
      {
        // TODO: Free old resources
      }

      materialData = new glMaterialData();

      switch (material->getMaterialType())
      {
      case Material::LIT:
        materialData->m_vertexShaderCode = readFile("shaders/vShaderLit.glsl");
        materialData->m_fragmentShaderCode = readFile("shaders/fShaderLit.glsl");
        break;
      case Material::LIT_NOTEXTURE:
        materialData->m_vertexShaderCode = readFile("shaders/vShaderLitNoTex.glsl");
        materialData->m_fragmentShaderCode = readFile("shaders/fShaderLitNoTex.glsl");
        break;
      }

      materialData->m_vertexShaderCodeSize = (GLint)materialData->m_vertexShaderCode.size();
      materialData->m_fragmentShaderCodeSize = (GLint)materialData->m_fragmentShaderCode.size();

      materialData->m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
      const char* vsource = materialData->m_vertexShaderCode.data();
      glShaderSource(materialData->m_vertexShader, 1, &vsource, &materialData->m_vertexShaderCodeSize);
      glCompileShader(materialData->m_vertexShader);
      GLenum err = glGetError();

      materialData->m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
      const char* fsource = materialData->m_fragmentShaderCode.data();
      glShaderSource(materialData->m_fragmentShader, 1, &fsource, &materialData->m_fragmentShaderCodeSize);
      glCompileShader(materialData->m_fragmentShader);
      err = glGetError();

      materialData->m_shaderProgram = glCreateProgram();
      glAttachShader(materialData->m_shaderProgram, materialData->m_vertexShader);
      glAttachShader(materialData->m_shaderProgram, materialData->m_fragmentShader);
      glLinkProgram(materialData->m_shaderProgram);
      err = glGetError();

      glUseProgram(materialData->m_shaderProgram);
      err = glGetError();

      if (material->getAlbedoTexture() != nullptr)
      {
        loadTexture(material->getAlbedoTexture());
      }

      materialData->m_projectionUniform = glGetUniformLocation(materialData->m_shaderProgram, "projectionMatrix");
      materialData->m_modelViewUniform = glGetUniformLocation(materialData->m_shaderProgram, "modelViewMatrix");
      materialData->m_diffuseColorUniform = glGetUniformLocation(materialData->m_shaderProgram, "diffuseColor");
      err = glGetError();

      material->setGraphicsData(materialData);
      material->setDirty(false);
    }
  }

  void GraphicsOpenGL::loadTexture(shared_ptr<Texture> texture)
  {
    glTextureData* textureData = new glTextureData();
    texture->setGraphicsData(textureData);

    size_t width = texture->getWidth();
    size_t height = texture->getHeight();

    glGenTextures(1, &textureData->m_diffuseTextureID);
    glBindTexture(GL_TEXTURE_2D, textureData->m_diffuseTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)texture->getBitsPerPixel(), (GLint)width, (GLint)height,
      0, texture->getFormat(), GL_UNSIGNED_BYTE, texture->getData());
  }

  void GraphicsOpenGL::createView(shared_ptr<View> view)
  {
    glViewData* viewData = new glViewData();
    view->setGraphicsData(viewData);

    vec2 size;
    view->getViewportSize(size);
    viewData->m_currentWidth = (uint32_t)size.x;
    viewData->m_currentHeight = (uint32_t)size.y;

      // Create the surface
      resize(viewData->m_currentWidth, viewData->m_currentHeight);
      viewData->m_frameIndex = m_frameIndex++;
  }

  void GraphicsOpenGL::resize(uint32_t width, uint32_t height)
  {
    m_width = width;
    m_height = height;
  }

  vector<char> GraphicsOpenGL::readFile(const string& filename) {
    ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  }

  void GraphicsOpenGL::updateFrameData(shared_ptr<View> view, vector<shared_ptr<Light>>& lights)
  {

  }

  void GraphicsOpenGL::updateMaterialData(shared_ptr<View> view)
  {

  }

  void GraphicsOpenGL::updateObjectData(shared_ptr<View> view, vector<shared_ptr<Renderable>>& renderables)
  {

  }
}
