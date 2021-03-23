
// NANO 33 IOT Web client talking to AirNow.gov to get AQI data.
// ============================================================
/*
  This sketch connects to a website using the Nano 33 IoT WiFi.
  Original example was written for a network using WPA encryption.
  For WEP or WPA, change the Wifi.begin() call accordingly.
  Needs Arduino Circuit Board with NINA WiFi modules supported:
  Arduino MKR WiFi 1010, MKR VIDOR 4000, UNO WiFi, NANO 33 IoT.
  ----------
  Based on old Ethernet code (deprecated) in favor of Nano WiFi:
  https://www.arduino.cc/en/Tutorial/LibraryExamples/ConnectNoEncryption
  13 July 2010 Created by dlf (Metodo2 srl).
  31 May 2012 Modified by Tom Igoe.
  ----------
  Inspired by code from Limor Fried/Ladyada @ Adafruit Industries.
  See her company text credits below.
  See Anne Barela's 2015 post in "The 21st Century Digital Home":
  https://21stdigitalhome.blogspot.com/2015/03/arduino-day-2015-project-air-quality.html
  See also: https://www.arduino.cc/en/Tutorial/LibraryExamples/WiFiNINAWiFiWebClient
  ----------
  28 Oct 2020 Heavily modified by Michael Stoddard @ BaxRoad.com
  converted from Ethernet to NANO 33 IoT WiFi, added LCD display.
*/
  // 2020-1027 Initial program inception.
  // 2020-1102 Display SSID name on LCD display.
  // 2020-1120 Working; doing end-to-end testing.
  // 2020-1204 Final LCD screen layout.
  // 2020-1207 Embellished LCD display code, added WiFi "." dots.
  // 2020-1208 Add TEST Zips to Profile 1 for the 6 AQI colors.
  // 2020-1211 Add 2nd AQI profile, ZipCodeRoller restart.
  // 2020-1214 Display AQI 0 as green, not white. Add El Centro CA.
  // 2020-1216 Add WS2812B library and code. Doesn't compile ..
  // 2020-1217 Implement Right Arrow to cycle between profiles.
  // 2020-1218 Display AQI last update Hour and Timezone.
  // 2020-1220 WS2812B LEDs work after new <FastLED.h> was fixed
  // 2020-1221 Reset FastLED LEDs on startup until data is acquired.
  // 2020-1222 Synchronize countdown blinking for LED and D13.
  // 2020-1223 Play with frozen/resume zip code rolling; TBF.
  // 2020-1224 Set refresh to 2 or 30 min, WS2812B LEDs to 4.
  // 2020-1225 Add indicator for locked/unlocked zipcode roller.
  // 2020-1226 Final debug code added, display microcode level.
  // 2020-1229 Testing Blynk App Zip Code entry.
  // 2021-0110 Reset timer if no AirNow msgs. after 10 seconds.
  // 2021-0110 "OK" button refreshes the current Zip Code data.
  // 2021-0123 Change Santa Barbara ZipCode from 93110 to 93101.
  // 2021-0123 Update secrets.h.
  // 2021-0203 Runtime warning: "function decode(&results) 
  //           deprecated" but not fixed by using decode();
  //           using back-level IRremote.h version 2.8.0 for now.
  // 2021-0205 Show server name connected to on serial monitor.
  // 2021-0208 Show credentials on serial monitor log.
  // 2021-0209 Show city name every 10 seconds during countdown.
  // 2021-0209 Add Montrose and Ladera Ranch zips to profile #2.
  // 2021-0210 Set refresh time from 30 minutes to 15 minutes.
  // 2021-0212 Set long time interval if profile #4 selected.
  // 2021-0213 Code cleanup, add zipcode 99705 to profile #1.
  //
  // Distributed under royalty-free MIT Creative Commons license;
  // License and credits text must be included in any redistribution.

/***************************************************
  Air Quality Monitoring
  Uses the Arduino Uno, Ethernet Shield, and Adafruit 1.8"
  display shield http://www.adafruit.com/products/802

  Check out the links above for our tutorials and wiring diagrams

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Based on code written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

char Version[] = "Version 02/13/21";
// Compiled with Arduino IDE 1.8.13

#include <SPI.h>
#include <WiFiNINA.h>
#include <IRremote.h>
#include <FastLED.h>

WiFiClient client;
#define LCDcode
#define LongDelay 900
#define ShortDelay 120
#define Profile1ZipCount 7
#define Profile2ZipCount 22

#define NUM_LEDS 4
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
#define LED_PIN 4

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Note: TFT HX8304B display no longer available, so this 
// code uses the Arduino 16X2 LCD display instead.
// Therefore, the original TFT code is commented out.
// #include <Adafruit_GFX.h> // Core Adafruit graphics library
// #include <Adafruit_HX8340B.h> // TFT display code

// RGB HEX Display Colors:
// ====================
#define WHITE  0xFFFFFF // =0 No Data
#define GREEN  0x008000
#define YELLOW 0xFFFF00
#define ORANGE 0xFFA500
#define RED    0xFF0000
#define PURPLE 0x800080
#define MAROON 0x800000
#define BLACK  0x000000 // >501 Error
#define BLUE   0x0000FF

//bool Debug; // now initialized in secrets.h
#include "arduino_secrets.h";
//char ssid[] == will be your network SSID (network name)
//char pass[] == will be your network password
//char apikey[] == will be your AirNow AQI access key
int keyIndex = 0; // your WEP network key Index number

String LCDcolor = "    ";
int TimeRemaining = 0;
uint8_t lineCount = 0;
char SixteenSpaces[] = "                ";
char ElevenSpaces[] = "           ";
int AqiValue = 0;
int CurrentAQIcolor = 0;
String CurrentLCDcolor = "    ";
char AqiQuality[] = "                                                ";
char AqiType[] = "     ";
String AqiColor = "   ";
String OZONE = "OZONE";
String BlynkZipCode = "95701"; // Baxter, CA (actually Alta, CA)
String location = "";
char None[] = "None ";
char NoConnection[] = "No Connection";
char UnknownQ[] = "Unknown";
int AQIcolorCode = 0;
int NextRefresh;
bool ServerDisconnected = false;
int HomeZipLocation = 1;
int CurrentZipLocation = 0;
int NumberOfZipCodes = 1;
int RetryCount = 0;
int ZipCodeProfile = 0;
int NumberOfProfiles = 4;
int TimeDelay = 3000; // 3 seconds
unsigned long last = millis(); // timestamp for last IR message

bool ZipCodeRoller;
bool ProfileRollFlag;

// List of AQI Pollutants:
// ===============================
// PM2.5 PM10 O3 NO2 CO SO2 NH3 Pb
// Note: Only PM2.5, PM10, O3, NO2, and CO reported by AirNow
 
int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128); // numeric IP for Google (no DNS)
//char server[] = "www.google.com"; // name address for Google (using DNS)
char server[] = "www.airnowapi.org";// name address for Airnow.gov DNS)

int RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
decode_results results;
long int IRvalue = 0;
long rssi = 0;
const uint8_t header[2] = { 0x01, 0x02 };

// TFT display will NOT use hardware SPI due to the SPI
// of the Ethernet shield, so other pins are used.
// This makes the display a bit slow ...
#define SD_CS     4   // Chip select line for SD card
#define TFT_CS    9   // Chip select line for TFT display
#define TFT_SCLK  6   // set these to be whatever pins you like!
#define TFT_MOSI  7   // set these to be whatever pins you like!
#define TFT_RST   8   // Reset line for TFT (0 = reset on Arduino reset)
// Adafruit_HX8340B tft(TFT_MOSI, TFT_SCLK, TFT_RST, TFT_CS);

void setup() {
     
#ifdef LCDcode
  lcd.begin();     // Initialize LCD display
  lcd.backlight(); // Power ON the LCD backlight
  lcd.clear();     // Clear out the buffer
#endif

  pinMode(LED_BUILTIN, OUTPUT); // required for NANO IoT 33
  digitalWrite(LED_BUILTIN, LOW); // reset programmer LED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  for (int ii=0; ii<NUM_LEDS; ii++) {
    leds[ii] = CRGB::Black;
    }
  FastLED.show();
    
  Serial.begin(9600); // Initialize USB serial port
  delay(500);
  // while (!Serial) {
  // wait for serial port to connect. 
  // Needed for native USB port only
  // }
  Serial1.begin(9600); // Initialize serial port 1
  delay(500);

  Serial.println();
  Serial.print(F("BaxRoad NANO 33 IoT  \"DAQIRI\"  "));
  Serial.println(Version);
  Serial.println(F("Digital Air Quailty Index Real-time Interrogator"));

#ifdef LCDcode  
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("BaxRoad \"DAQIRI\"");
  lcd.setCursor(0,1); lcd.print(Version);
  delay(TimeDelay);
#endif
  
  ZipCodeProfile   = 1; // 1st ZipCode List
  if (Debug == true) {
    ZipCodeProfile = 2; // 2nd ZipCode List
  }
  //ZipCodeProfile = 3; // AQI Test colors
  //ZipCodeProfile = 4; // Blynk ZipCode
  CurrentZipLocation = HomeZipLocation; // starting test location in the profile

  Serial.print("Debug is ");
  if (Debug) {
    Serial.println("enabled.");
    NextRefresh = ShortDelay; // 2 minutes
    } else {
    Serial.println("disabled.");
    NextRefresh = LongDelay; // 15 minutes
    }
  Serial.print("AQI refresh is set to ");
  Serial.print(NextRefresh);
  Serial.println(" seconds.");
      
  Serial.print("Location profile ");
  Serial.print(ZipCodeProfile);
  Serial.print(" -- ");
  switch (ZipCodeProfile) {
    case 1: {
      Serial.println("1st List");
      NumberOfZipCodes = Profile1ZipCount;
      break;
      }
   case 2: {
      Serial.println("2nd List");
      NumberOfZipCodes = Profile2ZipCount;
      break;
      }
   case 3: {
      Serial.println("AQI Test Colors");
      NumberOfZipCodes = 6;
      break;
      }
   case 4: {
      Serial.println("Blynk ZipCode");
      NumberOfZipCodes = 1;
      break;
      }   
    }
  Serial.println();
  ZipCodeRoller = false;
  ProfileRollFlag = true;
    
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true); // die here; don't continue
    }
  String fv = WiFi.firmwareVersion();
  Serial.print("WiFi firmware version ");
  Serial.print(fv);
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print(" is back-level; latest is ");
    Serial.print(WIFI_FIRMWARE_LATEST_VERSION);
    //Serial.print("please upgrade it");
    }
  Serial.println();
    
  irrecv.enableIRIn(); // Start the IR receiver
 
#ifdef LCDcode
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(SixteenSpaces); 
  lcd.setCursor(0,0); lcd.print(ssid);
  lcd.setCursor(0,1); lcd.print(SixteenSpaces);
  lcd.setCursor(0,1);
  RetryCount = 12;
#endif

  Serial.print("AQI Key: ");
  Serial.println(aqikey);
  
  Serial.print("Connecting to SSID: ");
  Serial.print(ssid);
  Serial.print("  ");
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print(".");
    
  #ifdef LCDcode
    RetryCount--;
    if (RetryCount > 0)
      lcd.print("."); // show progress dots;
  #endif
  
    // Connect to WPA/WPA2 network. 
    // Change this method if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait for the connection
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200); // leave ON for .2 second
    digitalWrite(LED_BUILTIN, LOW);
    delay(300); // turn OFF for .3 second
    }
    
  Serial.println();
  Serial.print("Connected to ");
  printWifiStatus();
  
#ifdef LCDcode
  //lcd.setCursor(0,1);
  //lcd.print(SixteenSpaces);
  lcd.setCursor(0,1);
  lcd.print("Connect! ");
  lcd.print(rssi);
  lcd.println("dBm ");
  delay(TimeDelay);
#endif

  }

void loop() {
  char buffer[256]; 
  // HTTP read buffer; the max line returned by the AirNow API 
  // is 158 chars. Increased from 160 to 256 incase any overflow.
  
  char partInfo[4][36]; // The air quality values are placed in 4
  // strings of length 10 chars.
  //Note: for Air Quality Description --
  //  Previous quality used to be up to 10 characters (example: "Unhealthy";
  //  now it can be more (example: "Unhealthy for Sensitive Groups")
  
  uint8_t tokenCount, valueCount;
  char *bufValue;
  char *bufPtr;
  char Time1[6];
  char Zone1[6];
  char CurrentTime1[6];
  char CurrentZone1[6];

  while (true) {
  ServerDisconnected = false;

  if (TimeRemaining <= 0) {
    TimeRemaining = NextRefresh+2; // re-init 
    lineCount = 0; // start fresh
 
  #ifdef LCDcode  
    lcd.clear();
    lcd.setCursor(0,0); 
    lcd.print("HTTP Connect SSL");
    lcd.setCursor(0,1); 
    lcd.print(SixteenSpaces);
    lcd.setCursor(0,1); lcd.print("IP ");
    IPAddress ip = WiFi.localIP();
    lcd.setCursor(3,1); lcd.print(ip);
  #endif
  
    Serial.println();
    Serial.println("Starting connection to remote server...");
    // if you get a connection, report back via serial:
    if (client.connectSSL(server, 443)) {
      Serial.print("You are now connected to server ");
      Serial.println(server);
      
    #ifdef LCDcode
      lcd.clear();
      lcd.setCursor(0,0); 
      lcd.print("Read AirNow Data");
      lcd.setCursor(0,1);
    #endif

      if (ZipCodeRoller) {
        CurrentZipLocation++; // Roll to location for next pass
        if (CurrentZipLocation > NumberOfZipCodes)
          CurrentZipLocation = 1; // restart
          }
        ZipCodeRoller = ProfileRollFlag; // set false to not roll the codes

        if (ZipCodeProfile == 1) { // "1st List"
        switch (CurrentZipLocation) {
          case 1: {
            zipcode = "93101";
            location = "Santa Barbara CA";
            break;
            }
          case 2: {
            zipcode = "93117";
            location = "Goleta, CA";
            break;
            }
          case 3: {
            zipcode = "92821";
            location = "Brea, CA";
            break;
            }
          case 4: {
            zipcode = "93436";
            location = "Lompoc, CA";
            break;
            }
          case 5: {
            zipcode = "93529";
            location = "June Lake, CA";
            break;
            }
          case 6: {
            zipcode = "93013";
            location = "Carpinteria, CA";
            break;
            }
          case 7: {
            zipcode = "99705";
            location = "North Pole, AK";
            break;
            }
          default: {
            zipcode = "93108";
            location = "Montecito, CA";
            break;
            }
          }
        } // end if (ZipCodeProfile == 1)

      if (ZipCodeProfile == 2) { // "2nd List"
      switch (CurrentZipLocation) {
        case 1: {
          zipcode = "92821";
          location = "Brea, CA";
          break;
          }
        case 2: {
          zipcode = "93101";
          location = "Santa Barbara CA";
          break;
          }
        case 3: {
          zipcode = "99705";
          location = "North Pole, AK";
          break;
          }
        case 4: {
          zipcode = "92243";
          location = "El Centro, CA";
          break;
          }
        case 5: {
          zipcode = "06601";
          location = "Bridgeport, CT";
          break;
          }
        case 6: {
          zipcode = "83861";
          location = "St. Maries, ID";
          break;
          }
        case 7: {
          zipcode = "93301";
          location = "Bakersfield, CA";
          break;
          }
        case 8: {
          zipcode = "20781";
          location = "Hyattsville, MD";
          break;
          }
        case 9: {
          zipcode = "99701";
          location = "Fairbanks, AK";
          break;
          }
        case 10: {
          zipcode = "95201"; 
          location = "Stockton, CA";
          break;
          }
        case 11: {
          zipcode = "96801";
          location = "Honolulu, HI";
          break;
          }
        case 12: {
          zipcode = "80219";
          location = "Denver, CO";
          break;
          }
        case 13: {
          zipcode = "87420";
          location = "Shiprock, NM";
          break;
          }
        case 14: {
          zipcode = "90740";
          location = "Seal Beach, CA";
          break;
          }
        case 15: {
          zipcode = "93041";
          location = "Oxnard, CA";
          break;
          }
        case 16: { 
          zipcode = "93440";
          location = "Lompoc, CA";
          break;
          }
        case 17: {
          zipcode = "95630"; 
          location = "Folsom, CA";
          break;
          }
        case 18: {
          zipcode = "58107";
          location = "Fargo, ND";
          break; 
          }
        case 19: {
          zipcode = "92259"; 
          location = "Ocotillo, CA";
          break;
          }
        case 20: {
          zipcode = "92309";
          location = "Zzyzx, CA";
          break; 
          }
        case 21: {
          zipcode = "91020"; 
          location = "Montrose, CA";
          break;
          }
        case 22: {
          zipcode = "92694";
          location = "Ladera Ranch, CA";
          break; 
          }
        default: {
          zipcode = "90009";
          location = "Los Angeles, CA";
          break;
          }
        }
      } // end if (ZipCodeProfile == 2)
         
    if (ZipCodeProfile == 3) { // "AQI Test Colors"
    switch (CurrentZipLocation) {
      case 1: {
        zipcode = "90001";
        location = "Green TEST";
        break;
        }
      case 2: {
        zipcode = "90002";
        location = "Yellow TEST";
        break;
        }
      case 3: { 
        zipcode = "90003";
        location = "Orange TEST";
        break;
        }
      case 4: {
        zipcode = "90004";
        location = "Red TEST";
        break; 
        }
      case 5: {
        zipcode = "90005";
        location = "Purple TEST";
        break;
        }
      case 6: {
        zipcode = "90006";
        location = "Maroon TEST";
        break;
        }
      default: {
        zipcode = "90007";
        location = "Code Error";
        break;
        }
      } // end if (ZipCodeProfile == 3)
    }

    if (ZipCodeProfile == 4) { // "Blynk ZipCode"
    switch (CurrentZipLocation) {
      case 1: {
        zipcode = BlynkZipCode;
        location = "Blynk Zipcode";
        break;
        }
      }
    } // end if (ZipCodeProfile == 4)
        
  #ifdef LCDcode
  String ForString = "In ZipCode ";
  String zipcodeString = ForString + zipcode;
  lcd.print(zipcodeString);
  delay(TimeDelay);
  #endif

  AqiValue = -1;
  strcpy(AqiQuality, UnknownQ);
  strncpy(AqiType, None, 5);
  if (ZipCodeProfile != 3) {
    CurrentAQIcolor = 8; // Blue light  - Acquiring AQI data
    } else {
    CurrentAQIcolor = 7; // Black out -  for AQI test mode  
    }
  CurrentLCDcolor = "WHI";
    
  Serial.print("Query Zipcode Profile ");
  Serial.print(ZipCodeProfile);
  Serial.print(" location ");
  Serial.print(CurrentZipLocation);
  Serial.print(" -- ");
  Serial.print(location);
  Serial.print("  ");
  Serial.println(zipcode);
       
  // Make an HTTP request to AirNow.gov:
  //   Old Latitude/Longitude format:
  //   client.println("GET /aq/forecast/latLong/current/?format=text/csv&latitude=38.8&longitude=-77.3&distance=25&API_KEY=46733A42-8902-45BF-A231-XXXXXX HTTP/1.1");
  //   New Zipcode format:
  
  String QueryString = "GET /aq/observation/zipCode/current/?format=text/csv&zipCode=" + zipcode + "&distance=25&API_KEY=" + aqikey + " HTTP/1.1";
  Serial.println(QueryString);
  client.println(QueryString);
  Serial.println("AirNowAPI Request sent"); 
  client.println("Host: www.airnowapi.org");
  Serial.print("Connection: closed. ");
  Serial.println("Responses (if any) follow ...");
  client.println("Connection: close");
  client.println();
  } // end if (client.connectSSL(server, 443))
  else
  {
    client.stop(); // per https://github.com/esp8266/Arduino/issues/72
    if (Debug)
      Serial.println("$Debug -- 'client.stop' #1 sent.");
    AqiValue = -1; //
    strcpy(AqiQuality, NoConnection);
    strncpy(AqiType, None, 5);
    CurrentAQIcolor = 7; // black;
    CurrentLCDcolor = "BLK";
    }

  #ifdef LCDcode
  lcd.setCursor(0,0); lcd.print(SixteenSpaces);
  lcd.setCursor(11,0); lcd.print(zipcode);
  #endif
    
  if (WiFi.RSSI() == 0) { // no WiFi connection
    
  #ifdef LCDcode
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(SixteenSpaces); 
    lcd.setCursor(0,0); lcd.print(ssid);
    lcd.setCursor(0,1); lcd.print(SixteenSpaces);
    lcd.setCursor(0,1);
    RetryCount = 12;
  #endif
      
    while (status != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(ssid);

      #ifdef LCDcode
      RetryCount--;
      if (RetryCount > 0)
        lcd.print("."); // show progress dots ...
      #endif
       
     // Connect to WPA/WPA2 network. 
     // Change this method if using open or WEP network:
     status = WiFi.begin(ssid, pass);
     // wait for the connection:
     delay(500);
     }
   if (Debug)
     Serial.println("$Debug -- client.connectSSL() sent.");
   status = client.connectSSL(server, 443);
   delay(500);
   }
  
 #ifdef LCDcode
   lcd.clear();
   //lcd.setCursor(0,0); 
   //lcd.print(SixteenSpaces);
   //lcd.setCursor(0,1); 
   //lcd.print(ElevenSpaces);
 #endif
    
   while (true) {
     if (client.available()) {
       byte numChar = client.readBytesUntil((char)0x0a,buffer,159);  // Read until a line feed character
        buffer[numChar]='\0';
        lineCount++;
        if (lineCount == 7) { // could be Date/Time string ...
          if ((buffer[0]=='D') && (buffer[1]=='a')) { // is it?
          
          #ifdef LCDcode
            lcd.clear(); // yes, proceed
            lcd.setCursor(0,0); lcd.print("Date ");
            for (int ii=5; ii<17; ii++) {
              lcd.setCursor(ii,0); lcd.print(buffer[6+ii]);
              }
            lcd.setCursor(0,1); 
            lcd.print("P");
            lcd.print(ZipCodeProfile); // profile
            if (CurrentZipLocation < 10) {
              lcd.print(CurrentZipLocation); // zip loc 0-9
              }
            else
              {
                char curzip = char(CurrentZipLocation - 10 + 'A');
                lcd.print(curzip); // zip loc 10+
                //Serial.print("$Debug curzip char = ");
                //Serial.println(curzip);
              }
            lcd.setCursor(4,1); //example "00:00:00 GMT"
            for (int ii=4; ii<16; ii++) {
              lcd.setCursor(ii,1); lcd.print(buffer[19+ii]);
              }
            //digitalWrite(LED_BUILTIN, HIGH);
            //delay(1000);
            /*
            lcd.setCursor(6,1); lcd.print(" ");
            lcd.setCursor(9,1); lcd.print(" ");
            digitalWrite(LED_BUILTIN, LOW);
            delay(1000);
            lcd.setCursor(6,1); lcd.print(":");
            lcd.setCursor(9,1); lcd.print(":");
            digitalWrite(LED_BUILTIN, HIGH);
            delay(1000);
            lcd.setCursor(6,1); lcd.print(" ");
            lcd.setCursor(9,1); lcd.print(" ");
            */
            //digitalWrite(LED_BUILTIN, LOW);
            //delay(1000);
            delay(TimeDelay);
          #endif
          
            } // end if ((buffer[0]=='D') && (buffer[1]=='a')) { // is it?
          } // end if (lineCount == 7) { // could be Date/Time string ...
    
        if ((lineCount == 8) || (lineCount == 9)) { // connection closed
        
        #ifdef LCDcode
          lcd.setCursor(0,1); lcd.print(SixteenSpaces);
        #endif
        
          }
        if (lineCount == 10) { // AQI pollutant record legend; ignore
        
        #ifdef LCDcode
        #endif
        
          }
        // Look for 6 AQI reading strings 1 each for pollutants measured
        if ((lineCount >= 11) && (lineCount <= 16)) { // could be a data string ...
          if ((buffer[0]=='"') && (buffer[1]=='2')) { // is it?
            Serial.print(lineCount); Serial.print(":");
            Serial.print("> "); // yes, proceed
            Serial.println(buffer);
            tokenCount=0;
            valueCount=0;
            bufPtr = strtok(buffer,",");
            while(bufPtr != NULL && valueCount < 31) { // was 11; updated for WiFi version
              
              if(valueCount == 1) { // Last Update Time
               strcpy(Time1, bufPtr+1); 
              }
              if(valueCount == 2) {// Last Update Zone
               strcpy(Zone1, bufPtr+1); 
              }
              
              if(valueCount > 6) {
                strcpy(partInfo[tokenCount], bufPtr+1);
                tokenCount++;
                }
              bufPtr = strtok(NULL,",");
              valueCount++;
              }

              for( uint8_t i=0; i<6; i++) {
              if(Time1[i] == '"') { // the second quote is the end of the string we want
                  Time1[i] = '\0'; //   so replace it with the C null end of string character
                  }
              if(Zone1[i] == '"') { // the second quote is the end of the string we want
                  Zone1[i] = '\0'; //   so replace it with the C null end of string character
                  }
                }
                
              //Serial.print("$Debug -- Time1 ");
              //Serial.print(Time1);
              //Serial.print(" -- Zone1 ");
              //Serial.println(Zone1);
              
            for( uint8_t i=0; i<4; i++) {
              for( uint8_t j=0; j<31; j++) { // was 10; updated for WiFi version
                if(partInfo[i][j] == '"') { // the second quote is the end of the string we want
                  partInfo[i][j] = '\0'; //   so replace it with the C null end of string character
                  }
                }
              if (Debug == true) {
                Serial.print("--> $Debug; Data[");
                Serial.print(i);
                Serial.print("]: ");
                Serial.println(partInfo[i]);
                }
              }
            Serial.println("-----------------------------");

            /* process the values */
            uint32_t colorAQI = AQI2hex(atoi(partInfo[1]));

            // TESTing bugfix for AQI 0
            if ((partInfo[1]-'0') == 0) {
              colorAQI = GREEN; // Force it
              strncpy(partInfo[1], "0", 2);
              strncpy(partInfo[2], "1", 2);
              strncpy(partInfo[3], "Good", 5);
              } 
            // end TESTing bugfix for AQI 0

            if (ZipCodeProfile == 3) { // AQI Test Colors?
            switch (CurrentZipLocation) { // find which TEST site
              case 1: {
                colorAQI = GREEN; // Test
                strncpy(partInfo[3], "Good", 5);
                strncpy(partInfo[1], "25", 3);
                break;
                }
              case 2: {
                colorAQI = YELLOW; // Test
                strncpy(partInfo[3], "Moderate", 9);
                strncpy(partInfo[1], "75", 3);
                break;
                }
              case 3: {
                colorAQI = ORANGE; // Test
                strncpy(partInfo[3], "Unhealhy for Sensitive Groups", 30);
                strncpy(partInfo[1], "125", 4);
                break;
                }
              case 4: {
                colorAQI = RED; // Test
                strncpy(partInfo[3], "Unhealhy", 9);
                strncpy(partInfo[1], "175", 4);
                break;
                }
              case 5: {
                colorAQI = PURPLE; // Test
                strncpy(partInfo[3], "VUnhealthy", 11);
                strncpy(partInfo[1], "250", 4);
                break;
                }
              case 6: {
                colorAQI = MAROON; // Test
                strncpy(partInfo[3], "Hazardous", 10);
                strncpy(partInfo[1], "350", 4);
                break;
                }
                default: {
                break;
                }
              }
            } // end (if (ZipCodeProfile == 3) {
            
            //drawtext("Type:", WHITE, 3, 20, 120);
            Serial.print("Type: ");
            //drawtext(partInfo[0], WHITE, 3, 110, 120); // Particulate
            if (strncmp(partInfo[0], "O3", 2) == 0) {
              strncpy(partInfo[0], "OZONE", 5); // spell it out 
              }
            Serial.println(partInfo[0]);

            //drawtext(partInfo[1], WHITE, 3, 85, 80); // AQI value
            Serial.print("Value: ");
            Serial.println(partInfo[1]);

            //tft.fillScreen(colorAQI);
            Serial.print("Color: ");
            //Serial.println(colorAQI,HEX);
            
            AQIcolorCode = -1; // no data
            if (colorAQI == WHITE) {
              Serial.println("WHITE");
              AQIcolorCode = 0;
              LCDcolor = "WHI";
              }
            if (colorAQI == GREEN) {
              Serial.println("GREEN");
              AQIcolorCode = 1;
              LCDcolor = "GRN";
              }
            if (colorAQI == YELLOW) {
              Serial.println("YELLOW");
              AQIcolorCode = 2;
              LCDcolor = "YEL";
              }
            if (colorAQI == ORANGE) {
              Serial.println("ORANGE");
              AQIcolorCode = 3;
              LCDcolor = "ORA";
              }
            if (colorAQI == RED) {
              Serial.println("RED");
              AQIcolorCode = 4;
              LCDcolor = "RED";
              }
            if (colorAQI == PURPLE) {
              Serial.println("PURPLE");
              AQIcolorCode = 5;
              LCDcolor = "PUR";
              }
            if (colorAQI == MAROON) {
              Serial.println("MAROON");
              AQIcolorCode = 6;
              LCDcolor = "MRN";
              }
            if (colorAQI == BLACK) {
              Serial.println("BLACK");
              AQIcolorCode = 7;
              LCDcolor = "BLK";
              }

            //drawtext(partInfo[3], WHITE, 3, 40, 40); // Level
            //drawtext("Air Quality Today", BLACK, 2, 10, 10);
            Serial.print("Air Quality: ");
            Serial.println(partInfo[3]);
            
            Serial.println("-----------------------------");
            digitalWrite(LED_BUILTIN, LOW); // reset
          
            // Individlal LCD Line1 example: "AQ ### CLR ZIPCO";
          
          #ifdef LCDcode
            lcd.clear();
            //lcd.setCursor(0,0); 
            //lcd.print(SixteenSpaces);
            lcd.setCursor(0,0);
            if (ZipCodeRoller != false) { // unlocked ?
              lcd.print("AQ "); // yes, show upper-case
            } else {
              lcd.print("aq "); // no, show lower-case
            }
          int pInfo = atoi(partInfo[1]);

          /*
          if (Debug) {
            Serial.print("$Debug -- pInfo[]: ");
            for (int ii=0; ii<4; ii++) {
              Serial.print(partInfo[ii]);
              Serial.print(" ");
              }
            Serial.println();
            }
            */
            
            while (true) {
              if (pInfo >= 100){
                lcd.setCursor(3,0); lcd.print(pInfo);
                break;
                }
              if (pInfo >= 10) {
                lcd.setCursor(3,0); lcd.print("0"); // leading zero
                lcd.setCursor(4,0); lcd.print(pInfo);
                break;
                }
              if (pInfo >= 0) {
                lcd.setCursor(3,0); lcd.print("00"); // leading zeros
                lcd.setCursor(5,0); lcd.print(pInfo);
                break;  
                }
                
              Serial.println("?? Bad Exit ??");
              break;
              }
              
            lcd.setCursor(6,0);
            lcd.print(" "); lcd.print(LCDcolor);
            lcd.setCursor(10,0); 
            lcd.print(" "); lcd.print(zipcode);  // *** zip
          #endif
    
            //Individual LCD Line2 example: "PType Qual MM:SS";
          
          #ifdef LCDcode
            lcd.setCursor(0,1); lcd.print(ElevenSpaces);
            lcd.setCursor(0,1); lcd.print(partInfo[0]);
            lcd.setCursor(5,1); lcd.print(" ");
            lcd.setCursor(6,1);
            lcd.print(partInfo[3][0]);
              lcd.print(partInfo[3][1]);
                lcd.print(partInfo[3][2]);
                  lcd.print(partInfo[3][3]);
                    lcd.setCursor(10,1); lcd.print(" ");
            delay(500);
          #endif
        
            if (atoi(partInfo[1]) > AqiValue) { // higest particulate?
              AqiValue = atoi(partInfo[1]); // yes, remember it
              strcpy(AqiQuality, partInfo[3]);
              strcpy(AqiType, partInfo[0]);
              CurrentAQIcolor = AQIcolorCode;
              CurrentLCDcolor = LCDcolor;
              strcpy(CurrentTime1, Time1);
              strcpy(CurrentZone1, Zone1);
              }
            } // if ((buffer[0]=='"') && (buffer[1]=='2'))
         } // if ((lineCount >= 11) && (lineCount <= 15))
       else {
          Serial.print(lineCount);
          Serial.print(": ");
          Serial.println(buffer);
          }
        } // end if (client.available()) 
      else // IR processing
        {
        //if (Debug) {
        //  Serial.println();
        //  Serial.println("IR processing");
        //}
        // Code moved here from IR processing loop
        if (irrecv.decode(&results)) { //Note - &results is deprecated
        //if (irrecv.decode()) { //IR event seen ... but fix doesn't work
          if (millis() - last > 250) { // and 1/4 sec since last one?
            IRvalue = results.value; // yes
              if (Debug) {
                Serial.println();
                Serial.print("IR data received: ");
                Serial.print(IRvalue, HEX);  // IR data
                Serial.println(); // CRLF
                }
              bool validIRkey = false; //
              while (true) {
                if ((IRvalue == 0xFFA25D)) {
                  validIRkey = true; break; // "1"
                  }
             
                if ((IRvalue == 0xFF629D)) {
                  validIRkey = true; break; // "2"
                  }
             
                if ((IRvalue == 0xFFE21D)) {
                  validIRkey = true; break; // "3"
                  }
             
                if ((IRvalue == 0xFF22DD)) {
                  validIRkey = true; break; // "4"
                  }
             
                if ((IRvalue == 0xFF02FD)) {
                  validIRkey = true; break; // "5"
                  }
             
                if ((IRvalue == 0xFFC23D)) {
                  validIRkey = true; break; // "6"
                  }
             
                if ((IRvalue == 0xFFE01F)) {
                  validIRkey = true; break; // "7"
                  }
             
                if ((IRvalue == 0xFFA857)) {
                  validIRkey = true; break; // "8"
                  }
             
                if ((IRvalue == 0xFF906F)) {
                  validIRkey = true; break; // "9"
                  }
             
                if ((IRvalue == 0xFF6897)) {
                  if (Debug) 
                    Serial.println("AQI IR request received ...");
                  validIRkey = true; break; // "*" (new AQI)
                  }
             
                if ((IRvalue == 0xFF9867)) {
                  validIRkey = true; break; // "0"
                  }
             
                if ((IRvalue == 0xFFB04F)) {
                  validIRkey = true; break; // "#" (Temperature)
                  }
             
                if ((IRvalue == 0xFF18E7)) { // "UP"
                  ZipCodeRoller = true;
                  ProfileRollFlag = true;
                  TimeRemaining = 0; // will advance to next zipcode
                  // code below needs to be fully tested
                  CurrentAQIcolor = 7; // BLACK
                  Serial1.print(header[0],HEX); // x01 "SOH"
                  Serial1.print(header[1],HEX); // x02 "STX"
                  Serial.print("[");
                  Serial.print(header[0],HEX); // x01 "SOH"
                  Serial.print(header[1],HEX); // x02 "STX"
                  Serial.print(IRvalue, HEX); // "UP" arrow
                  Serial1.print(IRvalue, HEX); // "UP" arrow
                  Serial.print("][");
                  Serial.print(CurrentAQIcolor); // AQI color
                  Serial1.print(CurrentAQIcolor); // AQI color
                  Serial.print("] ");
                  Serial1.println();
                  Serial.println("AQI update sent to UNO");
                  // end code above comment   
                  validIRkey = true; break; // and pass it to the Uno
                  }
             
                if ((IRvalue == 0xff4AB5)) {  // "DOWN"
                  if (ProfileRollFlag == true) {
                    ZipCodeRoller = false;
                    ProfileRollFlag = false;
                    if (Debug) {
                      Serial.print("Zip Code rolling frozen for profile ");
                      Serial.print(ZipCodeProfile);
                      Serial.print(" ZipCode "); 
                      Serial.println(zipcode);
                    }
                  }
                else
                  {
                    ZipCodeRoller = true;
                    ProfileRollFlag = true;
                    if (Debug) {
                      Serial.println("Zip Code rolling resumed");
                    }
                  }
                validIRkey = true; break;
                }
             
             if ((IRvalue == 0xFF10EF)) { // "LEFT"
             ZipCodeRoller = false;
             ProfileRollFlag = true;
             ZipCodeProfile--;
             if (ZipCodeProfile < 1)
               ZipCodeProfile = 1;
             Serial.print("ZipCodeProfile (L) set to ");
             Serial.println(ZipCodeProfile);
             CurrentZipLocation = HomeZipLocation;
             TimeRemaining = 0; // force advance to next zipcode
             switch (ZipCodeProfile) {
             case 1: {
               Serial.println("1st List");
               NumberOfZipCodes = Profile1ZipCount;
               break;
               }
             case 2: {
               Serial.println("2nd List");
               NumberOfZipCodes = Profile2ZipCount;
               break;
               }
             case 3: {
               Serial.println("AQI Test Colors");
               NumberOfZipCodes = 6;
               break;
               }
             case 4: {
               Serial.println("Blynk ZipCode");
               NumberOfZipCodes = 1;
               break;
               }
             }
             validIRkey = true; break;
           }
             
           if ((IRvalue == 0xFF5AA5)) { // "RIGHT"
             ZipCodeRoller = false;
             ProfileRollFlag = true;
             ZipCodeProfile++;
             if (ZipCodeProfile > NumberOfProfiles)
               ZipCodeProfile = NumberOfProfiles;
             Serial.print("ZipCodeProfile (R) set to ");
             Serial.println(ZipCodeProfile);
             CurrentZipLocation = HomeZipLocation;
             TimeRemaining = 0; // force advance to next zipcode
             switch (ZipCodeProfile) {
             case 1: {
               Serial.println("1st List");
               NumberOfZipCodes = Profile1ZipCount;
               break;
               }
             case 2: {
               Serial.println("2nd List");
               NumberOfZipCodes = Profile2ZipCount;
               break;
               }
             case 3: {
               Serial.println("AQI Test Colors");
               NumberOfZipCodes = 6;
               break;
               }
             case 4: {
               Serial.println("Blynk ZipCode");
               NumberOfZipCodes = 1;
               break;
               }
             }
           validIRkey = true; break;
           }
             
         if ((IRvalue == 0xFF38C7)) {  // "OK" (New AutoMode)
           ZipCodeRoller = false;
           // Don't change ProfileRollFlag
           Serial.println("(OK) hit to refresh current zipcode");
           //Serial.println(ZipCodeProfile);
           TimeRemaining = 0; // force refresh
           validIRkey = true; break;
           }

          if ((validIRkey == false) && (IRvalue >= 0xB00000) && (IRvalue <= 0xB99999)) {
            Serial.print("Blynk App ZipCode request r/eceived for ");
            char charVal[6];
            sprintf(charVal, "%06X", IRvalue);
            //strncpy(BlynkZipCode, charVal[1], 5);
            for (int ii=0; ii<5; ii++) {
              BlynkZipCode[ii] = charVal[ii+1];
              }
            Serial.println(BlynkZipCode);
            ZipCodeProfile = 4; // switch to Blynk profile 
            NumberOfZipCodes = 1;
            CurrentZipLocation = HomeZipLocation;
            ZipCodeRoller = false;
            ProfileRollFlag = true;
            NextRefresh = LongDelay; // restore long interval 2/12
            TimeRemaining = 0;
            // code below needs to be fully tested
                  CurrentAQIcolor = 7; // BLACK
                  Serial1.print(header[0],HEX); // x01 "SOH"
                  Serial1.print(header[1],HEX); // x02 "STX"
                  Serial.print("[");
                  Serial.print(header[0],HEX); // x01 "SOH"
                  Serial.print(header[1],HEX); // x02 "STX"
                  Serial.print(IRvalue, HEX); // "UP" arrow
                  Serial1.print(IRvalue, HEX); // "UP" arrow
                  Serial.print("][");
                  Serial.print(CurrentAQIcolor); // AQI color
                  Serial1.print(CurrentAQIcolor); // AQI color
                  Serial.print("] ");
                  Serial1.println();
                  Serial.println("AQI update sent to UNO");
            // end code above comment
            break;
            } 
           
           //Serial.println("?? Bad IR response ??");
           break; 
           }

         if (validIRkey == true) {
           digitalWrite(LED_BUILTIN, HIGH);
           Serial1.print(header[0],HEX); // x01 "SOH"
           Serial1.print(header[1],HEX); // x02 "STX"
           Serial.print("HEX [");
           Serial.print(header[0],HEX); // x01 "SOH"
           Serial.print(header[1],HEX); // x02 "STX"
           Serial.print(IRvalue, HEX);  // IR data
           Serial.print("]");
           Serial1.print(IRvalue, HEX);;  // IR data
           
           if ((IRvalue == 0x415149) || (IRvalue == 0xFF6897)) { // AQI request?
             TimeRemaining = 0; // yes, will force a refresh
             if (IRvalue == 0x415149) { // "AQI" in HEX
               Serial.print("[");
               Serial.print(CurrentAQIcolor); // AQI color
               Serial1.print(CurrentAQIcolor); // AQI color
               Serial1.println();
               Serial.print("]");
               Serial.println(" AQI refresh sent to UNO");
             }
             if (IRvalue == 0xFF6897) { // "*"
               CurrentAQIcolor = 7; // set LCD display dark
               Serial.print("[");
               Serial.print(CurrentAQIcolor); // AQI color
               Serial1.print(CurrentAQIcolor); // AQI color
               Serial1.println();
               Serial.print("]");
               Serial.println(" AQI update sent to UNO");
               ZipCodeRoller = false;
               ProfileRollFlag = true;
               CurrentZipLocation = HomeZipLocation;
               
               //------ added 2/12/21
               if (Debug == true) {
                 ZipCodeProfile = 2; // 2nd ZipCode List
                 NextRefresh = ShortDelay; // 2 minutes
                 NumberOfZipCodes = Profile2ZipCount;
                 }
               else
               {
                 ZipCodeProfile = 1; // 1st ZipCode List
                 NextRefresh = LongDelay; // 15 minutes
                 NumberOfZipCodes = Profile1ZipCount;
                 }
               //end------ added 2/12/21
                      
               }
             }
           else
           {
             Serial1.println();  
             Serial.println(" sent to UNO.");
             Serial.println();
             }
           //delay(500);
           digitalWrite(LED_BUILTIN, LOW);
           }
         last = millis();
         irrecv.resume(); // Restart
         } // end if (millis() - last > 250) { // and 1/4 sec since last one?
       } // end if (irrecv.decode(&results)) { //IR event seen ...
     // end NANO Code moved

     
     {
       CRGB Color;
       switch (CurrentAQIcolor) {
         case 0: {Color = CRGB::White; break;}
         case 1: {Color = CRGB::Green; break;}
         case 2: {Color = CRGB::Yellow; break;}
         case 3: {Color = CRGB::Orange; break;}
         case 4: {Color = CRGB::Red; break;}
         case 5: {Color = CRGB::Purple; break;}
         case 6: {Color = CRGB::FireBrick; break;}
         case 7: {Color = CRGB::Black; break;}
         case 8: {Color = CRGB::Blue; break;}
         case 9:
         default:{Color = CRGB::Black; break;}
         break;
         }
       for (int ii=0; ii<NUM_LEDS; ii++) {  
         leds[ii] = Color;
         }
       FastLED.show(); // show solid color mmm
       digitalWrite(LED_BUILTIN, HIGH);
       delay(200); // ON .2 second
       digitalWrite(LED_BUILTIN, LOW);
       if ((TimeRemaining == (NextRefresh-10)) && (lineCount < 11)) {
        // Ten seconds have elapsed with no AirNow response
        TimeRemaining = 10; // start countdown to next ZipCode
        }
       if (TimeRemaining <= 10) {
         for (int ii=0; ii<NUM_LEDS; ii++) {
           leds[ii] = CRGB::Black;
           }
         FastLED.show();
         }
       delay(800);
       }

     TimeRemaining--;

     if ((TimeRemaining % 60) == 0) { // bbb
       if (TimeRemaining == NextRefresh) {
         client.stop(); // per https://github.com/esp8266/Arduino/issues/72
         if (Debug) {
           Serial.println("$Debug -- 'client.stop' #2 sent.");
           }
         if (WiFi.RSSI() == 0) {
           // no socket available; probably no WiFiconnection
           if (true) {
            Serial.println("$Debug -- WiFi restart issued."); 
            }
          status = WiFi.begin(ssid, pass);
          // wait for the connection:
          delay(500);
          if (Debug)
            Serial.println("$Debug -- client.connectSSL() sent.");
          status = client.connectSSL(server, 443);
          //delay(TimeDelay);

          AqiValue = -1;
          strcpy(AqiQuality, NoConnection);
          strncpy(AqiType, None, 5);
          CurrentAQIcolor = 7; // black; 
          }
          
      #ifdef LCDcode
        lcd.setCursor(0,0); lcd.print(SixteenSpaces); // Last updated
        lcd.setCursor(0,0); lcd.print(location);
        lcd.setCursor(0,1); 
        if (AqiValue >= 0) {
          lcd.print("AQ update "); 
          lcd.setCursor(10,1); lcd.print(CurrentTime1); //HH
          lcd.setCursor(12,1); lcd.print(" ");
          lcd.setCursor(13,1); lcd.print(CurrentZone1); // TMZ");
          }
        else
        {
          lcd.print("No Data Results ");  
          }
        delay(TimeDelay); // currently 3 seconds
        lcd.setCursor(0,0); lcd.print(SixteenSpaces);
        lcd.setCursor(0,1); lcd.print(SixteenSpaces);
      #endif

         Serial.println();
         // update the UNO
         Serial.print("AQI value "); 
         Serial.print(AqiValue);
         Serial.print(" (");
         if (AqiValue < 0) {
            Serial.print("Not Available");
            
          #ifdef LCDcode
            lcd.setCursor(0,0); lcd.print(SixteenSpaces); // debug test 1
            lcd.setCursor(11,0); lcd.print(zipcode);
          #endif
          
          }
          else
            Serial.print(AqiQuality);
            
          Serial.print(") ");
          if (AqiValue >= 0) {
            Serial.print("for "); 
            Serial.print(AqiType);
            Serial.print(" ");
            }
         
          Serial.print("is ");
          switch (CurrentAQIcolor) {
            case 0:  {LCDcolor = "WHI"; break;}
            case 1:  {LCDcolor = "GRN"; break;}
            case 2:  {LCDcolor = "YEL"; break;}
            case 3:  {LCDcolor = "ORA"; break;}
            case 4:  {LCDcolor = "RED"; break;}
            case 5:  {LCDcolor = "PUR"; break;}
            case 6:  {LCDcolor = "MRN"; break;}
            case 7:
            default: {LCDcolor = "BLK"; break;}
            break;
            }
          Serial.print(LCDcolor);         
          Serial.print(" in ");
          Serial.print(location);
          Serial.print("  ");
          Serial.println(zipcode);
          
          Serial.print("Timer countdown running; AQI HEX [");
          digitalWrite(LED_BUILTIN, HIGH);
          Serial.print(header[0],HEX); // x01 "SOH"
          Serial1.print(header[0],HEX); // x01 "SOH"
          Serial.print(header[1],HEX); // x02 "STX"
          Serial1.print(header[1],HEX); // x02 "STX"
          IRvalue = 0x415149; // refresh request (this is AQI in HEX)
          Serial.print(IRvalue, HEX);  // IR data
          Serial1.print(IRvalue, HEX);;  // IR data
          Serial.print("][");
          Serial.print(CurrentAQIcolor); // AQI color
          Serial1.print(CurrentAQIcolor); // AQI color
          Serial1.println();
          //Serial.println();
          Serial.println("] refresh sent to UNO.");
          Serial.println();
          delay(500);
          digitalWrite(LED_BUILTIN, LOW);
          }
          
        Serial.print("Next AQI refresh is ");
        if (TimeRemaining > 0) {  
          Serial.print("in ");
          Serial.print(TimeRemaining);
          Serial.println(" seconds ...");
          }
        else
          {
            Serial.println("now !!");
            //Serial.println();
          }
        } // bbb


     int Remaining = int(TimeRemaining % 30);
     //(TimeRemaining <= NextRefresh) &&
     if ( ((Remaining) != 3) || (AqiValue <= 0) || 
          (TimeRemaining >= NextRefresh) || (TimeRemaining == 3) )
       {
         int MinutesRemaining = int(TimeRemaining / 60);
      
    #ifdef LCDcode
      lcd.setCursor(11,1); 
      if (MinutesRemaining < 10) {
        lcd.print("0");
        lcd.setCursor(12,1);  
        }
      lcd.print(MinutesRemaining);
      lcd.setCursor(13,1);
      lcd.print(":");
      int SecondsRemaining = int(TimeRemaining % 60);
      lcd.setCursor(14,1);
      if (SecondsRemaining <10) {
        lcd.print("0");
        lcd.setCursor(15,1);
        }
      lcd.print(SecondsRemaining);
    #endif
    
      }
      else
      {
        // time still running; show location
        
    #ifdef LCDcode
        lcd.setCursor(0,0); lcd.print(SixteenSpaces); // Last updated
        lcd.setCursor(0,0); lcd.print(location);
        lcd.setCursor(0,1); 
        if (AqiValue >= 0) {
          lcd.print("AQ update "); 
          lcd.setCursor(10,1); lcd.print(CurrentTime1); //HH
          lcd.setCursor(12,1); lcd.print(" ");
          lcd.setCursor(13,1); lcd.print(CurrentZone1); // TMZ");
          }
          //else
          //{
            //lcd.print("No Data Results ");  
          //}
          delay(TimeDelay); // currently 3 seconds
          TimeRemaining = TimeRemaining - ((TimeDelay/1000)-1);
          lcd.setCursor(0,0); lcd.print(SixteenSpaces);
          lcd.setCursor(0,1); lcd.print(SixteenSpaces);
    #endif
    
      }
    
      if (TimeRemaining <= 0) {
        digitalWrite(LED_BUILTIN, LOW); // reset
        break;
        }
      } // end else // IR processing
    
    if (lineCount == 9) { // connection closed; data follows
      
  #ifdef LCDcode
      lcd.setCursor(0,0); lcd.print(SixteenSpaces);
  #endif
  
      }
      
  if (lineCount == 10) { // found a header record
  // available to do something
    }
      
    if (lineCount >= 11) { // found data record(s)
      // blank both lines, display highest reading
      //lcd.setCursor(0,0); //lcd.print(SixteenSpaces);
      //lcd.setCursor(0,1); //lcd.print(ElevenSpaces);
      //Final LCD Line2 example: "PType Qual MM:SS";
      
    #ifdef LCDcode
      lcd.setCursor(0,0);
      if (ZipCodeRoller != false) { // unlocked ?
        lcd.print("AQ "); // yes, show upper-case
      } else {
        lcd.print("aq "); // no, show lower-case
      }
      int pInfo = AqiValue;
      while (true) {
        if (pInfo >= 100){
          lcd.setCursor(3,0); lcd.print(pInfo);
          break;
          }
        if (pInfo >= 10) {
          lcd.setCursor(3,0); lcd.print("0"); // leading zero
          lcd.setCursor(4,0); lcd.print(pInfo);
          break;
          }
        if (pInfo >= 0) {
          lcd.setCursor(3,0); lcd.print("00"); // leading zeros
          lcd.setCursor(5,0); lcd.print(pInfo);
          break;  
          }
          
        Serial.println("?? Bad Exit ??");
        break;
        } // end while (true)
     
      lcd.setCursor(6,0); lcd.print(" ");
      lcd.print(LCDcolor);
      lcd.setCursor(10,0); lcd.print(" ");
      lcd.print(zipcode);  // *** zip
    #endif

    #ifdef LCDcode
      //Final LCD Line2 example -- "PType Qual 12:34";
      //lcd.setCursor(0,1); 
      //lcd.print(ElevenSpaces);
      lcd.setCursor(0,1); lcd.print(AqiType);
      lcd.setCursor(5,1); lcd.print(" ");
      char char4[] = "    ";
      strncpy(char4, AqiQuality,4);
      lcd.setCursor(6,1); lcd.print(char4);
      lcd.setCursor(10,1); lcd.print(" ");
    #endif

        } // end if (lineCount >= 10) { // found data records
      if (ServerDisconnected == true) break;
      } // end while (true)
    break;
    } // end if (TimeRemaining <= 0)
  } // end while (true)
} // end void loop()

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print(" SSID: ");
  Serial.print(WiFi.SSID());
  Serial.println(" !!");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("local IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  rssi = WiFi.RSSI();
  Serial.print("RSSI signal strength: ");
  Serial.print(rssi);
  Serial.println(" dBm");
  }

uint32_t AQI2hex(uint16_t AQI) {
  if (AQI < 0)    return(WHITE); // no data
  if (AQI <=  50) return(GREEN);
  if (AQI <= 100) return(YELLOW);
  if (AQI <= 150) return(ORANGE);
  if (AQI <= 200) return(RED);
  if (AQI <= 300) return(PURPLE);
  if (AQI <= 500) return(MAROON);
  return(BLACK); // error
  }

void LEDflash() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200); // ON .2 second
  digitalWrite(LED_BUILTIN, LOW);
  delay(800); // OFF .8 second
  //Serial.print("$Debug -- LEDflash() called; ");
  //Serial.println(CurrentAQIcolor);
  }
