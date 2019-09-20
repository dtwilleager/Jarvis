#pragma once

#include "Entity.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include "View.h"
#include "Volume.h"

#include <string>
#include <vector>
#include <memory>
#include <map>

#include "vtkBoxWidget.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkColorTransferFunction.h"
#include "vtkDICOMImageReader.h"
#include "vtkImageData.h"
#include "vtkImageResample.h"
#include "vtkMetaImageReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlanes.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLImageDataReader.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkMatrix4x4.h"

using std::string;
using std::vector;
using std::shared_ptr;
using std::map;

namespace Jarvis
{
	class VtkLoader
	{
	public:
    VtkLoader();
		~VtkLoader();

    shared_ptr<Entity> loadVtkObjects(vtkRenderer* renderer);

  private:
    void addVolume(shared_ptr<Entity> parent, vtkVolume* volume);
	};
}

