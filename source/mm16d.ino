// +---------------------------------------------------------------------------+
// | MM16D v0.1 * Grow house control device                                    |
// | Copyright (C) 2023 Pozsár Zsolt <pozsarzs@gmail.com>                      |
// | mm16d.ino                                                                 |
// | Program for Adafruit Huzzah Feather                                       |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the European Union Public License 1.2 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ModbusIP_ESP8266.h>
#include <ModbusRTU.h>
#include <NTPClient.h>

// settings
const char   *WIFI_SSID                 = "";
const char   *WIFI_PASSWORD             = "";
const char   *NTPSERVER                 = "europe.pool.ntp.org";
const String  DBSERVER                  = "0.0.0.0";

// ports
const int     PRT_AI_OPMODE             = 0; // analog input
const int     PRT_DI_ALARM              = 5;
const int     PRT_DI_OCPROT             = 13;
const int     PRT_DI_SWMANU             = 12;
const int     PRT_DO_BUZZER             = 4;
const int     PRT_DO_LEDBLUE            = 2;
const int     PRT_DO_LEDRED             = 0;
const int     PRT_DO_HEAT               = 16;
const int     PRT_DO_LAMP               = 14;
const int     PRT_DO_VENT               = 15;

// output data
const String  B_DESC[23]                = {"sign lights (0/1: red/green)",
                                           "alarm",
                                           "overcurrent breaker (1: error)",
                                           "no connection to the NTP server (1: error)",
                                           "no connection to the database server (1: error)",
                                           "no connection to the MM17D (1: error)",
                                           "stand-by operation mode",
                                           "growing hyphae operation mode",
                                           "growing mushroom operation mode",
                                           "manual switch (0/1: auto/manual)",
                                           "enable lamp output",
                                           "enable ventilator output",
                                           "enable heater output",
                                           "status of the lamp output",
                                           "status of the ventilator output",
                                           "status of the heater output",
                                           "internal humidity is less than requied",
                                           "internal humidity is more than requied",
                                           "internal temperature is less than requied",
                                           "internal temperature is more than requied",
                                           "MM17D green LED",
                                           "MM17D yellow LED",
                                           "MM17D red LED"
                                          };
const String  I_DESC[3]                 = {"MM17D int. humidity in percent",
                                           "MM17D int. temperature in degree Celsius",
                                           "MM17D ext. temperature in degree Celsius"
                                          };
const String  B_NAME[23]                = {"signlight", "alarm", "breaker", "errorntp", "errordb",
                                           "errormm17d", "standby", "hyphae", "mushroom", "manual",
                                           "enalamp", "enavent", "enaheater", "lamp", "vent",
                                           "heater", "rhintless", "rhintmore", "tintless", "tintmore",
                                           "mm17dgreen", "mm17dyellow", "mm17dred"
                                          };
const String  I_NAME[3]                 = {"rhint", "tint", "text"};
boolean       b_values[23]              = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int           i_values[3]               = {0, 0, 0};

// other constants
const byte    SWMVERSION                = 0;
const byte    SWSVERSION                = 1;
const int     SERIALSPEED               = 9600;
const int     MB_UID                    = 1;
const long    INTERVAL                  = 10000;
const String  TEXTHTML                  = "text/html";
const String  TEXTPLAIN                 = "text/plain";

// environment parameters with default values
// growing hyphae: no light and airing
boolean       hheater_disable[24]       = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int           hheater_on                = 0;  // in degree Celsius
int           hheater_off               = 0;  // in degree Celsius
int           htemperature_min          = 0;  // in degree Celsius
int           htemperature_max          = 0;  // in degree Celsius
int           hhumidity_min             = 0;  // in percent
int           hhumidity_max             = 0;  // in percent
// growing mushroom
boolean       mheater_disable[24]       = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
boolean       mvent_disable[24]         = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
boolean       mvent_disablehightemp[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
boolean       mvent_disablelowtemp[24]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int           mheater_on                = 0;  // in degree Celsius
int           mheater_off               = 0;  // in degree Celsius
int           mtemperature_min          = 0;  // in degree Celsius
int           mtemperature_max          = 0;  // in degree Celsius
int           mhumidity_min             = 0;  // in percent
int           mhumidity_max             = 0;  // in percent
int           mlight_off                = 0;  // in hour
int           mlight_on                 = 0;  // in hour
int           mvent_on                  = 0;  // in minute
int           mvent_off                 = 0;  // in minute
int           mvent_hightemp            = 30; // in degree Celsius
int           mvent_lowtemp             = 10; // in degree Celsius

// other variables
int           syslog[64]                = {};
String        line;
String        myipaddress;
String        mymacaddress;
String        swversion;
unsigned long prevtime                  = 0;

// messages
const String MSG[39]                    =
{
  /*  0 */  "",
  /*  1 */  "MM16D * Grow house control device",
  /*  2 */  "Copyright (C) 2023 Pozsar Zsolt",
  /*  3 */  "http://www.pozsarzs.hu/",
  /*  4 */  "MM16D",
  /*  5 */  "* Initializing GPIO ports...",
  /*  6 */  "* Connecting to wireless network",
  /*  7 */  "* Starting NTP client",
  /*  8 */  "done",
  /*  9 */  "  my MAC address:         ",
  /* 10 */  "  my IP address:          ",
  /* 11 */  "  subnet mask:            ",
  /* 12 */  "  gateway IP address:     ",
  /* 13 */  "* Starting webserver...",
  /* 14 */  "* HTTP query received/transmitted ",
  /* 15 */  "* Modbus/TCP query received ",
  /* 16 */  "* Modbus/RTU query received/transmitted ",
  /* 17 */  "* E01: Cannot get setting from database!",
  /* 18 */  "* E02: Cannot put status data to database!",
  /* 19 */  "* E03: Error 404: page not found!",
  /* 20 */  "* E04: Cannot update system time!",
  /* 21 */  "* E05: Cannot access MM17D device!",
  /* 22 */  "* W01: Internal humidity is less than requied!",
  /* 23 */  "* W02: Internal humidity is more than requied!",
  /* 24 */  "* W03: Internal temperature is less than requied!",
  /* 25 */  "* W04: Internal temperature is more than requied!",
  /* 26 */  "* Red",
  /* 27 */  " LED is switched ",
  /* 28 */  "on.",
  /* 29 */  "off.",
  /* 30 */  "* Attention! The serial console is off!",
  /* 31 */  "  get summary page",
  /* 32 */  "  get help page",
  /* 33 */  "  get log page",
  /* 34 */  "  get all status data",
  /* 35 */  "* Starting Modbus/TCP server...",
  /* 36 */  "* Starting Modbus/RTU slave...",
  /* 37 */  "  my Modbus UID:          ",
  /* 38 */  "  serial port parameters: "
};

WiFiUDP ntpUDP;
ESP8266WebServer server(80);
ModbusIP mbtcp;
ModbusRTU mbrtu;
NTPClient timeClient(ntpUDP, NTPSERVER, 0, 32767);

// write a line to system log
void writetosyslog(int msgnum)
{
  if (syslog[63] == 0)
  {
    for (int i = 0; i < 64; i++)
    {
      if (syslog[i] == 0)
      {
        syslog[i] = msgnum;
        break;
      }
    }
  } else
  {
    for (int i = 1; i < 64; i++)
      syslog[i - 1] = syslog[i];
    syslog[63] = msgnum;
  }
}

// switch on/off blue LED
void blueled(boolean b)
{
  digitalWrite(PRT_DO_LEDBLUE, b);
}

// switch on/off red LED
void redled(boolean b)
{
  digitalWrite(PRT_DO_LEDRED, b);
  b_values[0] = b;
  mbtcp.Ists(0, b_values[0]);
  mbrtu.Ists(0, b_values[0]);
}

// blinking blue LED
void blinkblueled()
{
  blueled(true);
  delay(25);
  blueled(false);
}

// blinking all LEDs
void blinkallleds()
{
  blueled(true);
  delay(75);
  blueled(false);
  redled(true);
  delay(75);
  redled(false);
}

// beep sign
void beep(int num)
{
  for (int i = 0; i < num; i++)
  {
    tone(PRT_DO_BUZZER, 880);
    delay (100);
    noTone(PRT_DO_BUZZER);
    delay (100);
  }
}

// read configuration from remote database
boolean readconfigfromremotedb(String ipaddress)
{
  return false;
}

// read environment parameters from MM17D device
boolean readenvirparamsfromdevice()
{
  return false;
}

// read GPIO ports
void getinputs()
{
  for (int i = 6; i < 8; i++) b_values[i] = false;
  if (int(analogRead(PRT_AI_OPMODE)) < 100)
  {
    b_values[6] = true;
  } else
  {
    if (int(analogRead(PRT_AI_OPMODE)) > 900)
    {
      b_values[8] = true;
    } else
    {
      b_values[7] = true;
    }
  }
  b_values[1] = digitalRead(PRT_DI_ALARM);
  b_values[2] = digitalRead(PRT_DI_OCPROT);
  b_values[9] = digitalRead(PRT_DI_SWMANU);
}

void analise()
{
  int h = timeClient.getHours();
  int m = timeClient.getHours();

  // extreme measured values
  // - bad humidity
  if (i_values[0] < mhumidity_min) i_values[16] = true; else i_values[16] = false;
  if (i_values[0] > mhumidity_max) i_values[17] = true; else i_values[17] = false;
  // - bad temperature
  if (i_values[1] < mtemperature_min) i_values[18] = true; else i_values[18] = false;
  if (i_values[1] > mtemperature_max) i_values[19] = true; else i_values[19] = false;
  // error LED / green-red sign light
  b_values[0] = false;
  // alarm, breaker, manual mode
  if ((b_values[1]) or (b_values[2]) or (b_values[9])) b_values[0] = true;
  if (! b_values[6])
  {
    if (b_values[7])
    {
      // HYPHAE OPERATION MODE
      // extreme environment parameters
      // - bad humidity
      if (i_values[0] < hhumidity_min) i_values[16] = true; else i_values[16] = false;
      if (i_values[0] > hhumidity_max) i_values[17] = true; else i_values[17] = false;
      // - bad temperature
      if (i_values[1] < htemperature_min) i_values[18] = true; else i_values[18] = false;
      if (i_values[1] > htemperature_max) i_values[19] = true; else i_values[19] = false;
      // lamp
      b_values[13] = false;
      // ventilator
      b_values[14] = false;
      // heater
      // - switching according to measured parameters
      if (i_values[1] < hheater_on) b_values[15] = true;
      if (i_values[1] > hheater_off) b_values[15] = false;
      // - timed blocking
      if (hheater_disable[h]) b_values[15] = false;
    } else
    {
      if (b_values[8])
      {
        // MUSHROOM OPERATION MODE
        // extreme environment parameters
        // - bad humidity
        if (i_values[0] < mhumidity_min) i_values[16] = true; else i_values[16] = false;
        if (i_values[0] > mhumidity_max) i_values[17] = true; else i_values[17] = false;
        // - bad temperature
        if (i_values[1] < mtemperature_min) i_values[18] = true; else i_values[18] = false;
        if (i_values[1] > mtemperature_max) i_values[19] = true; else i_values[19] = false;
        // lamp
        // - switching according to measured parameters
        if ((h >= mlight_on) and (h < mlight_off)) b_values[13] = true; else b_values[13] = false;
        // ventilator
        // - switching according to measured parameters
        if ((m > mvent_on) and (m < mvent_off)) b_values[14] = true; else b_values[14] = false;
        // - timed and external temperature blocking
        if (mvent_disable[h]) b_values[14] = false;
        if ((i_values[2] < mvent_lowtemp) and (mvent_disablelowtemp[h])) b_values[14] = false;
        if ((i_values[2] > mvent_hightemp) and (mvent_disablehightemp[h])) b_values[14] = false;
        // - overriding due to extreme measured parameters
        if ((i_values[17]) and (i_values[2] < mtemperature_max)) b_values[14] = true;
        if ((i_values[19]) and (i_values[2] < mtemperature_max)) b_values[14] = true;
        // heater
        if (i_values[1] < mheater_on) b_values[15] = true;
        if (i_values[1] > mheater_off) b_values[15] = false;
        // - timed blocking
        if (mheater_disable[h]) b_values[15] = false;
      }
    }
  } else
  {
    // STAND-BY OPERATION MODE
    for (int i = 13; i < 15; i++) b_values[i] = false;
  }
  for (int i = 16; i < 19; i++)
    if (b_values[i]) writetosyslog(i + 6);
}

// write GPIO ports
void setoutputs()
{
  digitalWrite(PRT_DO_LEDRED, b_values[0]);
  // enable/disable outputs
  for (int i = 10; i < 12; i++)
    if (! b_values[i]) b_values[i + 3] = false;
  digitalWrite(PRT_DO_LAMP, b_values[13]);
  digitalWrite(PRT_DO_VENT, b_values[14]);
  digitalWrite(PRT_DO_HEAT, b_values[15]);
}

// write log to remote database
boolean writelogtoremotedb(String ipaddress)
{
  return false;
}

// blink blue LED and write to log
void httpquery()
{
  blinkblueled();
  writetosyslog(14);
}

// blink blue LED and write to log
uint16_t modbustcpquery(TRegister* reg, uint16_t val)
{
  blinkblueled();
  writetosyslog(15);
  return val;
}

// blink blue LED and write to log
uint16_t modbusrtuquery(TRegister* reg, uint16_t val)
{
  blinkblueled();
  writetosyslog(16);
  return val;
}

// error 404 page
void handleNotFound()
{
  httpquery();
  writetosyslog(19);
  server.send(404, TEXTPLAIN, MSG[19]);
}

// loop function
void loop(void)
{
  timeClient.update();
  b_values[3] = !timeClient.isTimeSet();
  if (b_values[3]) writetosyslog(20);
  unsigned long currtime = millis();
  if (currtime - prevtime >= INTERVAL)
  {
    prevtime = currtime;
    b_values[4] = ! readconfigfromremotedb(DBSERVER);
    b_values[5] = ! readenvirparamsfromdevice();
    if (b_values[4]) writetosyslog(17);
    if (b_values[5]) writetosyslog(21);
    getinputs();
    analise();
    setoutputs();
    b_values[4] = ! writelogtoremotedb(DBSERVER);
    if (b_values[4]) writetosyslog(18);
  }
  mbtcp.task();
  delay(10);
  mbrtu.task();
  yield();
}

// initializing function
void setup(void)
{
  swversion = String(SWMVERSION) + "." + String(SWSVERSION);
  // set serial port
  Serial.begin(SERIALSPEED);
  // write program information
  Serial.println("");
  Serial.println("");
  Serial.println(MSG[1] + " * v" + swversion );
  Serial.println(MSG[2] +  " <" + MSG[3] + ">");
  // initialize GPIO ports
  writetosyslog(5);
  Serial.println(MSG[5]);
  pinMode(PRT_DO_BUZZER, OUTPUT);
  pinMode(PRT_DO_LEDBLUE, OUTPUT);
  pinMode(PRT_DO_LEDRED, OUTPUT);
  digitalWrite(PRT_DO_LEDBLUE, LOW);
  digitalWrite(PRT_DO_LEDRED, LOW);
  // connect to wireless network
  writetosyslog(6);
  Serial.print(MSG[6]);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    blinkallleds();
    Serial.print(".");
  }
  Serial.println(MSG[8]);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  myipaddress = WiFi.localIP().toString();
  mymacaddress = WiFi.macAddress();
  Serial.println(MSG[9] + mymacaddress);
  Serial.println(MSG[10] + myipaddress);
  Serial.println(MSG[11] + WiFi.subnetMask().toString());
  Serial.println(MSG[12] + WiFi.gatewayIP().toString());
  // start NTP client
  writetosyslog(7);
  Serial.print(MSG[7]);
  timeClient.begin();
  // start Modbus/TCP server
  writetosyslog(35);
  Serial.print(MSG[35]);
  mbtcp.server();
  Serial.println(MSG[8]);
  // start Modbus/RTU slave
  writetosyslog(36);
  Serial.print(MSG[36]);
  mbrtu.begin(&Serial);
  mbrtu.setBaudrate(SERIALSPEED);
  mbrtu.slave(MB_UID);
  Serial.println(MSG[8]);
  Serial.println(MSG[37] + String(MB_UID));
  Serial.println(MSG[38] + String(SERIALSPEED) + " bps, 8N1");
  // set Modbus registers
  for (int i = 0; i < 22; i++)
  {
    mbtcp.addIsts(i, b_values[i]);
    mbrtu.addIsts(i, b_values[i]);
  }
  for (int i = 0; i < 2; i++)
  {
    mbtcp.addIreg(i, i_values[i]);
    mbrtu.addIreg(i, i_values[i]);
  }
  mbtcp.addIreg(9998, SWMVERSION * 256 + SWSVERSION);
  mbrtu.addIreg(9998, SWMVERSION * 256 + SWSVERSION);
  // set Modbus callback
  mbtcp.onGetIsts(0, modbustcpquery, 23);
  mbtcp.onGetIreg(0, modbustcpquery, 3);
  mbtcp.onGetIreg(9998, modbustcpquery);
  mbrtu.onGetIsts(0, modbustcpquery, 23);
  mbrtu.onGetIreg(0, modbusrtuquery, 3);
  mbrtu.onGetIreg(9998, modbusrtuquery);
  // start webserver
  writetosyslog(13);
  Serial.print(MSG[13]);
  server.onNotFound(handleNotFound);
  // help page
  server.on("/", []()
  {
    writetosyslog(32);
    line =
      "<html>\n"
      "  <head>\n"
      "    <title>" + MSG[1] + " | Help page</title>\n"
      "  </head>\n"
      "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
      "    <h2>" + MSG[1] + "</h2>\n"
      "    <br>\n"
      "    " + MSG[9] + mymacaddress + "<br>\n"
      "    " + MSG[10] + myipaddress + "<br>\n"
      "    " + MSG[37] + String(MB_UID) + "<br>\n"
      "    " + MSG[38] + String(SERIALSPEED) + " bps, 8N1<br>\n"
      "    software version: v" + swversion + "<br>\n"
      "    <hr>\n"
      "    <h3>Information and data access</h3>\n"
      "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Information pages</b></td></tr>\n"
      "      <tr>\n"
      "        <td>\n"
      "          <a href=\"http://" + myipaddress + "/\">http://" + myipaddress + "/</a>"
      "        </td>\n"
      "        <td>Help</td>\n"
      "        <td>" + TEXTHTML + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>\n"
      "          <a href=\"http://" + myipaddress + "/summary\">http://" + myipaddress + "/summary</a>"
      "        </td>\n"
      "        <td>Summary page</td>\n"
      "        <td>" + TEXTHTML + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>\n"
      "          <a href=\"http://" + myipaddress + "/log\">http://" + myipaddress + "/log</a>"
      "        </td>\n"
      "        <td>Log page</td>\n"
      "        <td>" + TEXTHTML + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Data access with HTTP</b></td>\n"
      "      <tr>\n"
      "        <td>\n"
      "          <a href=\"http://" + myipaddress + "/get/csv\">http://" + myipaddress + "/get/csv</a>"
      "        </td>\n"
      "        <td>Get all measured values in CSV format</td>\n"
      "        <td>" + TEXTPLAIN + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>\n"
      "          <a href=\"http://" + myipaddress + "/get/json\">http://" + myipaddress + "/get/json</a>"
      "        </td>\n"
      "        <td>Get all measured values in JSON format</td>\n"
      "        <td>" + TEXTPLAIN + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>\n"
      "          <a href=\"http://" + myipaddress + "/get/txt\">http://" + myipaddress + "/get/txt</a>"
      "        </td>\n"
      "        <td>Get all measured values in TXT format</td>\n"
      "        <td>" + TEXTPLAIN + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>\n"
      "          <a href=\"http://" + myipaddress + "/get/xml\">http://" + myipaddress + "/get/xml</a>"
      "        </td>\n"
      "        <td>Get all measured values in XML format</td>\n"
      "        <td>" + TEXTPLAIN + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Data access with Modbus</b></td>\n";
    for (int i = 0; i < 22; i++)
    {
      line = line +
             "      <tr>\n"
             "        <td>" + String(i + 10001) + "</td>\n"
             "        <td>" + B_DESC[i] + "</td>\n"
             "        <td>bit</td>\n"
             "      </tr>\n";
    }
    for (int i = 0; i < 2; i++)
    {
      line = line +
             "      <tr>\n"
             "        <td>" + String(i + 30001) + "</td>\n"
             "        <td>" + I_DESC[i] + "</td>\n"
             "        <td>integer</td>\n"
             "      </tr>\n";
    }
    line = line +
           "      <tr>\n"
           "        <td>39999</td>\n"
           "        <td>Software version</td>\n"
           "        <td>two byte</td>\n"
           "      </tr>\n"
           "    </table>\n"
           "    <br>\n"
           "    <hr>\n"
           "    <center>" + MSG[2] + " <a href=\"" + MSG[3] + "\">" + MSG[3] + "</a><center>\n"
           "    <br>\n"
           "  </body>\n"
           "</html>\n";
    server.send(200, TEXTHTML, line);
    httpquery();
    delay(100);
  });
  // summary page
  server.on("/summary", []()
  {
    writetosyslog(31);
    line =
      "<html>\n"
      "  <head>\n"
      "    <title>" + MSG[1] + " | Summary page</title>\n"
      "  </head>\n"
      "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
      "    <h2>" + MSG[1] + "</h2>\n"
      "    <br>\n"
      "    " + MSG[9] + mymacaddress + "<br>\n"
      "    " + MSG[10] + myipaddress + "<br>\n"
      "    " + MSG[37] + String(MB_UID) + "<br>\n"
      "    " + MSG[38] + String(SERIALSPEED) + " bps, 8N1<br>\n"
      "    software version: v" + swversion + "<br>\n"
      "    <hr>\n"
      "    <h3>Measured values</h3>\n"
      "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n";
    for (int i = 0; i < 2; i++)
    {
      line = line +
             "      <tr>\n"
             "        <td>" + I_DESC[i] + "</td>\n"
             "        <td align=\"right\">" + String(i_values[i]) + " %</td>\n"
             "      </tr>\n";
    }
    for (int i = 0; i < 22; i++)
    {
      line = line +
             "      <tr>\n"
             "        <td>" + B_DESC[i] + "</td>\n"
             "        <td align=\"right\">" + String(b_values[i]) + " %</td>\n"
             "      </tr>\n";
    }
    line = line +
           "    </table>\n"
           "    <br>\n"
           "    <hr>\n"
           "    <center>" + MSG[2] + " <a href=\"" + MSG[3] + "\">" + MSG[3] + "</a><center>\n"
           "    <br>\n"
           "  </body>\n"
           "</html>\n";
    server.send(200, TEXTHTML, line);
    httpquery();
    delay(100);
  });
  // log page
  server.on("/log", []()
  {
    writetosyslog(33);
    line =
      "<html>\n"
      "  <head>\n"
      "    <title>" + MSG[1] + " | System log</title>\n"
      "  </head>\n"
      "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
      "    <h2>" + MSG[1] + "</h2>\n"
      "    <br>\n"
      "    " + MSG[9] + mymacaddress + "<br>\n"
      "    " + MSG[10] + myipaddress + "<br>\n"
      "    " + MSG[37] + String(MB_UID) + "<br>\n"
      "    " + MSG[38] + String(SERIALSPEED) + " bps, 8N1<br>\n"
      "    software version: v" + swversion + "<br>\n"
      "    <hr>\n"
      "    <h3>Last 64 lines of system log:</h3>\n"
      "    <table border=\"0\" cellpadding=\"3\" cellspacing=\"0\">\n";
    for (int i = 0; i < 64; i++)
      if (syslog[i] > 0)
        line = line + "      <tr><td><pre>" + String(i) + "</pre></td><td><pre>" + MSG[syslog[i]] + "</pre></td></tr>\n";
    line = line +
           "    </table>\n"
           "    <br>\n"
           "    <hr>\n"
           "    <center>" + MSG[2] + " <a href=\"" + MSG[3] + "\">" + MSG[3] + "</a><center>\n"
           "    <br>\n"
           "  </body>\n"
           "</html>\n";
    server.send(200, TEXTHTML, line);
    httpquery();
    delay(100);
  });
  // get all measured data in CSV format
  server.on("/get/csv", []()
  {
    writetosyslog(34);
    line = "\"name\",\"" + MSG[4] + "\"\n"
           "\"version\",\"" + swversion + "\"\n";
    for (int i = 0; i < 2; i++)
      line = line + "\"" + I_NAME[i] + "\",\"" + String(i_values[i]) + "\"\n";
    for (int i = 0; i < 22; i++)
      line = line + "\"" + B_NAME[i] + "\",\"" + String(b_values[i]) + "\"\n";
    server.send(200, TEXTPLAIN, line);
    httpquery();
    delay(100);
  });
  // get all measured values in JSON format
  server.on("/get/json", []()
  {
    writetosyslog(34);
    line = "{\n"
           "  {\n"
           "    \"name\": \"" + MSG[4] + "\",\n"
           "    \"version\": \"" + swversion + "\"\n"
           "  },\n"
           "  {\n";
    for (int i = 0; i < 2; i++)
      line = line + "    \"" + I_NAME[i] + "\": \"" + String(i_values[i]) + "\",\n";
    line = line +
           "  },\n"
           "  {\n";
    for (int i = 0; i < 22; i++)
      line = line + "    \"" + B_NAME[i] + "\": \"" + String(b_values[i]) + "\",\n";
    line = line +
           "  }\n"
           "}";
    server.send(200, TEXTPLAIN, line);
    httpquery();
    delay(100);
  });
  // get all measured data in TXT format
  server.on("/get/txt", []()
  {
    writetosyslog(34);
    line = MSG[4] + "\n" +
           swversion + "\n";
    for (int i = 0; i < 2; i++)
      line = line + String(i_values[i]) + "\n";
    for (int i = 0; i < 22; i++)
      line = line + String(b_values[i]) + "\n";
    server.send(200, TEXTPLAIN, line);
    httpquery();
    delay(100);
  });
  // get all measured values in XML format
  server.on("/get/xml", []()
  {
    writetosyslog(34);
    line = "<xml>\n"
           "  <software>\n"
           "    <name>" + MSG[4] + "</name>\n"
           "    <version>" + swversion + "</version>\n"
           "  </software>\n"
           "  <integer>\n";
    for (int i = 0; i < 2; i++)
      line = line + "    <" + I_NAME[i] + ">" + String(i_values[i]) + "</" + I_NAME[i] + ">\n";
    line = line +
           "  </integer>\n"
           "  <bit>\n";
    for (int i = 0; i < 22; i++)
      line = line + "    <" + B_NAME[i] + ">" + String(b_values[i]) + "</" + B_NAME[i] + ">\n";
    line = line +
           "  </bit>\n"
           "</xml>";
    server.send(200, TEXTPLAIN, line);
    httpquery();
    delay(100);
  });
  server.begin();
  Serial.println(MSG[8]);
  Serial.println(MSG[30]);
  beep(1);
}
