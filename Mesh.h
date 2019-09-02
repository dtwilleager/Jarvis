#pragma once
#include "Geometry.h"
#include "Material.h"
#include "Renderable.h"

#include <string>
#include <memory>

using std::string;
using std::shared_ptr;

namespace Jarvis
{
  class Renderable;
  class Geometry;

  class Mesh: public Geometry
  {
  public:
    enum Primitive {
      TRIANGLES,
      LINES
    };

    Mesh(string name, Primitive primitive, size_t numVerts, size_t numVertexArrayBuffers);
    ~Mesh();

    Primitive             getPrimitive();
    void                  addVertexBuffer(unsigned int index, size_t size, size_t numBytes, float* data);
    void                  addIndexBuffer(size_t size, unsigned int* data);
    size_t                getVertexBufferSize(size_t index);
    size_t                getVertexBufferNumBytes(size_t index);
    float*				        getVertexBufferData(size_t index);
    size_t                getIndexBufferSize();
    size_t                getNumVerts();
    unsigned int*         getIndexBuffer();
    size_t                getNumBuffers();

  private:
    struct vertexData
    {
      size_t  size;
      size_t  numBytes;
      float*  data;
    };
 
    Primitive             m_primitive;
    size_t                m_numVerts;
    size_t                m_numVertexArrayBuffers;
    struct vertexData*    m_vertexData;
    unsigned int*         m_indexBuffer;
    size_t                m_indexBufferSize;
  };
}

