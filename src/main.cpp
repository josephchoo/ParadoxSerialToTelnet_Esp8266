// Modified version by Broadica
// Credits Reference 
// ESP8266 port by Atadiat
// ESP32 WiFi <-> 3x UART Bridge by AlphaLima www.LK8000.com

#include <ESP8266WiFi.h>
//////////////////////////////////////////////////////////////////////////
//////////////////////////////  Config  //////////////////////////////////
//#define MODE_AP // phone connects directly to ESP
#define MODE_STA // ESP connects to WiFi router
#define IP_STATIC 

bool debug = false;
const char *VERSION = "1.10";

#ifdef MODE_AP
// For AP mode:
const char *ssid = "Serial_wifi";  // You will connect your phone to this Access Point
const char *pw = "123456789"; // and this is the password
IPAddress ip(192, 168, 4, 1); // From RoboRemo app, connect to this IP
IPAddress netmask(255, 255, 255, 0);
// menu -> connect -> Internet(TCP) -> 192.168.4.1:8880  for UART0
#endif

#ifdef MODE_STA
// For STATION mode:
const char *ssid = "Serial_wifi";  // Your ROUTER SSID
const char *pw = "123456789"; // and WiFi PASSWORD
// Set your Static IP address
IPAddress staticIP(192, 168, 10, 22);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);
// menu -> connect -> Internet(TCP) -> [ESP_IP]:8880  for UART0
#endif

/*************************  COM Port 0 *******************************/
#define UART_BAUD0 9600            // Baudrate UART0
#define SERIAL_PARAM0 SERIAL_8N1    // Data/Parity/Stop UART0
#define SERIAL0_TCP_PORT 8880       // Wifi Port UART0
/*************************  COM Port 1 *******************************/
#define UART_BAUD1 9600            // Baudrate UART0
#define SERIAL_PARAM1 SERIAL_8N1    // Data/Parity/Stop UART0
#define SERIAL1_TCP_PORT 8881       // Wifi Port UART0

#define bufferSize 1024
#define MAX_NMEA_CLIENTS 2
#define NUM_COM 2
#define DEBUG_COM 0 // debug output to COM0
//////////////////////////////////////////////////////////////////////////

#include <WiFiClient.h>
WiFiServer server_0(SERIAL0_TCP_PORT);
WiFiServer server_1(SERIAL1_TCP_PORT);
WiFiServer *server[NUM_COM] = {&server_0, &server_1};
WiFiClient TCPClient[NUM_COM][MAX_NMEA_CLIENTS];

HardwareSerial* COM[NUM_COM] = {&Serial, &Serial1};

uint8_t buf1[NUM_COM][bufferSize];
uint16_t i1[NUM_COM] = {0,0};

uint8_t buf2[bufferSize];
uint16_t i2[NUM_COM] = {0,0};

unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long interval = 30000;

void initWiFi() {
  WiFi.disconnect(true); // somehow clean up connection issue

  if (debug) COM[DEBUG_COM]->println("WiFi Station mode");
  // STATION mode (ESP connects to router and gets an IP)
  // Assuming phone is also connected to that router
  // from RoboRemo you must connect to the IP of the ESP
  WiFi.mode(WIFI_STA);
#ifdef IP_STATIC
  if (!WiFi.config(staticIP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
#endif
  WiFi.begin(ssid, pw);
  if (debug) COM[DEBUG_COM]->print("Connecting to: ");
  if (debug) COM[DEBUG_COM]->println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    if (debug) COM[DEBUG_COM]->print(".");
    delay(1000);
  }
  if (debug) COM[DEBUG_COM]->println("");
  if (debug) COM[DEBUG_COM]->print("WiFi connected ");
  if (debug) COM[DEBUG_COM]->println(WiFi.localIP());
  if (debug) COM[DEBUG_COM]->print("RRSI: ");
  if (debug) COM[DEBUG_COM]->println(WiFi.RSSI());

  if (WiFi.localIP().toString().startsWith("169.254")) {
    ESP.restart(); //restart if it get automatic private ip; means it cant get ip from DHCP 
  }
}

void setup() {
  delay(500);

  COM[0]->begin(UART_BAUD0, SERIAL_PARAM0, SERIAL_FULL);
  COM[1]->begin(UART_BAUD1, SERIAL_PARAM1, SERIAL_FULL);

  if (debug) COM[DEBUG_COM]->println();
  if (debug) COM[DEBUG_COM]->print("WiFi serial bridge V");
  if (debug) COM[DEBUG_COM]->println(VERSION);

#ifdef MODE_AP
  if (debug) COM[DEBUG_COM]->println("Open ESP Access Point mode");
  //AP mode (phone connects directly to ESP) (no router)
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP
#endif

#ifdef MODE_STA
  initWiFi();
#endif

  if (debug) COM[DEBUG_COM]->println("Starting TCP Server 1");
  server[0]->begin(); // start TCP server
  server[0]->setNoDelay(true);
  COM[1]->println("Starting TCP Server 2");
  if (debug) COM[DEBUG_COM]->println("Starting TCP Server 2");
  server[1]->begin(); // start TCP server
  server[1]->setNoDelay(true);
}

void loop()
{
  currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    if (debug) COM[DEBUG_COM]->println(millis());
    if (debug) COM[DEBUG_COM]->println("Reconnecting to WiFi...");
    ESP.restart();
    previousMillis = currentMillis;
  }

  for (int num = 0; num < NUM_COM ; num++)
  {
    if (server[num]->hasClient())
    {
      for (byte i = 0; i < MAX_NMEA_CLIENTS; i++) {
        //find free/disconnected spot
        if (!TCPClient[num][i] || !TCPClient[num][i].connected()) {
          if (TCPClient[num][i]) TCPClient[num][i].stop();
          TCPClient[num][i] = server[num]->available();
          if (debug) COM[DEBUG_COM]->print("New client for COM");
          if (debug) COM[DEBUG_COM]->print(num);
          if (debug) COM[DEBUG_COM]->print('/');
          if (debug) COM[DEBUG_COM]->print(i);
          if (debug) COM[DEBUG_COM]->print(' ');
          if (debug) COM[DEBUG_COM]->println(TCPClient[num][i].localIP().toString());
          continue;
        }
      }
      //no free/disconnected spot so reject
      WiFiClient TmpserverClient = server[num]->available();
      TmpserverClient.stop();
    }
  }

  for (int num = 0; num < NUM_COM ; num++)
  {
    if (COM[num] != NULL)
    {
      for (byte cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
      {
        if (TCPClient[num][cln])
        {
          while (TCPClient[num][cln].available())
          {
            buf1[num][i1[num]] = TCPClient[num][cln].read(); // read char from client (LK8000 app)
            if (i1[num] < bufferSize - 1) i1[num]++;
          }

          COM[num]->write(buf1[num], i1[num]); // now send to UART(num):
          i1[num] = 0;
        }
      }

      if (COM[num]->available())
      {
        String myString[NUM_COM] ;
        while (COM[num]->available())
        {
          myString[num] += (char)COM[num]->read(); // read char from UART(num)
        }

        // now send to WiFi:
        for(byte cln = 0; cln < MAX_NMEA_CLIENTS; cln++)
        {   
          if(TCPClient[num][cln])                     
            TCPClient[num][cln].print(myString[num]);
        }        
        i2[num] = 0;
      }
    }    
  }
}