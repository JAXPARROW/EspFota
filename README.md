# espfota library for Arduino

## Purpose

A simple library to add support for Over-The-Air (OTA) updates to your project.

## Features

- [x] Web update (requires web server)
- [x] Batch firmware sync
- [x] Force firmware update
- [x] Check update from the server
- [x] Check update by specific device id
- [x] Check update by specific device mac
- [x] Record device whic updated sucessfull (optional and requires web server accepting POST data in Json format)

## How it works

This library tries to access a JSON file hosted on a webserver or json response from the api end point with key and values expected to be deserialized by the Esp8266 device, and reviews it to decide if a newer firmware has been published, if so it will download it and install it.

There are a few things that need to be in place for an update to work.

- A webserver with the firmware information in a JSON file
- Firmware version
- Firmware type
- Firmware bin location

## Usage

### Hosted JSON or JSON Response

This is hosted by a webserver or can be a json response from the endpoint and contains information about the latest firmware for the specific device.

```json
{
    "type": "esp8266-fota-http",
    "version": 3,
    "host": "192.168.2.100",
    "port": 80,
    "bin": "/fota/esp8266-fota-http-3.bin"
}
```

#### Firmware types

Types are used to compare with the current loaded firmware, this is used to make sure that when loaded, the device will still do the intended job.

As an example, a device used as a data logger should ony be updated with new versions of the data logger.

### Sketch

In this example a version 1  of 'espfota-fota-http' is in use, it would be updated when using the JSON example.

```cpp
#include <espfota.h>
#include <ESP8266WiFi.h>

const char *ssid = "";
const char *password = "";

espfota espfota("esp8266-fota-http", 1);

void setup(){
  espfota.checkURL = "http://server/fota/fota.json";
  Serial.begin(115200);
  setup_wifi();
}

void setup_wifi(){
  delay(10);
  Serial.print("Connecting to ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}

void loop(){
  bool updatedNeeded = espfota.execHTTPcheck();
  if (updatedNeeded)
  {
    espfota.execOTA();
  }
  delay(2000);
}
```

