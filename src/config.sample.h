#pragma once

// Set to true to enable additional debug output
#define ENABLE_DEBUG false

// System settings
#define SYSTEM_HOSTNAME "smartmeter-1"

// WiFi settings
#define WIFI_STA_SSID "SSID"
#define WIFI_STA_PASSWORD "PASSWORD"

// Webserver settings
#define WEBSERVER_PORT 80

// Updater settings
#define UPDATER_USERNAME "admin"
#define UPDATER_PASSWORD "PASSWORD"

// Analog input pin for TCR5000
#define SENSOR_ANALOG_PIN A0

// SignalDetector settings
// The amount of measurements to calculate the mean and stddev
// The bigger the number the slower the calculation
#define LAG_SIZE 300

// The number of measurements needed with the same state before a new state is accepted
#define COUNT_FOR_DETECTION 3
#define COUNT_FOR_RELEASE 3

// The threashold of the Z-Score when a measurement is accepted as a signal
#define ZSCORE_THREASHOLD 8