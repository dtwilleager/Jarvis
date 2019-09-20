cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project (Jarvis)

find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkIOImage
  vtkIOXML
  vtkInteractionWidgets
  vtkRenderingVolume${VTK_RENDERING_BACKEND}
)

set (EXTERNALS_DIR
  externals/
)

include(
  ${VTK_USE_FILE}
)
  
include_directories(
  ${EXTERNALS_DIR}/DevIL64/include
  ${EXTERNALS_DIR}/assimp/include
  ${EXTERNALS_DIR}/glew-1.10.0/include
  ${EXTERNALS_DIR}/glm
)

link_directories(
  ${EXTERNALS_DIR}/DevIL64
  ${EXTERNALS_DIR}/DevIL64/unicode
  ${EXTERNALS_DIR}/glew-1.10.0/lib/Release/x64
  ${EXTERNALS_DIR}/assimp/lib/x64
)

set (SOURCE_FILES
  Component.cpp
  Component.h
  CpuTimer.cpp
  CpuTimer.h
  d3dx12.h
  Entity.cpp
  Entity.h  
  FirstPersonProcessor.cpp
  FirstPersonProcessor.h
  Geometry.cpp
  Geometry.h  
  Graphics.cpp
  Graphics.h
  GraphicsDX12.cpp
  GraphicsDX12.h
  GraphicsOpenGL.cpp
  GraphicsOpenGL.h
  Jarvis.cpp
  Jarvis.h
  Light.cpp
  Light.h
  Material.cpp
  Material.h
  Mesh.cpp
  Mesh.h
  ModelLoader.cpp
  ModelLoader.h
  Processor.cpp
  Processor.h
  Renderable.cpp
  Renderable.h
  RenderBuffer.cpp
  RenderBuffer.h
  RenderScreenView.cpp
  RenderScreenView.h
  RenderTechnique.cpp
  RenderTechnique.h
  Resource.h
  RotationProcessor.cpp
  RotationProcessor.h
  stdafx.cpp
  stdafx.h
  targetver.h
  Texture.cpp
  Texture.h
  TranslationProcessor.cpp
  TranslationProcessor.h
  View.cpp
  View.h
  Volume.cpp
  Volume.h
  VtkLoader.cpp
  VtkLoader.h
  WorldManager.cpp
  WorldManager.h
  Jarvis.rc
)  

source_group ("" FILES ${SOURCE_FILES})

set (RENDERING_EXAMPLES_SRCS
  Jarvis
)

set (JARVIS_LIBRARIES
  openGL32
  glu32
  glew32
  DevIL
  assimp
)

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} ${SOURCE_FILES})
  set_target_properties (${name} PROPERTIES WIN32_EXECUTABLE true)
  target_link_libraries(${name} ${VTK_LIBRARIES} ${JARVIS_LIBRARIES})
endforeach()



