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
    add_sample(CurveInterpolator::Sample(500, 0.00000011));
    add_sample(CurveInterpolator::Sample(1000, 0.00000019));
    add_sample(CurveInterpolator::Sample(1500, 0.0000003));
    add_sample(CurveInterpolator::Sample(1800, 0.00000041));
    add_sample(CurveInterpolator::Sample(2000, 0.00000052));
    add_sample(CurveInterpolator::Sample(2200, 0.00000066));
    add_sample(CurveInterpolator::Sample(2400, 0.00000079));
    add_sample(CurveInterpolator::Sample(2600, 0.00000097));
    add_sample(CurveInterpolator::Sample(2800, 0.00000124));
    add_sample(CurveInterpolator::Sample(3000, 0.00000153));
    add_sample(CurveInterpolator::Sample(3200, 0.00000183));
    add_sample(CurveInterpolator::Sample(3400, 0.000002));
    add_sample(CurveInterpolator::Sample(3800, 0.00000205));
  }
};

#endif  // __SRC_FUEL_INTERPRETER_H__
