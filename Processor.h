#pragma once

#include "Component.h"

#include <string>

using std::string;

namespace Jarvis
{
  class Processor: public Component
  {
  public:
    Processor(string name, bool sendKeyboardEvents, bool sendMouseEvents);
    ~Processor();

    bool sendKeyboardEvents();
    bool sendMouseEvents();

    virtual void handleKeyboard(MSG* event);
    virtual void handleMouse(MSG* event);
    virtual void execute(double absoluteTime, double deltaTime);

  private:
    bool m_sendKeyboardEvents;
    bool m_sendMouseEvents;
  };
}
