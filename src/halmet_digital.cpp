#include "halmet_digital.h"

#include "halmet_string_utils.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/transforms/frequency.h"
#include "sensesp/ui/config_item.h"

using namespace sensesp;

// Default RPM count scale factor, corresponds to 100 pulses per revolution.
// This is rarely, if ever correct.
const float kDefaultFrequencyScale = 1 / 100.;

// Update period for tacho counter in milliseconds
const int kTachoUpdatePeriodMs = 500;

FloatProducer* ConnectTachoSender(int pin, String name) {
  String config_path_pin = halmet::getConfigPath("Tacho", name, "Pin");
  String title_pin = halmet::getTitle("Tacho", name, "Pin");
  String description_pin = halmet::getDescription("Tacho", name, "Input Pin");

  auto tacho_input = new DigitalInputCounter(
      pin, INPUT, RISING, kTachoUpdatePeriodMs, config_path_pin.c_str());

  ConfigItem(tacho_input)
      ->set_title(title_pin.c_str())
      ->set_description(description_pin.c_str());

  String config_path_multiplier =
      halmet::getConfigPath("Tacho", name, "Revolution Multiplier");
  String title_multiplier = halmet::getTitle("Tacho", name, "Multiplier");
  String description_multiplier = halmet::getDescription(
      "Tacho", name,
      "Multiplier");  // This call is correct based on new signature
  auto tacho_frequency =
      new Frequency(kDefaultFrequencyScale, config_path_multiplier.c_str());

  tacho_input->connect_to(tacho_frequency);

#ifdef ENABLE_SIGNALK
  String config_path_sk =
      halmet::getConfigPath("Tacho", name, "Revolutions SK Path");
  String sk_path_rev = halmet::getSkPath("propulsion", name, "revolutions");
  String title_sk = halmet::getTitle("Tacho", name, "Signal K Path");
  String description_sk = halmet::getDescription(
      "Tacho", name, "Signal K Path");  // This call is correct

  auto tacho_frequency_sk_output =
      new SKOutputFloat(sk_path_rev, config_path_sk.c_str());

  ConfigItem(tacho_frequency_sk_output)
      ->set_title(title_sk.c_str())
      ->set_description(description_sk.c_str());

  tacho_frequency->connect_to(tacho_frequency_sk_output);
#endif

  return tacho_frequency;
}

// Read delay for alarm input state in milliseconds
const int kAlarmReadDelayMs = 100;

BoolProducer* ConnectAlarmSender(int pin, String name) {
  auto* alarm_input = new DigitalInputState(pin, INPUT, kAlarmReadDelayMs);

#ifdef ENABLE_SIGNALK
  String config_path_alarm_sk = halmet::getConfigPath("Alarm", name, "SK Path");
  String sk_path_alarm = halmet::getSkPath("alarm", name, "");
  String title_alarm_sk = halmet::getTitle("Alarm", name, "Signal K Path");
  String description_alarm_sk = halmet::getDescription(
      "Alarm", name, "Signal K Path");  // This call is correct

  auto alarm_sk_output =
      new SKOutputBool(sk_path_alarm, config_path_alarm_sk.c_str());

  ConfigItem(alarm_sk_output)
      ->set_title(title_alarm_sk.c_str())
      ->set_description(description_alarm_sk.c_str());

  alarm_input->connect_to(alarm_sk_output);
#endif

  return alarm_input;
}
