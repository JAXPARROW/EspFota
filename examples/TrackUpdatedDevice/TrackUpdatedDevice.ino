/**
   esp8266 firmware OTA
   
   Purpose: Perform an OTA update from a bin located on a webserver (HTTP Only)

   Setup:
   Step 1 : Set your WiFi (ssid & password)
   Step 2 : set espfota()
   
   Upload:
   Step 1 : Menu > Sketch > Export Compiled Library. The bin file will be saved in the sketch folder (Menu > Sketch > Show Sketch folder)
   Step 2 : Upload it to your webserver
   Step 3 : Update your firmware JSON file ( see firwmareupdate )

*/

#include <espfota.h>
#include <ESP8266WiFi.h>

// Change to your WiFi credentials
const char *ssid = "";
const char *password = "";

// EspFota fota("<Type of Firme for this device>", <this version>);
EspFota fota("bedroom_lights", 1);

void setup()
{

  fota.trackUpdatedDevice = true; //boolean true or false to specify if you want to track updated devices
  fota.trackUpdatedDeviceURL = "http://server/updated/"; //pass the url for the endpoint (encoded used=Json)


  fota.checkUpdateURL = "http://server/updates/firmware.json";
  Serial.begin(115200);
  setup_wifi();
}

void setup_wifi()
{
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println(WiFi.localIP());
}

void loop()
{

  bool updatedNeeded = fota.execHTTPcheck();
  if (updatedNeeded)
  {
    fota.execOTA();
  }

  delay(2000);
}
