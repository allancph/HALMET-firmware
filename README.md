# HALMET Example Firmware

This repository provides example firmware for [HALMET: Hat Labs Marine Engine & Tank interface](https://shop.hatlabs.fi/products/halmet).

To get started with the example firmware, follow the generic SensESP [Getting Started](https://signalk.org/SensESP/pages/getting_started/) instructions but use this repository instead of the SensESP Project Template.

By default, the example firmware is configured to read the engine RPM from input D1 and the fuel level from input A1. D2 is configured as a low oil pressure alarm input.

To customize the software for your own purposes, edit the `src/main.cpp` file.
Parts intended to be customized are marked with `EDIT:` comments.
This includes configuring new sensors like coolant temperature and fuel flow,
including their pin assignments, sensor-specific calibrations (e.g. thermistor curves,
pulses per liter), and unit conversions.

## Supported Engine Parameters

This firmware example provides functionality for the following engine parameters:

*   **Engine RPM:** Read from digital input D1. Output to NMEA 2000 (PGN 127488) and Signal K.
*   **Fuel Level:** Read from analog input A1. Output to NMEA 2000 (PGN 127505) and Signal K.
*   **Low Oil Pressure Alarm:** Read from digital input D2. Output to NMEA 2000 (PGN 127489) and Signal K.
*   **Over Temperature Alarm:** Read from digital input D3 (active low). Output to NMEA 2000 (PGN 127489) and Signal K.
*   **Coolant Temperature:**
    *   Sensed via Analog Input (default A3 in `src/main.cpp`).
    *   Requires user configuration of a proper sensor transform (e.g., CurveInterpolator for a thermistor) to convert voltage to temperature. The provided Linear transform is a placeholder.
    *   The `calibration_factor` in the `ADS1115VoltageInput` also needs adjustment for the specific voltage divider and sensor.
    *   Output to NMEA 2000 (PGN 127489, expects Kelvin) and Signal K (expects Kelvin). Conversion to Kelvin needs to be implemented after Celsius conversion.
*   **Fuel Flow Rate:**
    *   Sensed via Digital Input (default D4 in `src/main.cpp` using `ConnectTachoSender`).
    *   Requires user configuration of the 'Tacho fuelFlow/Revolution Multiplier' based on the fuel flow sensor's pulses per volume unit (e.g., pulses/liter).
    *   Requires user configuration of the subsequent `Linear` transform's multiplier to accurately convert the sensor's frequency output to m³/s for Signal K.
    *   Output to NMEA 2000 (PGN 127489, typically L/h) and Signal K (m³/s). Unit conversions need careful setup.
