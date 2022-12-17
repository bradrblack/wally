#include "AdafruitIO_WiFi.h"
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager - use "dev" branch which PlatformIO won't install for you
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>
#include <FastLED.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "secrets.h" // like String openWeatherMapApiKey = "abcde12345";

// Replace with your country code and city
String city = "Kanata";
String countryCode = "CA";

#define buttonPin 0 // long press to reset device

char IO_USERNAME[64] = "";
char IO_KEY[64] = "";
char TIMEZONE[64] = "";
// flag for saving data
bool shouldSaveConfig = false;

static uint8_t objStorage[sizeof(AdafruitIO_WiFi)]; // RAM for the object

AdafruitIO_WiFi *io; // a pointer to the object, once it's constructed

const char *ntpServer = "pool.ntp.org";
struct tm timeinfo;
int clock_hour = 0;

char buf[25];
static const char *monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

WiFiManager wifiManager;
WiFiManagerParameter custom_IO_USERNAME("iouser", "Adafruit IO Username (optional)", IO_USERNAME, 60);
WiFiManagerParameter custom_IO_KEY("iokey", "Adafruit IO Key (optional)", IO_KEY, 60);
WiFiManagerParameter custom_TIMEZONE;

void sendMessage(String m, int tlen);
void sendMessage(char *m);

String getParam(String name)
{
  // read parameter from server, for customhmtl input
  String value;
  if (wifiManager.server->hasArg(name))
  {
    value = wifiManager.server->arg(name);
  }
  return value;
}

String httpGETRequest(const char *serverName)
{
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void showWeather()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";

    String w = httpGETRequest(serverPath.c_str());
    DynamicJsonDocument jsonBuffer(1024);

    DeserializationError error = deserializeJson(jsonBuffer, w);
    if (error)
    {

      Serial.println("de-serialization error");

      return;
    }

    String wmain;
    String wtemp;
    char ws[24];
    char wt[6];

    serializeJson(jsonBuffer["weather"][0]["main"], wmain);
    serializeJson(jsonBuffer["main"]["temp"], wtemp);

    sprintf(wt, "%0.1f~C", wtemp.toFloat());

    if (wmain == "\"Thunderstorm\"")
      wmain = String("\"T-Storm\"");

    Serial.printf("Weather: %s  Temp: %0.1f\n", wmain.substring(1, wmain.length() - 1), wtemp.toFloat());

    line1();
    green();
    sendMessage(wmain.substring(1, wmain.length() - 1), strlen(wt) - 2);

    orange();

    sendMessage(wt);
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

bool myGetLocalTime(struct tm *info)
{
  uint32_t start = millis();
  time_t now;
  while ((millis() - start) <= 10)
  {
    time(&now);
    localtime_r(&now, info);
    if (info->tm_year > (2016 - 1900))
    {
      return true;
    }
    delay(10);
  }
  return false;
}

// callback notifying us of the need to save config
void saveConfigCallback()
{

  String tzone;

  Serial.println("Saving new config");

  strcpy(IO_USERNAME, custom_IO_USERNAME.getValue());
  strcpy(IO_KEY, custom_IO_KEY.getValue());

  Serial.println("PARAM TIMEZONE = " + getParam("custom_TIMEZONE_id"));

  tzone = getParam("custom_TIMEZONE_id");
  tzone.toCharArray(TIMEZONE, tzone.length() + 1);

  DynamicJsonDocument json(256);

  json["IO_KEY"] = IO_KEY;
  json["IO_USERNAME"] = IO_USERNAME;
  json["TIMEZONE"] = TIMEZONE;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(json, Serial);
  Serial.println("");

  serializeJson(json, configFile);

  configFile.close();
  // end save
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  clear();
  sendMessage("Connect to WIFI");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void clear()
{
  Serial1.printf("%c", 0x0d);
  delay(10);
  Serial1.printf("%c", 0x1b);
  delay(3);
  Serial1.printf("[");
  delay(10);
  Serial1.printf("2");
  delay(10);
  Serial1.printf("J");
  delay(10);
  delay(10);
}
void green()
{

  Serial1.printf("%c", 0x1b);
  delay(5);
  Serial1.printf("[");
  delay(3);
  Serial1.printf("1");
  delay(3);
  Serial1.printf("%c", 0x3b);
  delay(3);
  Serial1.printf("3");
  delay(3);
  Serial1.printf("2");
  delay(3);
  Serial1.printf("m");
  delay(3);
}
void red()
{

  Serial1.printf("%c", 0x1b);
  delay(5);
  Serial1.printf("[");
  delay(3);
  Serial1.printf("1");
  delay(3);
  Serial1.printf("%c", 0x3b);
  delay(3);
  Serial1.printf("3");
  delay(3);
  Serial1.printf("1");
  delay(3);
  Serial1.printf("m");
  delay(3);
}
void orange()
{

  Serial1.printf("%c", 0x1b);
  delay(5);
  Serial1.printf("[");
  delay(3);
  Serial1.printf("1");
  delay(3);
  Serial1.printf("%c", 0x3b);
  delay(3);
  Serial1.printf("3");
  delay(3);
  Serial1.printf("3");
  delay(3);
  Serial1.printf("m");
  delay(3);
}
void line1()
{

  Serial1.printf("%c", 0x1b);
  delay(5);
  Serial1.printf("[");
  delay(3);
  Serial1.printf("H");
  delay(3);
  delay(3);
}

void line2()
{

  Serial1.printf("%c", 0x1b);
  delay(5);
  Serial1.printf("[");
  delay(3);
  Serial1.printf("2");
  delay(3);
  Serial1.printf("%c", 0x48);
  delay(3);
  delay(3);
}

void sendMessage(char *m)
{
  for (int i = 0; i < strlen(m); i++)
  {
    Serial1.print(m[i]);
    Serial.print(m[i]);
    delay(3);
  }
}
void sendMessage(String m, int tlen)
{

  for (int i = 0; i < m.length(); i++)
  {
    Serial1.print(m.substring(i, i + 1));
    Serial.print(m.substring(i, i + 1));
    delay(3);
  }
  for (int i = 0; i < (14 - m.length() - tlen); i++)
  {
    Serial.print(" ");
    Serial1.print(" ");
    delay(3);
  }
}
void showTime()
{

  if (timeinfo.tm_hour > 12)
    sprintf(buf, "%s %02d  %2d:%02d PM", monthNames[timeinfo.tm_mon], timeinfo.tm_mday, timeinfo.tm_hour - 12, timeinfo.tm_min);
  else if (timeinfo.tm_hour > 0)
    sprintf(buf, "%s %02d  %2d:%02d AM", monthNames[timeinfo.tm_mon], timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
  else
    sprintf(buf, "%s %02d  %2d:%02d AM", monthNames[timeinfo.tm_mon], timeinfo.tm_mday, 12, timeinfo.tm_min);

  Serial.println(buf);
  line2();
  red();
  sendMessage(buf);
}

void handleMessage(AdafruitIO_Data *data)
{

  Serial.print("received <- ");
  Serial.println(data->value());

  if (data->toString() == "red")
    red();
  else if (data->toString() == "orange")
    orange();
  else if (data->toString() == "green")
    green();
  else if (data->toString() == "line2")
    line2();
  else if (data->toString() == "clear")
    clear();
  else
  {
    clear();
    sendMessage(data->value());
  }
}

void readParamsFromFS()
{
  if (LittleFS.begin())
  {

    if (LittleFS.exists("/config.json"))
    {
      // file exists, reading and loading
      Serial.println("Reading config file");

      File configFile = LittleFS.open("/config.json", "r");
      if (configFile)
      {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(256);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        Serial.println();
        if (!deserializeError)
        {

          if (json.containsKey("IO_USERNAME"))
            strcpy(IO_USERNAME, json["IO_USERNAME"]);
          if (json.containsKey("IO_KEY"))
            strcpy(IO_KEY, json["IO_KEY"]);
        }
        else
        {
          Serial.println("Failed to load json config");
        }
        configFile.close();
      }
    }

    else
    {
      Serial.println("Failed to mount FS");
    }
  }
}

void resetAll()
{
  Serial.print("Reseting...");
  LittleFS.format();
  delay(1000);
  wifiManager.resetSettings();
  delay(2000);
  ESP.restart();
}

void setup()
{
  // wifiManager.resetSettings();
  // LittleFS.format();
  // delay(1000);

  pinMode(buttonPin, INPUT_PULLUP); // Set button input pin
  digitalWrite(buttonPin, HIGH);

  Serial.begin(115200);
  // wait for serial monitor to open
  while (!Serial)
    ;

  readParamsFromFS(); // get parameters from file system

  Serial1.begin(9600, SERIAL_8N1, 10, 13, true);

  clear();
  sendMessage("Starting...");
  delay(500);
  clear();
  sendMessage("Starting... 1");
  delay(500);
  clear();
  sendMessage("Starting... 2");
  delay(500);
  clear();
  sendMessage("Starting... 3");

  if (LittleFS.begin())
  {

    if (LittleFS.exists("/config.json"))
    {
      // file exists, reading and loading
      Serial.println("Reading config file");

      File configFile = LittleFS.open("/config.json", "r");
      if (configFile)
      {

        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument json(256);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        Serial.println();
        if (!deserializeError)
        {

          if (json.containsKey("IO_USERNAME"))
            strcpy(IO_USERNAME, json["IO_USERNAME"]);
          if (json.containsKey("IO_KEY"))
            strcpy(IO_KEY, json["IO_KEY"]);
          if (json.containsKey("TIMEZONE"))
            strcpy(TIMEZONE, json["TIMEZONE"]);
        }
        else
        {
          Serial.println("Failed to load json config");
        }
        configFile.close();
      }
    }
  }
  else
  {
    Serial.println("Failed to mount FS");
  }

  WiFi.mode(WIFI_STA);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setCustomHeadElement("<style>.c{text-align: center;} input,select{padding:5px;font-size:1em;-webkit-appearance:none; -ms-box-sizing:content-box; -moz-box-sizing:content-box; -webkit-box-sizing:content-box; box-sizing:content-box; } input,select{width:95%;} body{margin:0;text-align: center;font-family:verdana;background: #000000} button{margin:0;border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:normal;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==') no-repeat left center;background-size: 1em;}</style><style>.highlight{color:#d0d0d0}}body{margin:0;padding:0;align: center; background:#151515 0 0;color:#eaeaea;font-size:16px;line-height:1.5;font-family:Monaco,'Bitstream Vera Sans Mono','Lucida Console',Terminal,monospace}.container{width:95%;max-width:1000px;margin:0 auto}section{display:block;margin:0 0 20px 0}h1,h2,h3{align:center; text-align:center;},h4,h5,h6{color: #FFFFFF; margin:0 0 20px}li{line-height:1.4}header{background:rgba(0,0,0,.1);width:100%;border-bottom:1px dashed #b5e853;padding:20px 0;margin:0 0 40px 0}header h1{font-size:30px;line-height:1.5;margin:0 0 0 -40px;font-weight:700;font-family:Monaco,'Bitstream Vera Sans Mono','Lucida Console',Terminal,monospace;color:#b5e853;text-shadow:0 1px 1px rgba(0,0,0,.1),0 0 5px rgba(181,232,83,.1),0 0 10px rgba(181,232,83,.1);letter-spacing:-1px;-webkit-font-smoothing:antialiased}}header h1}header h2{font-size:18px;font-weight:300;color:#666}h1,h2,h3,h4,h5,h6{font-weight:400;font-family:Monaco,'Bitstream Vera Sans Mono','Lucida Console',Terminal,monospace;color:#b5e853;letter-spacing:-.03em;text-shadow:0 1px 1px rgba(0,0,0,.1),0 0 5px rgba(181,232,83,.1),0 0 10px rgba(181,232,83,.1)}hr{height:0;border:0;border-bottom:1px dashed #b5e853;color:#b5e853}button{display:inline-block;background:-webkit-linear-gradient(top,rgba(48,48,48,1),rgba(35,35,35,1) 50%,rgba(25,25,25,.8) 50%,rgba(21,21,21,.8));padding:8px 18px;border-radius:50px;border:2px solid rgba(0,0,0,.7);border-bottom:2px solid rgba(0,0,0,.7);border-top:2px solid #000;color:rgba(255,255,255,.8);font-family:Monaco,'Bitstream Vera Sans Mono','Lucida Console',Terminal,monospace;font-weight:700;font-size:13px;text-decoration:none;text-shadow:0 -1px 0 rgba(0,0,0,.75);box-shadow:inset 0 1px 0 rgba(255,255,255,.05)}button:hover{background:-webkit-linear-gradient(top,rgba(48,48,48,.6),rgba(35,35,35,.6) 50%,rgba(10,10,10,.8) 50%,rgba(0,0,0,.8)); border: 2px solid #b5e853;}button .icon{display:inline-block;width:16px;height:16px;margin:1px 8px 0 0;float:left} a{color:#63c0f5;text-shadow:0 0 5px rgba(104,182,255,.5)}.switch { position: relative; display: inline-block; width: 60px; height: 30px; } .switch input { opacity: 0; width: 0; height: 0; } .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; -webkit-transition: .4s; transition: .4s; } .slider:before { position: absolute; content: ''; height: 22px; width: 22px; left: 4px; bottom: 4px; background-color: white; -webkit-transition: .4s; transition: .4s; } input:checked + .slider { background-color: #b5e853; } input:focus + .slider { box-shadow: 0 0 1px #2196F3; } input:checked + .slider:before { -webkit-transform: translateX(30px); -ms-transform: translateX(30px); transform: translateX(30px); } .slider.round { border-radius: 30px; } .slider.round:before { border-radius: 50%; }</style>");
  const char *custom_TIMEZONE_dd = "<br/><label>Select a Timezone</label><br><select type='text' name='custom_TIMEZONE_id'> <option value='NST03:30NDT,M3.2.0/00:01:00,M11.1.0/00:01:00'>Newfoundland Standard/Daylight Time</option> <option value='AST4ADT,M3.2.0/01:00:00,M11.1.0/02:00:00'>Atlantic Standard/Daylight Time</option> <option value='EST5EDT,M3.2.0/01:00:00,M11.1.0/02:00:00' selected>Eastern Standard/Daylight Time</option> <option value='CST6CDT,M3.2.0/01:00:00,M11.1.0/02:00:00'>Central Standard/Daylight Time</option> <option value='CST6'>Central Standard Time without Daylight Savings</option> <option value='MST7MDT,M3.2.0/01:00:00,M11.1.0/02:00:00'>Mountain Standard/Daylight Time</option> <option value='PST8PDT,M3.2.0/01:00:00,M11.1.0/02:00:00'>Pacific Standard/Daylight Time</option></select>";

  // add all your parameters here
  wifiManager.addParameter(&custom_IO_USERNAME);
  wifiManager.addParameter(&custom_IO_KEY);
  new (&custom_TIMEZONE) WiFiManagerParameter(custom_TIMEZONE_dd); // custom html input
  wifiManager.addParameter(&custom_TIMEZONE);

  custom_IO_KEY.setValue(IO_KEY, 64);
  custom_IO_USERNAME.setValue(IO_USERNAME, 64);
  custom_TIMEZONE.setValue(TIMEZONE, 64);

  WiFiManagerParameter custom_text("<hr><h5 align='center'>Choose an available network, enter details and press \"Save\". AdafruitIO username and key used for integration with voice assistant control from Google Assistant or Alexa.</h5><hr>");
  wifiManager.addParameter(&custom_text);
  wifiManager.setTitle("Wallboard Gadget Setup");
  wifiManager.setBreakAfterConfig(true);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  std::vector<const char *> menu = {"wifi", "info", "update", "exit"};
  wifiManager.setMenu(menu);
  wifiManager.setClass("invert");

  wifiManager.setTimeout(120);
  wifiManager.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wifiManager.setAPClientCheck(true);      // avoid timeout if client connected to softap

  Serial.println("calling autoconnect");
  if (!wifiManager.autoConnect("Wallboard Gadget Setup"))
  {
    Serial.println("failed to connect and hit timeout");
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("Connected to WiFi.");
    Serial.println("The values in the file are: ");
    Serial.println("\tIO_USERNAME : " + String(IO_USERNAME));
    Serial.println("\tIO_KEY : " + String(IO_KEY));
    Serial.println("\tTIMEZONE : " + String(TIMEZONE));

    if ((strlen(IO_USERNAME) == 0) && (strlen(IO_KEY) == 0))
      resetAll();

    configTzTime(TIMEZONE, ntpServer);
    delay(3000);

    io = new (objStorage) AdafruitIO_WiFi(IO_USERNAME, IO_KEY, "", "");

    AdafruitIO_Feed *wallboard = io->feed("wallboard");

    Serial.printf("Connecting to Adafruit IO with User: %s Key: %s\n", IO_USERNAME, IO_KEY);
    io->connect();
    // wait for a connection
    int i = 0;

    while ((io->status() < AIO_CONNECTED) && (i < 200))
    {

      Serial.print(".");
      i = (i + 1);
      Serial.println(io->statusText());
      delay(500);
    }
    if (io->status() < AIO_CONNECTED)
    {
      Serial.print("Failed to connect to IO");
    }

    //
    Serial.println(io->statusText());

    wallboard->onMessage(handleMessage);

    /*
    ArduinoOTA.setHostname("wallboard");
    ArduinoOTA.setPassword("admin");
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
        } else { // U_FS
          type = "filesystem";
        }

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("Start updating " + type);
      });
      ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
      });
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      });
      ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
          Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
          Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
          Serial.println("End Failed");
        }
      });
      ArduinoOTA.begin();
      Serial.println("OTA Ready");
    */

    // wallboard->get();

    // we are connected
    Serial.println();
    Serial.println(io->statusText());
    clear();
    sendMessage("Ready...");

    if (MDNS.begin("wallboard"))
    {
      Serial.println("MDNS responder started");
    }

    if (!myGetLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
    }
    else
    {

      Serial.printf("Time: %i : %i : %i\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
    if (timeinfo.tm_hour == 0)
    {
      clock_hour = 12;
    }
    else
      clock_hour = timeinfo.tm_hour < 13 ? timeinfo.tm_hour : timeinfo.tm_hour - 12;
  }
  delay(10000);
  showTime();

  showWeather();
}

void loop()
{
  // ArduinoOTA.handle();
  io->run();

  if (digitalRead(buttonPin) == LOW)
  {
    delay(3000);
    if (digitalRead(buttonPin) == LOW)
    {
      Serial.println("Reseting ...");
      clear();
      line1();
      red();
      sendMessage("Reseting...");
      resetAll();
      delay(20);
    }
  }

  EVERY_N_HOURS(1)
  {
    configTzTime(TIMEZONE, ntpServer);
    delay(1000);
    Serial.println("Updating time from NTP");
    if (!myGetLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
    }
  }
  EVERY_N_MINUTES(10)
  {
    myGetLocalTime(&timeinfo);
    if (timeinfo.tm_sec == 0)
    {
      if (timeinfo.tm_hour == 0)
      {
        clock_hour = 12;
      }
      else
      {
        clock_hour = timeinfo.tm_hour < 13 ? timeinfo.tm_hour : timeinfo.tm_hour - 12;
      }
      showTime();
    }
  }
}
