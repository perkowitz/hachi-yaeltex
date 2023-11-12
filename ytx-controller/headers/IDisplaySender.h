// Interface for an object that sends display events to a DisplayReceiver.

#ifndef IDISPLAY_SENDER_H
#define IDISPLAY_SENDER_H

#include <stdint.h>
#include "Arduino.h"
#include "IDisplayReceiver.h"


class IDisplaySender {

  public:
    virtual void setDisplayReceiver(IDisplayReceiver& receiver) = 0;

  private:

};




#endif
