#include "stdafx.h"
#include "Mesh.h"

#include <cstring>

using std::memcpy;

namespace Jarvis
{
  Mesh::Mesh(string name, Primitive primitive, size_t numVerts, size_t numVertexArrayBuffers): Geometry(name, GeometryType::GEOMETRY_MESH),
    m_primitive(primitive),
    m_numVerts(numVerts),
    m_numVertexArrayBuffers(numVertexArrayBuffers)
  {
    m_vertexData = new struct vertexData[numVertexArrayBuffers];
    for (size_t i = 0; i < numVertexArrayBuffers; i++)
    {
      m_vertexData[i].data = nullptr;
    }
  }


  Mesh::~Mesh()
  {
    for (size_t i = 0; i < m_numVertexArrayBuffers; i++)
    {
      if (m_vertexData[i].data != nullptr)
      { 
        delete m_vertexData[i].data;
      }
    }
    delete m_vertexData;
  }

  Mesh::Primitive Mesh::getPrimitive()
  {
    return m_primitive;
  }

  void Mesh::addVertexBuffer(unsigned int index, size_t size, size_t numBytes, float* data)
  {
    unsigned int dindex = 0;

    m_vertexData[index].size = size;
    m_vertexData[index].numBytes = numBytes;
    m_vertexData[index].data = new float[m_numVerts*size * sizeof(float)];
    memcpy(m_vertexData[index].data, data, numBytes);
    m_dirty = true;
    //for (unsigned int i = 0; i<m_numVerts; i++)
    //{
    //  for (unsigned int j = 0; j<size; j++)
    //  {
    //    m_vertexData[index].data[dindex] = data[dindex++];
    //  }
    //}
  }

  void Mesh::addIndexBuffer(size_t size, unsigned int* data)
  {
    m_indexBufferSize = size;
    m_indexBuffer = new unsigned int[size];
    memcpy(m_indexBuffer, data, size*sizeof(unsigned int));
    m_dirty = true;
    //for (unsigned int i = 0; i<size; i++)
    //{
    //  m_indexBuffer[i] = data[i];
    //}
  }

  size_t Mesh::getVertexBufferSize(size_t index)
  {
    return m_vertexData[index].size;
  }

  size_t Mesh::getVertexBufferNumBytes(size_t index)
  {
    return m_vertexData[index].numBytes;
  }

  float* Mesh::getVertexBufferData(size_t index)
  {
    return m_vertexData[index].data;
  }

  size_t Mesh::getIndexBufferSize() 
  { 
    return m_indexBufferSize; 
  }

  size_t Mesh::getNumVerts() 
  { 
    return m_numVerts; 
  }

  unsigned int* Mesh::getIndexBuffer()
  {
    return m_indexBuffer;
  }

  size_t Mesh::getNumBuffers() 
  { 
    return m_numVertexArrayBuffers; 
  }
}