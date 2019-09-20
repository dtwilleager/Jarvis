#include "stdafx.h"
#include "VtkLoader.h"


using std::make_shared;
using std::static_pointer_cast;

namespace Jarvis
{
  VtkLoader::VtkLoader()
  {
  }


  VtkLoader::~VtkLoader()
  {
  }

  shared_ptr<Entity> VtkLoader::loadVtkObjects(vtkRenderer* renderer)
  {
    vtkVolumeCollection* vc = renderer->GetVolumes();

    if (vc == nullptr)
    {
      return nullptr;
    }

    shared_ptr<Entity> rootEntity = make_shared<Entity>("Vtk Volumes");

    // Start with Volumes for now
    vc->InitTraversal();
    vtkVolume* volume = vc->GetNextVolume();
    while (volume != nullptr)
    {
      addVolume(rootEntity, volume);
      volume = vc->GetNextVolume();
    }

    return (rootEntity);
  }

  void VtkLoader::addVolume(shared_ptr<Entity> parent, vtkVolume* volume)
  {
    shared_ptr<Entity> entity = make_shared<Entity>("Vtk Volume ");
    parent->addChild(entity);
    // Assume this mapper for now.
    vtkOpenGLGPUVolumeRayCastMapper* mapper = (vtkOpenGLGPUVolumeRayCastMapper *)volume->GetMapper();
    vtkImageData * data = mapper->GetInput();
    data->ComputeBounds();
    int dim[3];
    data->GetDimensions(dim);

    uint32_t width = dim[0];
    uint32_t height = dim[1];
    uint32_t depth = dim[2];

    vtkMatrix4x4* vtkMatrix = volume->GetMatrix();
    double* mData = vtkMatrix->GetData();
    glm::mat4 entityTransform;
    for (uint32_t i = 0; i < 4; ++i)
    {
      for (uint32_t j = 0; j < 4; ++j)
      {
        entityTransform[i][j] = (float)vtkMatrix->GetElement(i, j);
      }
    }
    entity->setTransform(entityTransform);

    double* bounds = data->GetBounds();
    double* spacing = data->GetSpacing();
    unsigned short* scalars = (unsigned short *)data->GetScalarPointer();

    shared_ptr<Volume> jVolume = make_shared<Volume>("Vtk Volume Component");
    jVolume->setBounds(bounds);
    jVolume->setSpacing(spacing);
    jVolume->setDimensions(dim);
    jVolume->setImageData(scalars);

    shared_ptr<Texture> texture = make_shared<Texture>("Volume Image", dim[0], dim[1], dim[2],
      sizeof(unsigned short), width * height * depth * sizeof(unsigned short), Texture::FORMAT_VOLUME);
    texture->setData((unsigned char*)scalars);

    shared_ptr<Material> material = make_shared<Material>("VolumeMaterial", Material::VOLUME);
    material->setAlbedoTexture(texture);
    jVolume->setMaterial(material);

    shared_ptr<Renderable> renderable = make_shared<Renderable>("Volume Renderable");
    renderable->addGeometry(jVolume);

    entity->addComponent(renderable);
  }
}
