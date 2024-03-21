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

    // m3 per second

    add_sample(CurveInterpolator::Sample(1000, 0.0000006944));
    add_sample(CurveInterpolator::Sample(1200, 0.0000009722));
    add_sample(CurveInterpolator::Sample(1500, 0.000001389));
    add_sample(CurveInterpolator::Sample(1700, 0.000001851));
    add_sample(CurveInterpolator::Sample(2000, 0.000002314));
    add_sample(CurveInterpolator::Sample(2200, 0.000002778));
    add_sample(CurveInterpolator::Sample(2500, 0.000003241));
    add_sample(CurveInterpolator::Sample(2700, 0.000003704));
    add_sample(CurveInterpolator::Sample(3000, 0.000004167));

    // m3 per hour per hour

// add_sample(CurveInterpolator::Sample(1000, 2.49984));
// add_sample(CurveInterpolator::Sample(1200, 3.49992));
// add_sample(CurveInterpolator::Sample(1500, 5.000399999999999));
// add_sample(CurveInterpolator::Sample(1700, 6.6636));
// add_sample(CurveInterpolator::Sample(2000, 8.3304));
// add_sample(CurveInterpolator::Sample(2200, 10.000799999999998));
// add_sample(CurveInterpolator::Sample(2500, 11.6676));
// add_sample(CurveInterpolator::Sample(2700, 13.3344));
// add_sample(CurveInterpolator::Sample(3000, 15.0012));

// liters per hour

// add_sample(CurveInterpolator::Sample(1000, 0.8));
// add_sample(CurveInterpolator::Sample(1200, 1.2));
// add_sample(CurveInterpolator::Sample(1500, 1.3));
// add_sample(CurveInterpolator::Sample(1700, 1.6));
// add_sample(CurveInterpolator::Sample(2000, 2.4));
// add_sample(CurveInterpolator::Sample(2200, 3.5));
// add_sample(CurveInterpolator::Sample(2500, 4.9));
// add_sample(CurveInterpolator::Sample(2700, 6.5));
// add_sample(CurveInterpolator::Sample(3000, 8.6));
  
  }
};

#endif  // __SRC_FUEL_INTERPRETER_H__
