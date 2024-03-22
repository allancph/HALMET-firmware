#ifndef __SRC_FUEL_INTERPRETER_H__
#define __SRC_FUEL_INTERPRETER_H__

#include "sensesp/sensors/sensor.h"
#include "sensesp/transforms/curveinterpolator.h"


using namespace sensesp;

class FuelInterpreter : public CurveInterpolator {
 public:
  FuelInterpreter(String config_path = "")
      : CurveInterpolator(NULL, config_path) {
    // Populate a lookup table to translate RPM to m3/s
    clear_samples();
    // addSample(CurveInterpolator::Sample(RPM, m3/s));

    // sample numbers for yanmar 3jh4e


// Conversion factor from liters per hour to m³/s
double conversionFactor = 1.0 / 3600000;

add_sample(CurveInterpolator::Sample(1000, 0.8 * conversionFactor));
add_sample(CurveInterpolator::Sample(1200, 1.2 * conversionFactor));
add_sample(CurveInterpolator::Sample(1500, 1.3 * conversionFactor));
add_sample(CurveInterpolator::Sample(1700, 1.6 * conversionFactor));
add_sample(CurveInterpolator::Sample(2000, 2.4 * conversionFactor));
add_sample(CurveInterpolator::Sample(2200, 3.5 * conversionFactor));
add_sample(CurveInterpolator::Sample(2500, 4.9 * conversionFactor));
add_sample(CurveInterpolator::Sample(2700, 6.5 * conversionFactor));
add_sample(CurveInterpolator::Sample(3000, 8.6 * conversionFactor));
  
  }
};

#endif  // __SRC_FUEL_INTERPRETER_H__
