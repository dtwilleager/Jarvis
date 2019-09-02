#include "stdafx.h"
#include "View.h"

#define _USE_MATH_DEFINES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <math.h>
#include <glm/gtx/euler_angles.hpp>

namespace Jarvis
{
  View::View(string name) :
    m_fieldOfView(90.0f),
    m_nearClip(0.01f),
    m_farClip(10000.0f),
    m_viewportPosition(vec2(0, 0)),
    m_viewportSize(vec2(1, 1)),
    m_graphicsData(nullptr)
  {
    computeTransforms();
  }

  View::~View()
  {
  }

  void View::getViewTransform(mat4& viewTransform)
  {
    viewTransform = m_viewMatrix;
  }

  void View::setViewTransform(mat4& viewTransform)
  {
    m_viewMatrix = viewTransform;
  }

  void View::getProjectionTransform(mat4& projectionTransform)
  {
    projectionTransform = m_projectionMatrix;
  }

  void View::setEye(vec3 eye)
  {
    m_eye = eye;
  }

  vec3 View::getEye()
  {
    return m_eye;
  }

  void View::setTarget(vec3 target)
  {
    m_target = target;
  }

  vec3 View::getTarget()
  {
    return m_target;
  }

  void View::setUp(vec3 up)
  {
    m_up = up;
  }

  vec3 View::getUp()
  {
    return m_up;
  }


  void View::setFieldOfView(float fieldOfView)
  {
    m_fieldOfView = fieldOfView;
  }

  float View::getFieldOfView()
  {
    return m_fieldOfView;
  }

  void View::setNearClip(float nearClip)
  {
    m_nearClip = nearClip;
  }

  float View::getNearClip()
  {
    return m_nearClip;
  }

  void View::setFarClip(float farClip)
  {
    m_farClip = farClip;
  }

  float View::getFarClip()
  {
    return m_farClip;
  }

  void View::setViewportPosition(vec2& position)
  {
    m_viewportPosition = position;
  }

  void View::getViewportPosition(vec2& position)
  {
    position = m_viewportPosition;
  }

  void View::setViewportSize(vec2& size)
  {
    m_viewportSize = size;
  }

  void View::getViewportSize(vec2& size)
  {
    size = m_viewportSize;
  }

  void View::computeTransforms()
  {
    m_projectionMatrix = glm::perspective(m_fieldOfView, m_viewportSize.x / m_viewportSize.y, m_nearClip, m_farClip);
  }

  //void View::addCompositeMesh(shared_ptr<Mesh> mesh)
  //{
  //  m_compositeMeshes.push_back(mesh);
  //}

  //shared_ptr<Mesh> View::getCompositeMesh(size_t index)
  //{
  //  return m_compositeMeshes[index];
  //}

  //size_t View::numCompositeMeshes()
  //{
  //  return m_compositeMeshes.size();
  //}

  void View::setGraphicsData(void * graphicsData)
  {
    m_graphicsData = graphicsData;
  }

  void* View::getGraphicsData()
  {
    return m_graphicsData;
  }
}