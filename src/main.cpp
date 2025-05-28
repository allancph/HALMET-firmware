// Signal K application template file.
//
// This application demonstrates core SensESP concepts in a very
// concise manner. You can build and upload the application as is
// and observe the value changes on the serial port monitor.
//
// You can use this source file as a basis for your own projects.
// Remove the parts that are not relevant to you, and add your own code
// for external hardware libraries.

#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifdef ENABLE_NMEA2000_OUTPUT
#include <NMEA2000_esp32.h>
#endif

#include "n2k_senders.h"
#include "sensesp/net/discovery.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/system/system_status_led.h"
#include "sensesp/transforms/lambda_transform.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/ui/config_item.h"

#ifdef ENABLE_SIGNALK
#include "sensesp_app_builder.h"
#define BUILDER_CLASS SensESPAppBuilder
#else
#include "sensesp_minimal_app_builder.h"
#endif

#include "halmet_analog.h"
#include "halmet_const.h"
#include "halmet_digital.h"
#include "halmet_display.h"
#include "halmet_serial.h"
#include "sensesp/net/http_server.h"
#include "sensesp/net/networking.h"

using namespace sensesp;
using namespace halmet;

#ifndef ENABLE_SIGNALK
#define BUILDER_CLASS SensESPMinimalAppBuilder
SensESPMinimalApp* sensesp_app;
Networking* networking;
MDNSDiscovery* mdns_discovery;
HTTPServer* http_server;
SystemStatusLed* system_status_led;
#endif

/////////////////////////////////////////////////////////////////////
// Declare some global variables required for the firmware operation.

#ifdef ENABLE_NMEA2000_OUTPUT
tNMEA2000* nmea2000;
elapsedMillis n2k_time_since_rx = 0;
elapsedMillis n2k_time_since_tx = 0;
#endif

// Global I2C bus instance. Consider passing as a dependency for improved
// testability in larger projects.
TwoWire* i2c;
// Global display instance. Consider passing as a dependency for improved
// testability in larger projects.
Adafruit_SSD1306* display;

// Global array for holding alarm states for the local OLED display summary.
// Protected by kNumAlarmChannels. Store alarm states in an array for local
// display output
const int kNumAlarmChannels = 4;
bool alarm_states[kNumAlarmChannels] = {false, false, false, false};

// Set the ADS1115 GAIN to adjust the analog input voltage range.
// On HALMET, this refers to the voltage range of the ADS1115 input
// AFTER the 33.3/3.3 voltage divider.

// GAIN_TWOTHIRDS: 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
// GAIN_ONE:       1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
// GAIN_TWO:       2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
// GAIN_FOUR:      4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
// GAIN_EIGHT:     8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
// GAIN_SIXTEEN:   16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

const adsGain_t kADS1115Gain = GAIN_ONE;

/////////////////////////////////////////////////////////////////////
// Test output pin configuration. If ENABLE_TEST_OUTPUT_PIN is defined,
// GPIO 33 will output a pulse wave at 380 Hz with a 50% duty cycle.
// If this output and GND are connected to one of the digital inputs, it can
// be used to test that the frequency counter functionality is working.
#define ENABLE_TEST_OUTPUT_PIN
#ifdef ENABLE_TEST_OUTPUT_PIN
const int kTestOutputPin = GPIO_NUM_33;
// With the default pulse rate of 100 pulses per revolution (configured in
// halmet_digital.cpp), this frequency corresponds to 3.8 r/s or about 228 rpm.
const int kTestOutputFrequency = 380;
const int kTestOutputLEDCChannel = 0;
const int kTestOutputLEDCResolution = 13;
// Duty cycle for 50% at 13-bit resolution = 2^(13-1) = 4096
const int kTestOutputLEDCDuty50Pct = 4096;
#endif

// Default NMEA2000 Node Address
const uint8_t kDefaultN2kNodeAddress = 71;

// Event Loop Intervals
const int kNMEA2000ParseIntervalMs = 1;
const int kIPDisplayIntervalMs = 1000;
const int kAlarmDisplayIntervalMs = 1000;

// Configuration Sort Orders (base values)
const int kSortOrderAnalogVoltageA2 = 3000;
const int kSortOrderTankNMEA = 3005;
const int kSortOrderCoolantTempVoltage = 3006;
const int kSortOrderCoolantTempTransform = 3007;
const int kSortOrderEngineDynamicNMEA = 3010;
const int kSortOrderAlarmSKLowOil = 3011;
const int kSortOrderAlarmSKOverTemp = 3012;
const int kSortOrderTachoNMEA = 3015;
const int kSortOrderFuelFlowFrequency = 3016;
const int kSortOrderFuelFlowRateM3s = 3017;
const int kSortOrderFuelFlowRateLh = 3018;

/////////////////////////////////////////////////////////////////////
// The setup function performs one-time application initialization.
void setup() {
  SetupLogging(ESP_LOG_DEBUG);

  // These calls can be used for fine-grained control over the logging level.
  // esp_log_level_set("*", esp_log_level_t::ESP_LOG_DEBUG);

  Serial.begin(115200);

  /////////////////////////////////////////////////////////////////////
  // Initialize the application framework

  // Construct the global SensESPApp() object
  BUILDER_CLASS builder;
  sensesp_app = (&builder)
                    // EDIT: Set a custom hostname for the app.
                    ->set_hostname("halmet")
                    // EDIT: Optionally, hard-code the WiFi and Signal K server
                    // settings. This is normally not needed.
                    //->set_wifi("My WiFi SSID", "my_wifi_password")
                    //->set_sk_server("192.168.10.3", 80)
                    // EDIT: Enable OTA updates with a password.
                    //->enable_ota("my_ota_password")
                    ->get_app();

  // initialize the I2C bus
  i2c = new TwoWire(0);
  i2c->begin(kSDAPin, kSCLPin);

  // Initialize ADS1115
  auto ads1115 = new Adafruit_ADS1115();

  ads1115->setGain(kADS1115Gain);
  bool ads_initialized = ads1115->begin(kADS1115Address, i2c);
  debugD("ADS1115 initialized: %d", ads_initialized);

#ifdef ENABLE_TEST_OUTPUT_PIN
  pinMode(kTestOutputPin, OUTPUT);
  // Set the LEDC peripheral to a 13-bit resolution
  ledcSetup(kTestOutputLEDCChannel, kTestOutputFrequency,
            kTestOutputLEDCResolution);
  // Attach the channel to the GPIO pin to be controlled
  ledcAttachPin(kTestOutputPin, kTestOutputLEDCChannel);
  // Set the duty cycle to 50%
  // Duty cycle value is calculated based on the resolution
  // For 13-bit resolution, max value is 8191, so 50% is 4096
  ledcWrite(kTestOutputLEDCChannel, kTestOutputLEDCDuty50Pct);
#endif

#ifdef ENABLE_NMEA2000_OUTPUT
  /////////////////////////////////////////////////////////////////////
  // Initialize NMEA 2000 functionality

  nmea2000 = new tNMEA2000_esp32(kCANTxPin, kCANRxPin);

  // Reserve enough buffer for sending all messages.
  nmea2000->SetN2kCANSendFrameBufSize(250);
  nmea2000->SetN2kCANReceiveFrameBufSize(250);

  // Set Product information
  // EDIT: Change the values below to match your device.
  nmea2000->SetProductInformation(
      "20231229",  // Manufacturer's Model serial code (max 32 chars)
      104,         // Manufacturer's product code
      "HALMET",    // Manufacturer's Model ID (max 33 chars)
      "1.0.0",     // Manufacturer's Software version code (max 40 chars)
      "1.0.0"      // Manufacturer's Model version (max 24 chars)
  );

  // For device class/function information, see:
  // http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf

  // For mfg registration list, see:
  // https://actisense.com/nmea-certified-product-providers/
  // The format is inconvenient, but the manufacturer code below should be
  // one not already on the list.

  // EDIT: Change the class and function values below to match your device.
  nmea2000->SetDeviceInformation(
      GetBoardSerialNumber(),  // Unique number. Use e.g. Serial number.
      140,                     // Device function: Engine
      50,                      // Device class: Propulsion
      2046);                   // Manufacturer code

  nmea2000->SetMode(tNMEA2000::N2km_NodeOnly, kDefaultN2kNodeAddress);
  nmea2000->EnableForward(false);
  nmea2000->Open();

  // No need to parse the messages at every single loop iteration; 1 ms will do
  event_loop()->onRepeat(kNMEA2000ParseIntervalMs,
                         []() { nmea2000->ParseMessages(); });
#endif  // ENABLE_NMEA2000_OUTPUT

#ifndef ENABLE_SIGNALK
  // Initialize components that would normally be present in SensESPApp
  networking = new Networking("/System/WiFi Settings", "", "");
  ConfigItem(networking);
  mdns_discovery = new MDNSDiscovery();
  http_server = new HTTPServer();
  system_status_led = new SystemStatusLed(LED_BUILTIN);
#endif

  // Initialize the OLED display
  bool display_present = InitializeSSD1306(sensesp_app.get(), &display, i2c);

  ///////////////////////////////////////////////////////////////////
  // Analog inputs

#ifdef ENABLE_SIGNALK
  bool enable_signalk_output = true;
#else
  bool enable_signalk_output = false;
#endif

  // Connect the tank senders.
  // EDIT: To enable more tanks, uncomment the lines below.
  auto tank_a1_volume = ConnectTankSender(
      ads1115, 0, "Fuel", "fuel.main",
      kSortOrderAnalogVoltageA2,  // Assuming Tank A1 is related to this base
      enable_signalk_output);
  // auto tank_a2_volume = ConnectTankSender(ads1115, 1, "A2");
  // auto tank_a3_volume = ConnectTankSender(ads1115, 2, "A3");
  // auto tank_a4_volume = ConnectTankSender(ads1115, 3, "A4");

#ifdef ENABLE_NMEA2000_OUTPUT
  // Tank 1, instance 0. Capacity 200 liters. You can change the capacity
  // in the web UI as well.
  // EDIT: Make sure this matches your tank configuration above.
  N2kFluidLevelSender* tank_a1_sender = new N2kFluidLevelSender(
      "/Tanks/Fuel/NMEA 2000", 0, N2kft_Fuel, 200, nmea2000);

  ConfigItem(tank_a1_sender)
      ->set_title("Tank A1 NMEA 2000")
      ->set_description("NMEA 2000 tank sender for tank A1")
      ->set_sort_order(kSortOrderTankNMEA);

  tank_a1_volume->connect_to(&(tank_a1_sender->tank_level_));
#endif  // ENABLE_NMEA2000_OUTPUT

  if (display_present) {
    // EDIT: Duplicate the lines below to make the display show all your tanks.
    tank_a1_volume->connect_to(new LambdaConsumer<float>(
        [](float value) { PrintValue(display, 2, "Tank A1", 100 * value); }));
  }

  // Read the voltage level of analog input A2
  auto a2_voltage = new ADS1115VoltageInput(ads1115, 1, "/Voltage A2");

  ConfigItem(a2_voltage)
      ->set_title("Analog Voltage A2")
      ->set_description("Voltage level of analog input A2")
      ->set_sort_order(kSortOrderAnalogVoltageA2);

  a2_voltage->connect_to(new LambdaConsumer<float>(
      [](float value) { debugD("Voltage A2: %f", value); }));

  // If you want to output something else than the voltage value,
  // you can insert a suitable transform here.
  // For example, to convert the voltage to a distance with a conversion
  // factor of 0.17 m/V, you could use the following code:
  // auto a2_distance = new Linear(0.17, 0.0);
  // a2_voltage->connect_to(a2_distance);

#ifdef ENABLE_SIGNALK
  a2_voltage->connect_to(
      new SKOutputFloat("sensors.a2.voltage", "Analog Voltage A2",
                        new SKMetadata("V", "Analog Voltage A2")));
  // Example of how to output the distance value to Signal K.
  // a2_distance->connect_to(
  //     new SKOutputFloat("sensors.a2.distance", "Analog Distance A2",
  //                       new SKMetadata("m", "Analog Distance A2")));
#endif

  ///////////////////////////////////////////////////////////////////
  // Coolant Temperature Sensing (Analog Input A3 / ADC Channel 2)
  // This section configures sensing for engine coolant temperature.
  // It typically uses a thermistor sensor connected to an analog input.
  // The ADS1115VoltageInput reads the raw voltage.
  // IMPORTANT: The 'calibration_factor' for ADS1115VoltageInput
  // ('/Sensors/Coolant Temperature/Voltage/Calibration Factor')
  // must be adjusted based on your specific voltage divider and sensor.
  auto coolant_temp_voltage = new ADS1115VoltageInput(
      ads1115, 2,
      "/Sensors/Coolant Temperature/Voltage");  // ADC Channel 2 is A3

  ConfigItem(coolant_temp_voltage)
      ->set_title("Coolant Temperature Voltage")
      ->set_description("Raw voltage from coolant temperature sensor")
      ->set_sort_order(kSortOrderCoolantTempVoltage);

  // The Linear transform below is a PLACEHOLDER.
  // For accurate temperature readings, replace or configure this transform.
  // Common approaches:
  // 1. Use a CurveInterpolator transform with calibration data (voltage vs. temp).
  // 2. Implement a custom transform for your specific thermistor (e.g., using Steinhart-Hart equation).
  auto coolant_temperature_celsius =
      new Linear(1.0, 0.0, "/Sensors/Coolant Temperature/LinearTransform");

  coolant_temp_voltage->connect_to(coolant_temperature_celsius);

  ConfigItem(coolant_temperature_celsius)
      ->set_title("Coolant Temperature Processing")
      ->set_description(
          "Linear transform for coolant temperature (placeholder - CONFIGURE FOR YOUR SENSOR)")
      ->set_sort_order(kSortOrderCoolantTempTransform);

  // FIXME: The Linear transform is a placeholder.
  // A proper thermistor curve (e.g. Steinhart-Hart) or specific sensor
  // calibration should be implemented here to convert voltage to Celsius.
  // FIXME: NMEA 2000 (PGN 127489) and Signal K expect temperature in Kelvin.
  // The current output (even after proper Celsius conversion) needs conversion to Kelvin.

#ifdef ENABLE_NMEA2000_OUTPUT
  // engine_dynamic_sender should be initialized further down if alarms are
  // enabled If not, it needs to be initialized here or ensured it's available.
  // For now, assuming it will be available from the alarm section.
  // Connect to NMEA 2000, field expects Kelvin.
  // FIXME: Convert to Kelvin from Celsius before connecting.
  // The current coolant_temperature_celsius is actually outputting raw voltage.
  // engine_dynamic_sender->temperature_ is of type N2kSignalKTemperatureInput.
  // We need to ensure engine_dynamic_sender is initialized before this point if
  // not already. Let's find N2kEngineParameterDynamicSender instantiation. It's
  // inside the alarm section. This means coolant temperature N2K output will
  // only work if alarms are also sending to N2K. This is acceptable for now as
  // per current structure.
#endif  // NMEA2000 output is handled below, after engine_dynamic_sender is
        // initialized.

#ifdef ENABLE_SIGNALK
  coolant_temperature_celsius->connect_to(new SKOutputFloat(
      "propulsion.main.coolantTemperature", "/SignalK/Coolant Temperature",
      new SKMetadata(
          "K", "Coolant Temperature",
          "Engine Coolant Temperature (currently raw voltage, placeholder)")));
  // FIXME: The metadata unit is K, but the value is currently voltage.
  // Update when actual Celsius conversion and then Kelvin conversion is
  // implemented.
#endif

  ///////////////////////////////////////////////////////////////////
  // Digital alarm inputs

  // EDIT: More alarm inputs can be defined by duplicating the lines below.
  // Make sure to not define a pin for both a tacho and an alarm.
  auto alarm_d2_input = ConnectAlarmSender(kDigitalInputPin2, "D2");
  auto alarm_d3_input = ConnectAlarmSender(kDigitalInputPin3, "D3");
  // auto alarm_d4_input = ConnectAlarmSender(kDigitalInputPin4, "D4");

  // Update the alarm states based on the input value changes.
  // EDIT: If you added more alarm inputs, uncomment the respective lines below.
  alarm_d2_input->connect_to(
      new LambdaConsumer<bool>([](bool value) { alarm_states[1] = value; }));
  // In this example, alarm_d3_input is active low, so invert the value.
  auto alarm_d3_inverted = alarm_d3_input->connect_to(
      new LambdaTransform<bool, bool>([](bool value) { return !value; }));
  alarm_d3_inverted->connect_to(
      new LambdaConsumer<bool>([](bool value) { alarm_states[2] = value; }));
  // alarm_d4_input->connect_to(
  //     new LambdaConsumer<bool>([](bool value) { alarm_states[3] = value; }));

#ifdef ENABLE_NMEA2000_OUTPUT
  // EDIT: This example connects the D2 alarm input to the low oil pressure
  // warning. Modify according to your needs.
  N2kEngineParameterDynamicSender* engine_dynamic_sender = nullptr;
// Removed redundant inner #ifdef ENABLE_NMEA2000_OUTPUT
  engine_dynamic_sender = new N2kEngineParameterDynamicSender(
      "/NMEA 2000/Engine 1 Dynamic", 0, nmea2000);

  ConfigItem(engine_dynamic_sender)
      ->set_title("Engine 1 Dynamic")
      ->set_description("NMEA 2000 dynamic engine parameters for engine 1")
      ->set_sort_order(kSortOrderEngineDynamicNMEA);

  alarm_d2_input->connect_to(engine_dynamic_sender->low_oil_pressure_);

  // This is just an example -- normally temperature alarms would not be
  // active-low (inverted).
  alarm_d3_inverted->connect_to(engine_dynamic_sender->over_temperature_);

  // Connect coolant temperature to NMEA 2000 if sender is available
  if (engine_dynamic_sender != nullptr) {
    // FIXME: Convert to Kelvin from Celsius before connecting.
    // The current coolant_temperature_celsius is actually outputting raw
    // voltage.
    coolant_temperature_celsius->connect_to(
        engine_dynamic_sender->temperature_); // Pass shared_ptr directly
  }
#endif // ENABLE_NMEA2000_OUTPUT for engine_dynamic_sender block related to alarms and coolant temp

  // FIXME: Transmit the alarms over SK as well.

#ifdef ENABLE_SIGNALK
  // Signal K output for Low Oil Pressure (D2)
  auto sk_low_oil_pressure =
      new SKOutputBool("notifications.propulsion.main.lowOilPressure",
                       "/SignalK/Alarms/Low Oil Pressure");
  alarm_d2_input->connect_to(sk_low_oil_pressure);

  ConfigItem(sk_low_oil_pressure)
      ->set_title("SK Alarm: Low Oil Pressure")
      ->set_description("Signal K path for low oil pressure alarm")
      ->set_sort_order(kSortOrderAlarmSKLowOil);

  // Signal K output for Over Temperature (D3, inverted)
  auto sk_over_temperature =
      new SKOutputBool("notifications.propulsion.main.overTemperature",
                       "/SignalK/Alarms/Over Temperature");
  alarm_d3_inverted->connect_to(sk_over_temperature);

  ConfigItem(sk_over_temperature)
      ->set_title("SK Alarm: Over Temperature")
      ->set_description("Signal K path for over temperature alarm")
      ->set_sort_order(kSortOrderAlarmSKOverTemp);

  // FIXME: If other alarms (e.g., alarm_d4_input) are enabled,
  // connect them to their respective Signal K paths following this pattern.
#endif  // ENABLE_SIGNALK

  ///////////////////////////////////////////////////////////////////
  // Digital tacho inputs

  // Connect the tacho senders. Engine name is "main".
  // EDIT: More tacho inputs can be defined by duplicating the line below.
  Frequency* tacho_d1_frequency = ConnectTachoSender(kDigitalInputPin1, "main");

  ///////////////////////////////////////////////////////////////////
  // Fuel Flow Sensing (Digital Input D4 / kDigitalInputPin4)

  // FIXME: Configure 'Tacho fuelFlow/Revolution Multiplier' based on sensor
  // (e.g., pulses/liter). This is CRITICAL for accurate fuel flow.
  // The output of ConnectTachoSender (fuel_flow_frequency) will be:
  // - Hz (pulses/sec) if 'Revolution Multiplier' is 1.0.
  // - VolumeUnit/sec if 'Revolution Multiplier' is (1.0 / pulses_per_volume_unit).
  // For example, if sensor is 1000 pulses/liter, set multiplier to 0.001 to get Liters/sec.
  auto fuel_flow_frequency = ConnectTachoSender(kDigitalInputPin4, "fuelFlow");

  ConfigItem(
      fuel_flow_frequency)  // Assuming ConnectTachoSender returns the Frequency
                            // transform
                                ->set_title("Fuel Flow Pulses/Frequency")
                                ->set_description(
                                    "Frequency or directly converted rate from fuel flow sensor (Hz or Volume/sec)")
                                ->set_sort_order(kSortOrderFuelFlowFrequency);

  // This Linear transform converts the output of fuel_flow_frequency to m³/s for Signal K.
  // - If fuel_flow_frequency is in Hz (pulses/sec):
  //   Multiplier should be (1 / pulses_per_liter) * 0.001 to get m³/s.
  //   E.g., for 1000 pulses/liter: (1/1000) * 0.001 = 0.000001.
  // - If fuel_flow_frequency is already in Liters/sec (due to Tacho multiplier):
  //   Multiplier should be 0.001 to get m³/s.
  // FIXME: Adjust kPlaceholderHzToM3sMultiplier based on your 'Tacho fuelFlow/Revolution Multiplier' setting
  //        and sensor's pulses per volume unit to ensure output is m³/s.
  const float kPlaceholderHzToM3sMultiplier = 0.000001; // Example: assumes 1000 pulses/L and Tacho multiplier = 1.0
                                                      // Or assumes Tacho multiplier is 0.001 (L/pulse) and sensor is 1 pulse/sec (L/sec)
  auto fuel_flow_rate_m3s = new Linear(kPlaceholderHzToM3sMultiplier, 0.0,
                                       "/Sensors/Fuel Flow/RateConversionToM3s");

  fuel_flow_frequency->connect_to(fuel_flow_rate_m3s);

  ConfigItem(fuel_flow_rate_m3s)
      ->set_title("Fuel Flow Rate (m3/s)")
      ->set_description("Converts fuel flow sensor output to m3/s for Signal K. CONFIGURE MULTIPLIER.")
      ->set_sort_order(kSortOrderFuelFlowRateM3s);

#ifdef ENABLE_NMEA2000_OUTPUT
  // NMEA 2000 PGN 127489 Engine Parameters, Dynamic - Fuel Rate is typically L/h.
  // Conversion from m³/s: (m³/s) * (1000 L/m³) * (3600 s/hour) = L/hour.
  // FIXME: Verify the exact unit expected by your NMEA 2000 plotter for PGN 127489 fuel_rate_.
  //        The N2kMessages library might also have expectations for the input unit to SetN2kEngineDynamicParam.
  const float kM3sToLitersPerHour = 3600000.0;
  auto fuel_flow_rate_Lh = new Linear(kM3sToLitersPerHour, 0.0,
                                      "/Sensors/Fuel Flow/ToLitersPerHour");

  fuel_flow_rate_m3s->connect_to(fuel_flow_rate_Lh);

  ConfigItem(fuel_flow_rate_Lh)
      ->set_title("Fuel Flow Rate (L/h)")
      ->set_description(
          "Converts fuel flow rate from m3/s to L/h for NMEA 2000")
      ->set_sort_order(kSortOrderFuelFlowRateLh);

  if (engine_dynamic_sender != nullptr) {
    // FIXME: Verify NMEA2000 fuel_rate_ unit. Assuming L/h for PGN 127489.
    fuel_flow_rate_Lh->connect_to(engine_dynamic_sender->fuel_rate_); // Pass shared_ptr directly
  }
#endif // This #endif is for the NMEA2000 block for fuel_flow_rate_Lh

#ifdef ENABLE_SIGNALK
  fuel_flow_rate_m3s->connect_to(
      new SKOutputFloat("propulsion.main.fuel.rate", "/SignalK/Fuel Flow Rate",
                        new SKMetadata("m3/s", "Fuel Flow Rate",
                                       "Engine Fuel Consumption Rate")));
#endif

#ifdef ENABLE_NMEA2000_OUTPUT
  // Connect outputs to the N2k senders.
  // EDIT: Make sure this matches your tacho configuration above.
  //       Duplicate the lines below to connect more tachos, but be sure to
  //       use different engine instances.
  N2kEngineParameterRapidSender* engine_rapid_sender =
      new N2kEngineParameterRapidSender("/NMEA 2000/Engine 1 Rapid Update", 0,
                                        nmea2000);  // Engine 1, instance 0

  ConfigItem(engine_rapid_sender)
      ->set_title("Engine 1 Rapid Update")
      ->set_description("NMEA 2000 rapid update engine parameters for engine 1")
      ->set_sort_order(kSortOrderTachoNMEA);

  tacho_d1_frequency->connect_to(&(engine_rapid_sender->engine_speed_));

#endif  // ENABLE_NMEA2000_OUTPUT

  if (display_present) {
    tacho_d1_frequency->connect_to(new LambdaConsumer<float>(
        [](float value) { PrintValue(display, 3, "RPM D1", 60 * value); }));

    // Display coolant temperature
    coolant_temperature_celsius->connect_to(
        new LambdaConsumer<float>([](float value) {
          PrintValue(display, 5, "CoolantT", value);
        }));  // FIXME: Value is currently voltage, not Celsius/Kelvin

    // Display fuel flow rate (m3/s for now, could be L/min or L/h for display)
    // FIXME: Consider displaying in L/min or L/h for better readability.
    fuel_flow_rate_m3s->connect_to(new LambdaConsumer<float>([](float value) {
      PrintValue(display, 6, "FuelFlow", value * 3600000.0);
    }));  // Show L/h for now
  }

  ///////////////////////////////////////////////////////////////////
  // Display setup

  // Connect the outputs to the display
  if (display_present) {
#ifdef ENABLE_SIGNALK
    event_loop()->onRepeat(kIPDisplayIntervalMs, []() {
      PrintValue(display, 1, "IP:", WiFi.localIP().toString());
    });
#endif

    // Create a poor man's "christmas tree" display for the alarms
    event_loop()->onRepeat(kAlarmDisplayIntervalMs, []() {
      char state_string[kNumAlarmChannels + 1] = {};  // +1 for null terminator
      for (int i = 0; i < kNumAlarmChannels; i++) {
        state_string[i] = alarm_states[i] ? '*' : '_';
      }
      PrintValue(display, 4, "Alarm", state_string);
    });
  }

  // To avoid garbage collecting all shared pointers created in setup(),
  // loop from here.
  while (true) {
    loop();
  }
}

void loop() { event_loop()->tick(); }
