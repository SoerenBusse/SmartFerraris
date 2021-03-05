#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include "config.h"
#include "CircularVector.h"
#include "SignalDetector.h"

// Inspired by MarcusWichelmann: https://github.com/MarcusWichelmann/ESP8266-SmartMeter-Exporter/blob/master/src/main.cpp

#if !LWIP_IPV6
#error Please select a lwIP variant with IPv6 support.
#endif

ESP8266WebServer _webServer{WEBSERVER_PORT};
ESP8266HTTPUpdateServer _updater{true};

CircularVector *_circularVector = nullptr;
SignalDetector *_signalDetector = nullptr;

unsigned long _lastDetectionTime = 0;
unsigned long _detectionTimeDifference = 0;

long _lastDetectionWattage = 0;
int _lastDetectionValue = 0;
int _lastDetectionMean = 0;
float _lastDetectionStddev = 0;
float _lastDetectionZScore = 0;
int _lastMeasurementValue = 0;

unsigned long _lastMeasurementTime = 0;
unsigned long _totalMeasurementCount = 0;

void connectToWiFi()
{
  Serial.printf("Connecting to wifi SSID \"%s\" ...", WIFI_STA_SSID);

  // Configure WiFi/network stack
  WiFi.mode(WIFI_STA);
  WiFi.hostname(SYSTEM_HOSTNAME);

  if (WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD) == WL_CONNECT_FAILED)
  {
    Serial.println(" BEGIN FAILED");
    ESP.restart();
  }

  // Wait for IPv4 & IPv6 addresses other than link-local
  bool hasV4 = false, hasv6 = false;
  while (!hasV4 || !hasv6)
  {
    for (auto entry : addrList)
    {
      IPAddress addr = entry.addr();
      if (addr.isLocal())
        continue;

      if (!hasV4)
        hasV4 = addr.isV4();
      if (!hasv6)
        hasv6 = addr.isV6();
    }

    Serial.print('.');
    delay(500);
  }

  Serial.println(" OK");
  Serial.println();

  // Print ip addresses
  Serial.println("IP addresses:");
  for (auto entry : addrList)
  {
    Serial.printf("[%s] %s", entry.ifname().c_str(), entry.toString().c_str());

    if (entry.isLegacy())
      Serial.printf(", mask: %s, gateway: %s", entry.netmask().toString().c_str(), entry.gw().toString().c_str());

    if (entry.isLocal())
      Serial.print(" (link-local)");

    Serial.println();
  }
  Serial.println();

  // Print dns server list
  Serial.print("DNS servers:");
  for (int i = 0; i < DNS_MAX_SERVERS; i++)
  {
    IPAddress dns = WiFi.dnsIP(i);
    if (dns.isSet())
      Serial.printf(" %s", dns.toString().c_str());
  }
  Serial.println();
  Serial.println();
}

void handleRootRequest()
{
  _webServer.send(200, "text/plain; charset=utf-8",
                  "ESP8266 SmartFerraris Exporter for Prometheus\n\n"
                  "Metrics URL: /metrics\n"
                  "Update URL: /update\n");
}

void handleRebootRequest()
{
  _webServer.send(200, "text/plain; charset=utf-8", "Rebooting!");
  ESP.restart();
}

void handleMetricsRequest()
{
  static const char *metricsTemplate =
      "# HELP esp_uptime The uptime of the ESP in milliseconds.\n"
      "# TYPE esp_uptime counter\n"
      "esp_uptime %u\n\n"
      "# HELP esp_free_heap The free heap of the ESP.\n"
      "# TYPE esp_free_heap gauge\n"
      "esp_free_heap %u\n\n"
      "# HELP smartferraris_measurement_count The total measurements taken since start.\n"
      "# TYPE smartferraris_measurement_count counter\n"
      "smartferraris_measurement_count %d\n\n"
      "# HELP smartferraris_current_wattage The currently used energy in W.\n"
      "# TYPE smartferraris_current_wattage gauge\n"
      "smartferraris_current_wattage %d\n\n"
      "# HELP smartferraris_last_value The last analog value read from TCR5000.\n"
      "# TYPE smartferraris_last_value gauge\n"
      "smartferraris_last_value %d\n\n"
      "# HELP smartferraris_last_value The last value of a detected signal.\n"
      "# TYPE smartferraris_last_value gauge\n"
      "smartferraris_last_signal_value %d\n\n"
      "# HELP smartferraris_calculated_mean The calculated mean of the lag.\n"
      "# TYPE smartferraris_calculated_mean counter\n"
      "smartferraris_calculated_mean %d\n\n"
      "# HELP smartferraris_calculated_stddev The calculated standard derivation of the lag.\n"
      "# TYPE smartferraris_calculated_stddev counter\n"
      "smartferraris_calculated_stddev %f\n\n"
      "# HELP smartferraris_calculated_zscore The calculated zscore of the lag.\n"
      "# TYPE smartferraris_calculated_zscore gauge\n"
      "smartferraris_calculated_zscore %f\n\n"
      "# HELP smartferraris_detection_time_difference The difference between each detection.\n"
      "# TYPE smartferraris_detection_time_difference gauge\n"
      "smartferraris_detection_time_difference %d\n\n"
      "# HELP smartferraris_current_mean The current mean.\n"
      "# TYPE smartferraris_current_mean gauge\n"
      "smartferraris_current_mean %d\n\n";

  // Make sure the pointer is initialized
  if (_circularVector == nullptr)
  {
    _webServer.send(500, "text/plain; charset=utf-8", "CircularVector is nullptr");
  }

  char responseBuffer[2000];
  snprintf(responseBuffer, sizeof(responseBuffer), metricsTemplate,
           millis(),
           ESP.getFreeHeap(),
           _totalMeasurementCount,
           _lastDetectionWattage,
           _lastMeasurementValue,
           _lastDetectionValue,
           _lastDetectionMean,
           _lastDetectionStddev,
           _lastDetectionZScore,
           _detectionTimeDifference,
           _circularVector->GetCurrentMean());

  _webServer.send(200, "text/plain; charset=utf-8", responseBuffer);
}

void startWebServer()
{
  Serial.print("Starting web server ...");

  _webServer.on("/", handleRootRequest);
  _webServer.on("/metrics", handleMetricsRequest);
  _webServer.on("/reboot", handleRebootRequest);
  _webServer.begin();

  Serial.println(" OK");
}

void configureUpdater()
{
  Serial.print("Configuring updater ...");

  _updater.setup(&_webServer, "/update", UPDATER_USERNAME, UPDATER_PASSWORD);

  Serial.println(" OK");
}

void onSignalDetected()
{
  // Save only last detection time on first detection
  if (_lastDetectionTime == 0)
  {
    Serial.println("Got first detection");
    _lastDetectionTime = millis();
    return;
  }

  Serial.println("Got detection");

  // Calculate time difference between detection
  _detectionTimeDifference = millis() - _lastDetectionTime;

  // Calculate wattage
  long wattage = 3600000000 / (KWH_PER_U * _detectionTimeDifference);

  _lastDetectionWattage = wattage;
  _lastDetectionTime = millis();

  // Save statistics of this detection
  _lastDetectionValue = _circularVector->GetCurrentValue();
  _lastDetectionMean = _circularVector->GetCurrentMean();
  _lastDetectionStddev = _circularVector->GetCurrentStddev();
  _lastDetectionZScore = _circularVector->GetCurrentZScore();
}

void configureSmartFerraris()
{
  _circularVector = new CircularVector(LAG_SIZE, MIN_THRESHOLD, ZSCORE_THREASHOLD);
  _signalDetector = new SignalDetector(_circularVector, COUNT_FOR_DETECTION, COUNT_FOR_RELEASE, &onSignalDetected);
}

void setup()
{
  // Turn builtin LED on during setup
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  Serial.setDebugOutput(ENABLE_DEBUG);
  Serial.println();

  Serial.printf("-- SmartFerraris Exporter (%s, %s) --\n", SYSTEM_HOSTNAME, WiFi.macAddress().c_str());
  Serial.println(ESP.getFullVersion());
  Serial.println();

  connectToWiFi();
  startWebServer();
  configureUpdater();
  configureSmartFerraris();

  Serial.println("Ready.");
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  if (millis() >= _lastMeasurementTime + MEASUREMENT_DELAY)
  {
    _lastMeasurementTime = millis();

    int analogValue = analogRead(SENSOR_ANALOG_PIN);
    _lastMeasurementValue = analogValue;
    _totalMeasurementCount++;

    _signalDetector->AddMeasurement(analogValue);
  }

  _webServer.handleClient();
}