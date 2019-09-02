#include "stdafx.h"
#include "Processor.h"

namespace Jarvis
{
  Processor::Processor(string name, bool sendKeyboardEvents, bool sendMouseEvents): Component(name, Component::PROCESS),
    m_sendKeyboardEvents(sendKeyboardEvents),
    m_sendMouseEvents(sendMouseEvents)
  {
  }


  Processor::~Processor()
  {
  }

  bool Processor::sendKeyboardEvents()
  {
    return m_sendKeyboardEvents;
  }

  bool Processor::sendMouseEvents()
  {
    return m_sendMouseEvents;
  }

  void Processor::handleKeyboard(MSG* event)
  {

  }

  void Processor::handleMouse(MSG* event)
  {

  }

  void Processor::execute(double absoluteTime, double deltaTime)
  {
  }
}
