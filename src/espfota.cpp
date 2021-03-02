/*
   ESP8266 Firmware OTA
   Date: February 2021
   Author: Jackson Linus 
   Purpose: Perform an OTA update from a bin located on a webserver also fetch bin by passing device mac address or
            device id to the url parameter (Support HTTP Only)
*/


#include "espfota.h"
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "ArduinoJson.h"
//#include <Update.h>



EspFota::EspFota(String firwmareType, int firwmareVersion)
{
    _firwmareType = firwmareType;
    _firwmareVersion = firwmareVersion;
    useDeviceID = false;
    useDeviceMAC = false;
}

// Utility to extract header value from headers
String EspFota::getHeaderValue(String header, String headerName)
{
    return header.substring(strlen(headerName.c_str()));
}

// OTA Logic
void EspFota::execOTA()
{

    WiFiClient client;
    int contentLength = 0;
    bool isValidContentType = false;

    Serial.println("Connecting to: " + String(_host));
    // Connect to Webserver
    if (client.connect(_host.c_str(), _port))
    {
        // Connection Succeed.
        // Fecthing the bin
        Serial.println("Fetching Bin: " + String(_bin));

        // Get the contents of the bin file
        client.print(String("GET ") + _bin + " HTTP/1.1\r\n" +
                     "Host: " + _host + "\r\n" +
                     "Cache-Control: no-cache\r\n" +
                     "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (client.available() == 0)
        {
            if (millis() - timeout > 5000)
            {
                Serial.println("Client Timeout !");
                client.stop();
                return;
            }
        }

        while (client.available())
        {
            // read line till /n
            String line = client.readStringUntil('\n');
            // remove space, to check if the line is end of headers
            line.trim();

            if (!line.length())
            {
                //headers ended
                break; // and get the OTA started
            }

            // Check if the HTTP Response is 200
            // else break and Exit Update
            if (line.startsWith("HTTP/1.1"))
            {
                if (line.indexOf("200") < 0)
                {
                    Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
                    break;
                }
            }

            // extract headers here
            // Start with content length
            if (line.startsWith("Content-Length: "))
            {
                contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
                Serial.println("Got " + String(contentLength) + " bytes from server");
            }

            // Next, the content type
            if (line.startsWith("Content-Type: "))
            {
                String contentType = getHeaderValue(line, "Content-Type: ");
                Serial.println("Got " + contentType + " payload.");
                if (contentType == "application/octet-stream")
                {
                    isValidContentType = true;
                }
            }
        }
    }
    else
    {
        // Connect to webserver failed
        // May be try?
        // Probably a choppy network?
        Serial.println("Connection to " + String(_host) + " failed. Please check your setup");
        // retry??
        // execOTA();
    }

    // Check what is the contentLength and if content type is `application/octet-stream`
    Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

    // check contentLength and content type
    if (contentLength && isValidContentType)
    {
        // Check if there is enough to OTA Update
        bool canBegin = Update.begin(contentLength);

        // If yes, begin
        if (canBegin)
        {
            Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            // No activity would appear on the Serial monitor
            // So be patient. This may take 2 - 5mins to complete
            size_t written = Update.writeStream(client);

            if (written == contentLength)
            {
                Serial.println("Written : " + String(written) + " successfully");
                
                if(trackUpdatedDevice)
                {
                //On success updates, send payload to the server //////////////////////////////////////////////////////////
                recordUpdatedDevice();
                delay(2000);
                }
                else
                {
                    Serial.println("Don't track updated device opted");
                }

                
            }
            else
            {
                Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
                // retry??
                // execOTA();
            }

            if (Update.end())
            {
                Serial.println("OTA done!");
                if (Update.isFinished())
                {
                    Serial.println("Update successfully completed. Rebooting.");
                
                    ESP.restart();
                }
                else
                {
                    Serial.println("Update not finished? Something went wrong!");
                }
            }
            else
            {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
            }
        }
        else
        {
            // not enough space to begin OTA
            // Understand the partitions and
            // space availability
            Serial.println("Not enough space to begin OTA");
            client.flush();
        }
    }
    else
    {
        Serial.println("There was no content in the response");
        client.flush();
    }
}

bool EspFota::execHTTPcheck()
{

    String useURL;

    if (useDeviceID)
    {
        // String deviceID = getDeviceID() ;
        useURL = checkUpdateURL + "?device_id=" + getDeviceID();
    }
    if (useDeviceMAC)
    {
        // String DeviceMAC = getDeviceMAC() ;
        useURL = checkUpdateURL + "?mac_address=" + getDeviceMAC();
    }
    else
    {
        useURL = checkUpdateURL;
    }

    WiFiClient client;
    _port = 80;

    Serial.println("Getting HTTP");
    Serial.println(useURL);
    Serial.println("------");
    if ((WiFi.status() == WL_CONNECTED))
    { //Check the current connection status

        HTTPClient http;

        http.begin(useURL);        //Specify the URL
        int httpCode = http.GET(); //Make the request

        if (httpCode == 200)
        { //Check is a file was returned

            String payload = http.getString();

            int str_len = payload.length() + 1;
            char JSONMessage[str_len];
            payload.toCharArray(JSONMessage, str_len);

            StaticJsonDocument<300> JSONDocument; //Memory pool
            DeserializationError err = deserializeJson(JSONDocument, JSONMessage);

            if (err)
            { //Check for errors in parsing
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(err.c_str());
                delay(5000);
                return false;
            }


            JsonArray json_response = JSONDocument.as<JsonArray>(); //Pass Response Objects as Array 
            for (JsonObject JSONDocument : json_response)   //Iterate through objects
            {


            const char *pltype = JSONDocument["type"];
            int plversion = JSONDocument["version"];
            const char *plhost = JSONDocument["host"];
            _port = JSONDocument["port"];
            const char *plbin = JSONDocument["bin"];

            String jshost(plhost);
            String jsbin(plbin);

            _host = jshost;
            _bin = jsbin;

            String fwtype(pltype);

            if (plversion > _firwmareVersion && fwtype == _firwmareType)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

    } //End of Deserialization

        else
        {
            Serial.println("Error on HTTP request");
            return false;
        }

        http.end(); //Free the resources
        return false;
    }
    return false;
}

String EspFota::getDeviceID()
{
    char deviceid[21];
    uint64_t chipid;

    chipid = ESP.getChipId();
    
    sprintf(deviceid, "%" PRIu64, chipid);
    String thisID(deviceid);
    return thisID;
}

String EspFota::getDeviceMAC()
{
    String devicemac;
    devicemac = WiFi.macAddress();
    String thisMAC(devicemac);
    return thisMAC;
}

// Force a firmware update regartless on current version
bool EspFota::forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath)
{
    _host = firwmareHost;
    _bin = firwmarePath;
    _port = firwmarePort;
    execOTA();

    return true;
}


void EspFota::recordUpdatedDevice()
{
    if(WiFi.status()== WL_CONNECTED){

    HTTPClient httpClient;
    WiFiClient wifiClient;

    String useURL;

    useURL = trackUpdatedDeviceURL;

    httpClient.begin(useURL); //http://192.168.0.105:8000/updated/
    httpClient.addHeader("Content-Type", "application/json");

    int firmware_version = _firwmareVersion;
    String firmware_type = _firwmareType;
    String device_id = getDeviceID();
    String device_mac = getDeviceMAC();

    DynamicJsonDocument doc(300);

    doc["type"] = firmware_type;
    doc["version"] = firmware_version;
    doc["hardware_id"] = device_id;
    doc["device_id"] = device_mac;

    String data_payload;
    serializeJson(doc, data_payload);

    int httpResponseCode = httpClient.POST(data_payload);

    if(httpResponseCode>0){
        String response = httpClient.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);

        delay(2000);
        httpClient.end();
    }

    else if(httpResponseCode != 201){
        Serial.println("Error trackig -> Check your POST endpoint");
        delay(1000);

    }
    else{

        Serial.printf("Error occurred while sending HTTP POST: %s\n",httpClient.errorToString(httpResponseCode).c_str() );      
    }

    }

    else{

        Serial.println("Wi-Fi not connected, check connectivity !");
    }
}