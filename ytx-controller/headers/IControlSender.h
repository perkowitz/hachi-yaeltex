// Interface for an object that sends control events to a ControlReceiver.

#ifndef ICONTROL_SENDER_H
#define ICONTROL_SENDER_H

#include <stdint.h>
#include "Arduino.h"
#include "IControlReceiver.h"


class IControlSender {

  public:
    virtual void setControlReceiver(IControlReceiver receiver) = 0;

  private:

};




#endif
