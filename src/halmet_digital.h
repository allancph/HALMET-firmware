#ifndef __SRC_HALMET_DIGITAL_H__
#define __SRC_HALMET_DIGITAL_H__

#include "sensesp/sensors/sensor.h"
#include "sensesp/transforms/frequency.h" // Required for Frequency type

using namespace sensesp;

Frequency* ConnectTachoSender(int pin, String name);
BoolProducer* ConnectAlarmSender(int pin, String name);

#endif
