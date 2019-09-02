#include "stdafx.h"
#include "Light.h"

using std::make_shared;

namespace Jarvis
{
  Light::Light(string name, Type type, bool castShadow) : Component(name, Component::LIGHT),
    m_type(type),
    m_dirty(true),
    m_position(vec3()),
    m_direction(vec3(0.0f, 0.0f, -1.0f)),
    m_ambient(vec3(0.2f, 0.2f, 0.2f)),
    m_diffuse(vec3(0.8f, 0.8f, 0.8f)),
    m_specular(vec3(0.2f, 0.2f, 0.2f)),
    m_innerConeAngle(0.0f),
    m_outerConeAngle(90.0f),
    m_attenuation(vec3(1.0f, 0.0f, 0.0f)),
    m_castShadow(castShadow)
  {
    if (castShadow)
    {
      //if (m_type == DIRECTIONAL)
      //{
      //  m_shadowView = make_shared<Jarvis::View>(name + " shadow view", View::SHADOW);
      //  m_shadowView->setViewportSize(vec2(2048, 2048));
      //}
      //else if (m_type == POINT)
      //{
      //  m_shadowView = make_shared<Jarvis::View>(name + " shadow cube view", View::SHADOW_CUBE);
      //  m_shadowView->setViewportSize(vec2(1024, 1024));
      //}
    }
  }


  Light::~Light()
  {
  }

  Light::Type Light::getLightType()
  {
    return m_type;
  }

  void Light::setDirty(bool dirty)
  {
    m_dirty = dirty;
  }

  bool Light::isDirty()
  {
    return m_dirty;
  }

  void Light::setPosition(vec3& position)
  {
    m_position = position;
  }

  void Light::getPosition(vec3& position)
  {
    position = m_position;
  }

  void Light::setViewPosition(vec3& position)
  {
    m_viewPosition = position;
  }

  void Light::getViewPosition(vec3& position)
  {
    position = m_viewPosition;
  }

  void Light::setDirection(vec3& direction)
  {
    m_direction = direction;
  }

  void Light::getDirection(vec3& direction)
  {
    direction = m_direction;
  }

  void Light::setAmbient(vec3& ambient)
  {
    m_ambient = ambient;
  }

  void Light::getAmbient(vec3& ambient)
  {
    ambient = m_ambient;
  }

  void Light::setDiffuse(vec3& diffuse)
  {
    m_diffuse = diffuse;
  }

  void Light::getDiffuse(vec3& diffuse)
  {
    diffuse = m_diffuse;
  }

  void Light::setSpecular(vec3& specular)
  {
    m_specular = specular;
  }

  void Light::getSpecular(vec3& specular)
  {
    specular = m_specular;
  }

  void Light::setInnerConeAngle(float angle)
  {
    m_innerConeAngle = angle;
  }

  float Light::getInnerConeAngle()
  {
    return m_innerConeAngle;
  }

  void Light::setOuterConeAngle(float angle)
  {
    m_outerConeAngle = angle;
  }

  float Light::getOuterConeAngle()
  {
    return m_outerConeAngle;
  }

  void Light::setAttenuation(vec3& attenuation)
  {
    m_attenuation = attenuation;
  }

  void Light::getAttenuation(vec3& attenuation)
  {
    attenuation = m_attenuation;
  }

  void Light::setCastShadow(bool castShadow)
  {
    m_castShadow = castShadow;
  }

  bool Light::getCastShadow()
  {
    return m_castShadow;
  }

  shared_ptr<View> Light::getShadowView()
  {
    return m_shadowView;
  }
}