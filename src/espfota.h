/*
   ESP8266 Firmware OTA
   Date: February 2021
   Author: Jackson Linus 
   Purpose: Perform an OTA update from a bin located on a webserver also fetch bin by passing device mac address or
            device id to the url parameter (Support HTTP Only)
*/


#ifndef ESPFOTA_H
#define ESPFOTA_H

#include "Arduino.h"

class EspFota
{
public:
  EspFota(String firwmareType, int firwmareVersion);
  bool forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath);
  void execOTA();
  void recordUpdatedDevice();
  bool execHTTPcheck();
  bool useDeviceID;
  bool useDeviceMAC;
  bool trackUpdatedDevice;
  String checkUpdateURL;
  String trackUpdatedDeviceURL;


private:
  String getHeaderValue(String header, String headerName);
  String getDeviceID();
  String getDeviceMAC();
  String _firwmareType;
  int _firwmareVersion;
  String _host;
  String _bin;
  int _port;

};

#endif