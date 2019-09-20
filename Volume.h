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

  class Volume: public Geometry
  {
  public:
    Volume(string name);
    ~Volume();

    void setDimensions(int* dimensions);
    void getDimensions(uint32_t* dimensions);
    void setBounds(double* bounds);
    void getBounds(float* bounds);
    void setSpacing(double* spacing);
    void getSpacing(double* spacing);
    void setImageData(unsigned short* data);
    unsigned short* getImageData();

  private:

    int             m_dimensions[3];
    double          m_bounds[6];
    double          m_spacing[3];
    unsigned short* m_imageData;
  };
}

