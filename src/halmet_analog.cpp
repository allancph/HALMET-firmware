#include "halmet_analog.h"

#include "halmet_string_utils.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/valueproducer.h"
#include "sensesp/transforms/curveinterpolator.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/ui/config_item.h"

namespace halmet {

// HALMET constant measurement current (A)
const float kMeasurementCurrent = 0.01;

// Default fuel tank size, in m3
const float kTankDefaultSize = 120. / 1000;

// Default values for the tank level curve interpolator
const float kDefaultTankSenderEmptyOhms = 0.0f;
const float kDefaultTankSenderFullOhms = 180.0f;
const float kDefaultTankSenderMaxOhms =
    1000.0f;  // Represents a sensor saturation or upper limit
const float kDefaultTankLevelMinRatio = 0.0f;   // 0% full
const float kDefaultTankLevelFullRatio = 1.0f;  // 100% full

sensesp::FloatProducer* ConnectTankSender(Adafruit_ADS1115* ads1115,
                                          int channel, const String& name,
                                          const String& sk_id, int sort_order,
                                          bool enable_signalk_output) {
  const uint ads_read_delay = 500;  // ms

  // Configure the sender resistance sensor

  auto sender_resistance =
      new sensesp::RepeatSensor<float>(ads_read_delay, [ads1115, channel]() {
        int16_t adc_output = ads1115->readADC_SingleEnded(channel);
        float adc_output_volts = ads1115->computeVolts(adc_output);
        return kVoltageDividerScale * adc_output_volts / kMeasurementCurrent;
      });

  if (enable_signalk_output) {
    String resistance_sk_config_path =
        halmet::getConfigPath("Tanks", name, "Resistance/SK Path");
    String resistance_title =
        halmet::getTitle("Tank", name, "Sender Resistance SK Path");
    String resistance_description = halmet::getDescription(
        "Signal K path for the sender resistance of the", name, "tank");
    String resistance_sk_path =
        halmet::getSkPath("tanks", sk_id, "senderResistance");
    String resistance_meta_display_name =
        halmet::getMetaDisplayName("Resistance", name, "");
    String resistance_meta_description =
        halmet::getMetaDescription("Measured tank", name, "sender resistance");

    auto sender_resistance_sk_output = new sensesp::SKOutputFloat(
        resistance_sk_path, resistance_sk_config_path.c_str(),
        new sensesp::SKMetadata("ohm", resistance_meta_display_name.c_str(),
                                resistance_meta_description.c_str()));

    ConfigItem(sender_resistance_sk_output)
        ->set_title(resistance_title.c_str())
        ->set_description(resistance_description.c_str())
        ->set_sort_order(sort_order);

    sender_resistance->connect_to(sender_resistance_sk_output);
  }

  // Configure the piecewise linear interpolator for the tank level (ratio)

  String curve_config_path =
      halmet::getConfigPath("Tanks", name, "Level Curve");
  String curve_title = halmet::getTitle("Tank", name, "Level Curve");
  String curve_description = halmet::getDescription(
      "Piecewise linear curve for the", name, "tank level");

  auto tank_level =
      (new sensesp::CurveInterpolator(nullptr, curve_config_path.c_str()))
          ->set_input_title("Sender Resistance (ohms)")
          ->set_output_title("Fuel Level (ratio)");

  ConfigItem(tank_level)
      ->set_title(curve_title.c_str())
      ->set_description(curve_description.c_str())
      ->set_sort_order(sort_order + 1);

  if (tank_level->get_samples().empty()) {
    // If there's no prior configuration, provide a default curve
    tank_level->clear_samples();
    tank_level->add_sample(sensesp::CurveInterpolator::Sample(
        kDefaultTankSenderEmptyOhms, kDefaultTankLevelMinRatio));
    tank_level->add_sample(sensesp::CurveInterpolator::Sample(
        kDefaultTankSenderFullOhms, kDefaultTankLevelFullRatio));
    tank_level->add_sample(sensesp::CurveInterpolator::Sample(
        kDefaultTankSenderMaxOhms,
        kDefaultTankLevelFullRatio));  // Assuming max ohms still means full
  }

  sender_resistance->connect_to(tank_level);

  if (enable_signalk_output) {
    String level_config_path =
        halmet::getConfigPath("Tanks", name, "Current Level SK Path");
    String level_title = halmet::getTitle("Tank", name, "Level SK Path");
    String level_description =
        halmet::getDescription("Signal K path for the", name, "tank level");
    String level_sk_path = halmet::getSkPath("tanks", sk_id, "currentLevel");
    String level_meta_display_name =
        halmet::getMetaDisplayName("Tank", name, "level");
    String level_meta_description =
        halmet::getMetaDescription("Tank", name, "level");

    auto tank_level_sk_output = new sensesp::SKOutputFloat(
        level_sk_path, level_config_path.c_str(),
        new sensesp::SKMetadata("ratio", level_meta_display_name.c_str(),
                                level_meta_description.c_str()));

    ConfigItem(tank_level_sk_output)
        ->set_title(level_title.c_str())
        ->set_description(level_description.c_str())
        ->set_sort_order(sort_order + 2);

    tank_level->connect_to(tank_level_sk_output);
  }

  // Configure the linear transform for the tank volume

  String volume_config_path =
      halmet::getConfigPath("Tanks", name, "Total Volume");
  String volume_title = halmet::getTitle("Tank", name, "Total Volume");
  String volume_description =
      halmet::getDescription("Calculated total volume of the", name, "tank");
  auto tank_volume =
      new sensesp::Linear(kTankDefaultSize, 0, volume_config_path.c_str());

  ConfigItem(tank_volume)
      ->set_title(volume_title.c_str())
      ->set_description(volume_description.c_str())
      ->set_sort_order(sort_order + 3);

  tank_level->connect_to(tank_volume);

  if (enable_signalk_output) {
    String volume_sk_config_path =
        halmet::getConfigPath("Tanks", name, "Current Volume SK Path");
    String volume_title_sk = halmet::getTitle("Tank", name, "Volume SK Path");
    String volume_description_sk =
        halmet::getDescription("Signal K path for the", name, "tank volume");
    String volume_sk_path = halmet::getSkPath("tanks", sk_id, "currentVolume");
    String volume_meta_display_name =
        halmet::getMetaDisplayName("Tank", name, "volume");
    String volume_meta_description =
        halmet::getMetaDescription("Calculated tank", name, "remaining volume");

    auto tank_volume_sk_output = new sensesp::SKOutputFloat(
        volume_sk_path, volume_sk_config_path.c_str(),
        new sensesp::SKMetadata("m3", volume_meta_display_name.c_str(),
                                volume_meta_description.c_str()));

    ConfigItem(tank_volume_sk_output)
        ->set_title(volume_title_sk.c_str())
        ->set_description(volume_description_sk.c_str())
        ->set_sort_order(sort_order + 4);

    tank_volume->connect_to(tank_volume_sk_output);
  }

  return tank_level;
}

}  // namespace halmet
