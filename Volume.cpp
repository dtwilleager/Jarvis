#include "stdafx.h"
#include "Volume.h"

#include <cstring>

using std::memcpy;

namespace Jarvis
{
  Volume::Volume(string name): Geometry(name, GeometryType::GEOMETRY_VOLUME),
    m_imageData(nullptr)
  {
  }


  Volume::~Volume()
  {
  }

  void Volume::setDimensions(int* dimensions)
  {
    m_dimensions[0] = dimensions[0];
    m_dimensions[1] = dimensions[1];
    m_dimensions[2] = dimensions[2];
  }

  void Volume::getDimensions(uint32_t* dimensions)
  {
    dimensions[0] = m_dimensions[0];
    dimensions[1] = m_dimensions[1];
    dimensions[2] = m_dimensions[2];
  }

  void Volume::setBounds(double* bounds)
  {
    m_bounds[0] = bounds[0];
    m_bounds[1] = bounds[1];
    m_bounds[2] = bounds[2];
    m_bounds[3] = bounds[3];
    m_bounds[4] = bounds[4];
    m_bounds[5] = bounds[5];
  }

  void Volume::getBounds(float* bounds)
  {
    bounds[0] = (float)m_bounds[0];
    bounds[1] = (float)m_bounds[1];
    bounds[2] = (float)m_bounds[2];
    bounds[3] = (float)m_bounds[3];
    bounds[4] = (float)m_bounds[4];
    bounds[5] = (float)m_bounds[5];
  }

  void Volume::setSpacing(double* spacing)
  {
    m_spacing[0] = spacing[0];
    m_spacing[1] = spacing[1];
    m_spacing[2] = spacing[2];
  }

  void Volume::getSpacing(double* spacing)
  {
    spacing[0] = m_spacing[0];
    spacing[1] = m_spacing[1];
    spacing[2] = m_spacing[2];
  }

  void Volume::setImageData(unsigned short* data)
  {
    if (m_imageData != nullptr)
    {
      free(m_imageData);
    }

    size_t size = m_dimensions[0] * m_dimensions[1] * m_dimensions[2] * sizeof(unsigned short);
    m_imageData = (unsigned short*)malloc(size);
    memcpy(m_imageData, data, size);
  }

  unsigned short* Volume::getImageData()
  {
    return m_imageData;
  }
}