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

#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ModbusIP_ESP8266.h>
#include <ModbusRTU.h>
#include <NTPClient.h>
#include <StringSplitter.h> // Note: Change MAX = 5 to MAX = 6 in StringSplitter.h.

//#define       SERIAL_CONSOLE

// settings
const int     COM_SPEED                 = 9600;
const int     MB_UID                    = 1; // slave device UID
const char   *NTPSERVER                 = "europe.pool.ntp.org";
const char   *WIFI_SSID                 = "";
const char   *WIFI_PASSWORD             = "";

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

// name of the Modbus registers
const String  DI_NAME[22]               =
{
  /* 10001 */       "signlight",
  /* 10002 */       "alarm",
  /* 10003 */       "breaker",
  /* 10004 */       "errorntp",
  /* 10005 */       "errormm17d",
  /* 10006 */       "standby",
  /* 10007 */       "hyphae",
  /* 10008 */       "mushroom",
  /* 10009 */       "manual",
  /* 10010 */       "ena_lamp",
  /* 10011 */       "ena_vent",
  /* 10012 */       "ena_heater",
  /* 10013 */       "lamp",
  /* 10014 */       "vent",
  /* 10015 */       "heater",
  /* 10016 */       "rhintless",
  /* 10017 */       "rhintmore",
  /* 10018 */       "tintless",
  /* 10019 */       "tintmore",
  /* 10020 */       "mm17dgreen",
  /* 10021 */       "mm17dyellow",
  /* 10022 */       "mm17dred"
};
const String  IR_NAME[3]                =
{
  /* 30001 */       "mm17drhint",
  /* 30002 */       "mm17dtint",
  /* 30003 */       "mm17dtext"
};
const String  HR_NAME[6]                =
{
  /* 40001-40008 */ "name",
  /* 40009-40011 */ "version",
  /* 40012-40017 */ "mac_address",
  /* 40018-40021 */ "ip_address",
  /* 40022       */ "modbus_uid",
  /* 40023-40028 */ "com_speed"
};
const String  HR100_NAME[29]            =
{
  /* 40101       */ "enable_channels",
  /* 40102       */ "hheater_disable1",
  /* 40103       */ "hheater_disable2",
  /* 40104       */ "hheater_on",
  /* 40105       */ "hheater_off",
  /* 40106       */ "htemperature_min",
  /* 40107       */ "htemperature_max",
  /* 40108       */ "hhumidity_min",
  /* 40109       */ "hhumidity_max",
  /* 40110       */ "mheater_disable1",
  /* 40111       */ "mheater_disable2",
  /* 40112       */ "mvent_disable1",
  /* 40113       */ "mvent_disable2",
  /* 40114       */ "mvent_disablehightemp1",
  /* 40115       */ "mvent_disablehightemp2",
  /* 40116       */ "mvent_disablelowtemp1",
  /* 40117       */ "mvent_disablelowtemp2",
  /* 40118       */ "mvent_hightemp",
  /* 40119       */ "mvent_lowtemp",
  /* 40120       */ "mheater_on",
  /* 40121       */ "mheater_off",
  /* 40122       */ "mtemperature_min",
  /* 40123       */ "mtemperature_max",
  /* 40124       */ "mhumidity_min",
  /* 40125       */ "mhumidity_max",
  /* 40126       */ "mlight_on",
  /* 40127       */ "mlight_off",
  /* 40128       */ "mvent_on",
  /* 40129       */ "mvent_off",
};

// default environment parameters
const uint16_t HR100_DEFVALUES[29]      =
{
  /* 40101       */ 7, // 0b0000000000000111
  /* 40102       */ 0, // 0b0000000000000000
  /* 40103       */ 0, // 0b0000000000000000
  /* 40104       */ 292,
  /* 40105       */ 293,
  /* 40106       */ 290,
  /* 40107       */ 298,
  /* 40108       */ 85,
  /* 40109       */ 95,
  /* 40110       */ 0, // 0b0000000000000000
  /* 40111       */ 0, // 0b0000000000000000
  /* 40112       */ 0, // 0b0000000000000000
  /* 40113       */ 0, // 0b0000000000000000
  /* 40114       */ 0, // 0b0000000000000000
  /* 40115       */ 0, // 0b0000000000000000
  /* 40116       */ 0, // 0b0000000000000000
  /* 40117       */ 0, // 0b0000000000000000
  /* 40118       */ 303,
  /* 40119       */ 288,
  /* 40120       */ 287,
  /* 40121       */ 288,
  /* 40122       */ 286,
  /* 40123       */ 289,
  /* 40124       */ 85,
  /* 40125       */ 95,
  /* 40126       */ 14,
  /* 40127       */ 22,
  /* 40128       */ 15,
  /* 40129       */ 45
};

// other constants
const long    INTERVAL                  = 10000;
const String  SWNAME                    = "MM16D";
const String  SWVERSION                 = "0.1.0";
const String  TEXTHTML                  = "text/html";
const String  TEXTPLAIN                 = "text/plain";
const String  DOCTYPEHTML               = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";

// other variables
int           syslog[64]                = {};
String        line;
String        myipaddress;
String        mymacaddress;
unsigned long prevtime                  = 0;

// messages
const String  MSG[35]                    =
{
  /*  0 */  "",
  /*  1 */  "MM16D * Grow house control device",
  /*  2 */  "Copyright (C) 2023 Pozsar Zsolt",
  /*  3 */  "  software version:       ",
  /*  4 */  "Starting device...",
  /*  5 */  "* Initializing GPIO ports...",
  /*  6 */  "* Connecting to wireless network",
  /*  7 */  "done",
  /*  8 */  "  my MAC address:         ",
  /*  9 */  "  my IP address:          ",
  /* 10 */  "  subnet mask:            ",
  /* 11 */  "  gateway IP address:     ",
  /* 12 */  "* Starting NTP client:    ",
  /* 13 */  "* Starting Modbus/TCP server",
  /* 14 */  "* Starting Modbus/RTU master",
  /* 15 */  "  slave Modbus UID:       ",
  /* 16 */  "  serial port speed:      ",
  /* 17 */  "* Starting webserver",
  /* 18 */  "* Ready, the serial console is off.",
  /* 19 */  "* Modbus query transmitted",
  /* 20 */  "",
  /* 21 */  "  get help page",
  /* 22 */  "  get summary page",
  /* 23 */  "  get log page",
  /* 24 */  "  get all data",
  /* 25 */  "* E01: Cannot update system time!",
  /* 26 */  "* E02: Cannot access MM17D device!",
  /* 27 */  "* E03: No such page!",
  /* 28 */  "* E04: ",
  /* 29 */  "* E05: ",
  /* 30 */  "* W01: Internal humidity is less than requied!",
  /* 31 */  "* W02: Internal humidity is more than requied!",
  /* 32 */  "* W03: Internal temperature is less than requied!",
  /* 33 */  "* W04: Internal temperature is more than requied!",
  /* 34 */  "http://www.pozsarzs.hu"
};
const String  DI_DESC[22]                =
{
  /*  0 */  "sign lights (0/1: red/green)",
  /*  1 */  "alarm",
  /*  2 */  "overcurrent breaker (1: error)",
  /*  3 */  "no connection to the NTP server (1: error)",
  /*  4 */  "no connection to the MM17D (1: error)",
  /*  5 */  "stand-by operation mode",
  /*  6 */  "growing hyphae operation mode",
  /*  7 */  "growing mushroom operation mode",
  /*  8 */  "manual switch (0/1: auto/manual)",
  /*  9 */  "enable lamp output",
  /* 10 */  "enable ventilator output",
  /* 11 */  "enable heater output",
  /* 12 */  "status of the lamp output",
  /* 13 */  "status of the ventilator output",
  /* 14 */  "status of the heater output",
  /* 15 */  "internal humidity is less than requied",
  /* 16 */  "internal humidity is more than requied",
  /* 17 */  "internal temperature is less than requied",
  /* 18 */  "internal temperature is more than requied",
  /* 19 */  "MM17D green LED",
  /* 20 */  "MM17D yellow LED",
  /* 21 */  "MM17D red LED"
};
const String  IR_DESC[3]                =
{
  /*  0 */  "MM17D internal relative humidity in %",
  /*  1 */  "MM17D internal temperature in ",
  /*  2 */  "MM17D external temperature in "
};
const String  HR100_DESC[17]            =
{
  /*  0 */  "enable/disable channels",
  /*  1 */  "time-dependent heating prohibition in h",
  /*  2 */  "heating switch-on temperature in ",
  /*  3 */  "heating switch-off temperature in ",
  /*  4 */  "minimum temperature in ",
  /*  5 */  "maximum temperature in ",
  /*  6 */  "minimum humidity in %",
  /*  7 */  "maximum humidity in %",
  /*  8 */  "time-dependent ventilation prohibition in h",
  /*  9 */  "high ext. temp. and time-dependent ventilation prohibition in h",
  /* 10 */  "low ext. temp. and time-dependent ventilation prohibition in h",
  /* 11 */  "external temperature upper limit in ",
  /* 12 */  "external temperature lower limit in ",
  /* 13 */  "lighting switch-on in h",
  /* 14 */  "lighting switch-off in h",
  /* 15 */  "ventilating switch-on in m",
  /* 16 */  "ventilating switch-off in m"
};

WiFiUDP ntpUDP;
ESP8266WebServer httpserver(80);
ModbusIP mbtcp;
ModbusRTU mbrtu;
NTPClient timeClient(ntpUDP, NTPSERVER, 0, 32767);

// --- CONVERTERS ---
// convert Hex string to byte
byte hs2b(String recv) {
  return strtol(recv.c_str(), NULL, 16);
}

// convert uint16_t to String in binary format
String uint16t2bs(uint16_t myNum, byte NumberOfBits)
{
  String s;
  if (NumberOfBits <= 16)
  {
    myNum = myNum << (16 - NumberOfBits);
    for (int i = 0; i < NumberOfBits; i++)
    {
      if (bitRead(myNum, 15) == 1) s += "1"; else s += "0";
      myNum = myNum << 1;
    }
  }
  return s;
}

// --- SYSTEM LOG ---
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

// --- STATIC MODBUS REGISTERS ---
// fill holding registers with configuration data
void fillholdingregisters()
{
  int    itemCount;
  String s;
  // name: 0-7/40001-40008
  s = SWNAME;
  while (s.length() < 8)
    s = char(0x00) + s;
  for (int i = 0; i < 8; i++)
    mbtcp.Hreg(i, char(s[i]));
  // version: 8-10/40009-40011
  StringSplitter *splitter1 = new StringSplitter(SWVERSION, '.', 3);
  itemCount = splitter1->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter1->getItemAtIndex(i);
    mbtcp.Hreg(8 + i, item.toInt());
  }
  delete splitter1;
  // MAC-address: 11-16/40012-40017
  StringSplitter *splitter2 = new StringSplitter(mymacaddress, ':', 6);
  itemCount = splitter2->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter2->getItemAtIndex(i);
    mbtcp.Hreg(11 + i, hs2b(item));
  }
  delete splitter2;
  // IP-address: 17-20/40018-40021
  StringSplitter *splitter3 = new StringSplitter(myipaddress, '.', 4);
  itemCount = splitter3->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter3->getItemAtIndex(i);
    mbtcp.Hreg(17 + i, item.toInt());
  }
  delete splitter3;
  // MB UID: 21/40022
  mbtcp.Hreg(21, MB_UID);
  // serial speed: 22-27/40023-40028
  s = String(COM_SPEED);
  while (s.length() < 6)
    s = char(0x00) + s;
  for (int i = 0; i < 7; i++)
    mbtcp.Hreg(22 + i, char(s[i]));
  // default environment parameters: 100-128 40101-40129
  for (int i = 0; i < 29; i++) mbtcp.Hreg(100 + i, HR100_DEFVALUES[i]);
}

// --- LEDS AND BUZZER ---
// switch on/off blue LED
void blueled(boolean b)
{
  digitalWrite(PRT_DO_LEDBLUE, b);
}

// switch on/off red LED
void redled(boolean b)
{
  mbtcp.Ists(0, b);
  digitalWrite(PRT_DO_LEDRED, b);
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

// ---  CONTROL ---
// callback
boolean cb(Modbus::ResultCode event, uint16_t transactionId, void* data)
{
  if (event != Modbus::EX_SUCCESS) mbtcp.Hreg(4, true); else mbtcp.Hreg(4, false);
  return true;
}

// read MM17D device
boolean readmm17d()
{
  writetosyslog(19);
  if (! mbrtu.slave())
    mbrtu.pullIreg(MB_UID, 0, 0, 3, cb);
  while (mbrtu.slave())
    mbrtu.task();
  blinkblueled();
  if (! mbrtu.slave())
    mbrtu.pullIsts(MB_UID, 0, 19, 3, cb);
  while (mbrtu.slave())
    mbrtu.task();
  blinkblueled();
  return ! mbtcp.Hreg(4);
}

// read GPIO ports
void getinputs()
{
  for (int i = 5; i < 8; i++) mbtcp.Ists(i, false);
  if (int(analogRead(PRT_AI_OPMODE)) < 100)
  {
    mbtcp.Ists(5, true);
  } else
  {
    if (int(analogRead(PRT_AI_OPMODE)) > 900)
    {
      mbtcp.Ists(7, true);
    } else
    {
      mbtcp.Ists(6, true);
    }
  }
  mbtcp.Ists(1, digitalRead(PRT_DI_ALARM));
  mbtcp.Ists(2, digitalRead(PRT_DI_OCPROT));
  mbtcp.Ists(8, digitalRead(PRT_DI_SWMANU));
}

void analise()
{
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  // enable/disable channels
  for (int i = 0; i < 3; i++)
    if ((mbtcp.Hreg(100) and pow(2, i)) > 0)
      mbtcp.Ists(9 + i, true);
    else
      mbtcp.Ists(9 + i, false);
  // error LED/green-red sign light
  mbtcp.Ists(0, false);
  // alarm, breaker, manual mode
  if ((mbtcp.Ists(1)) or (mbtcp.Ists(2)) or (mbtcp.Ists(8))) mbtcp.Ists(0, true);
  if (! mbtcp.Ists(5))
  {
    if (mbtcp.Ists(6))
    {
      // HYPHAE OPERATION MODE
      // extreme environment parameters
      // - bad humidity
      if (mbtcp.Ireg(0) < mbtcp.Hreg(107)) mbtcp.Ists(15, true); else mbtcp.Ists(15, false);
      if (mbtcp.Ireg(0) > mbtcp.Hreg(108)) mbtcp.Ists(16, true); else mbtcp.Ists(16, false);
      // - bad temperature
      if (mbtcp.Ireg(1) < mbtcp.Hreg(105)) mbtcp.Ists(17, true); else mbtcp.Ists(17, false);
      if (mbtcp.Ireg(1) > mbtcp.Hreg(106)) mbtcp.Ists(18, true); else mbtcp.Ists(18, false);
      // lamp
      mbtcp.Ists(12,  false);
      // ventilator
      mbtcp.Ists(13,  false);
      // heater
      // - switching according to measured parameters
      if (mbtcp.Ireg(1) < mbtcp.Hreg(103)) mbtcp.Ists(14, true);
      if (mbtcp.Ireg(1) > mbtcp.Hreg(104)) mbtcp.Ists(14, false);
      // - timed blocking
      // ---- 40103 ----- ---- 40102 -----
      //         22221111 1111110000000000
      //         32109876 5432109876543210
      // xxxxxxxx00000000 0000000000000000
      if (h < 16)
        mbtcp.Ists(14, ! (mbtcp.Hreg(102) and pow(2, h))); else mbtcp.Ists(14, ! (mbtcp.Hreg(103) and pow(2, h - 16)));
    } else
    {
      if (mbtcp.Ists(8))
      {
        // MUSHROOM OPERATION MODE
        // extreme environment parameters
        // - bad humidity
        if (mbtcp.Ireg(0) < mbtcp.Hreg(123)) mbtcp.Ists(15, true); else mbtcp.Ists(15, false);
        if (mbtcp.Ireg(0) > mbtcp.Hreg(124)) mbtcp.Ists(16, true); else mbtcp.Ists(16, false);
        // - bad temperature
        if (mbtcp.Ireg(1) < mbtcp.Hreg(121)) mbtcp.Ists(17, true); else mbtcp.Ists(17, false);
        if (mbtcp.Ireg(1) > mbtcp.Hreg(122)) mbtcp.Ists(18, true); else mbtcp.Ists(18, false);
        // lamp
        // - timed switching
        if ((h >= mbtcp.Hreg(125)) and (h < mbtcp.Hreg(126))) mbtcp.Ists(12, true); else mbtcp.Ists(12, false);
        // ventilator
        // - timed switching
        if ((m > mbtcp.Hreg(127)) and (m < mbtcp.Hreg(128))) mbtcp.Ists(13, true); else mbtcp.Ists(13, false);
        // - timed and external temperature blocking
        if (h < 16)
          mbtcp.Ists(13, ! (mbtcp.Hreg(111) and pow(2, h))); else mbtcp.Ists(13, ! (mbtcp.Hreg(112) and pow(2, h - 16)));
        if (mbtcp.Ireg(2) > mbtcp.Hreg(117))
          if (h < 16)
              mbtcp.Ists(13, ! (mbtcp.Hreg(113) and pow(2, h))); else mbtcp.Ists(13, ! (mbtcp.Hreg(114) and pow(2, h - 16)));
        if (mbtcp.Ireg(2) < mbtcp.Hreg(118))
          if (h < 16)
              mbtcp.Ists(13, ! (mbtcp.Hreg(115) and pow(2, h))); else mbtcp.Ists(13, ! (mbtcp.Hreg(116) and pow(2, h - 16)));
        // - overriding due to extreme measured parameters
        if ((mbtcp.Ireg(16)) and (mbtcp.Ireg(2)) < mbtcp.Hreg(124)) mbtcp.Ists(14, true);
        if ((mbtcp.Ireg(18)) and (mbtcp.Ireg(2)) < mbtcp.Hreg(122)) mbtcp.Ists(14, true);
        // heater
        if (mbtcp.Ireg(1) < mbtcp.Hreg(119)) mbtcp.Ists(14, true);
        if (mbtcp.Ireg(1) > mbtcp.Hreg(120)) mbtcp.Ists(14, false);
        // - timed blocking
        if (h < 16)
          mbtcp.Ists(14, ! (mbtcp.Hreg(109) and pow(2, h))); else mbtcp.Ists(14, ! (mbtcp.Hreg(110) and pow(2, h - 16)));
      }
    }
  } else
  {
    // STAND-BY OPERATION MODE
    for (int i = 12; i < 15; i++) mbtcp.Ists(i, false);
  }
  for (int i = 15; i < 19; i++)
    if (mbtcp.Ists(i)) writetosyslog(i + 15);
}

// write GPIO ports
void setoutputs()
{
  digitalWrite(PRT_DO_LEDRED, mbtcp.Ists(0));
  // enable/disable outputs
  for (int i = 9; i < 12; i++)
    if (! mbtcp.Ists(i)) mbtcp.Ists(i + 3, false);
  digitalWrite(PRT_DO_LAMP, mbtcp.Ists(12));
  digitalWrite(PRT_DO_VENT, mbtcp.Ists(13));
  digitalWrite(PRT_DO_HEAT, mbtcp.Ists(14));
}

// --- WEBPAGES ---
// error 404 page
void handleNotFound()
{
  Serial.println(SWVERSION);
  writetosyslog(27);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | Error 404</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n"
         "    " + MSG[8] + mymacaddress + "<br>\n"
         "    " + MSG[9] + myipaddress + "<br>\n"
         "    " + MSG[3] + "v" + SWVERSION + "<br>\n"
         "    " + MSG[15] + String(MB_UID) + "<br>\n"
         "    " + MSG[16] + String(COM_SPEED) + "<br>\n"
         "    <hr>\n"
         "    <h3>ERROR 404!</h3>\n"
         "    No such page!\n"
         "    <br>"
         "    <div align=\"right\"><a href=\"/\">back</a></div>"
         "    <br>\n"
         "    <hr>\n"
         "    <center>" + MSG[2] + " <a href=\"" + MSG[34] + "\">" + MSG[34] + "</a></center>\n"
         "    <br>\n"
         "  </body>\n"
         "</html>\n";
  httpserver.send(404, TEXTHTML, line);

  delay(100);
}

// help page
void handleHelp()
{
  String s;
  writetosyslog(21);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | Help</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n"
         "    " + MSG[8] + mymacaddress + "<br>\n"
         "    " + MSG[9] + myipaddress + "<br>\n"
         "    " + MSG[3] + "v" + SWVERSION + "<br>\n"
         "    " + MSG[15] + String(MB_UID) + "<br>\n"
         "    " + MSG[16] + String(COM_SPEED) + "<br>\n"
         "    <hr>\n"
         "    <h3>Information and data access</h3>\n"
         "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>Information pages</b></td></tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/\">http://" + myipaddress + "/</a></td>\n"
         "        <td>help</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/summary\">http://" + myipaddress + "/summary</a></td>\n"
         "        <td>summary page</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/log\">http://" + myipaddress + "/log</a></td>\n"
         "        <td>log</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>Data access with HTTP</b></td>\n"
         "      <tr>\n"
         "        <td>\n"
         "          <a href=\"http://" + myipaddress + "/get/csv\">http://" + myipaddress + "/get/csv</a>"
         "        </td>\n"
         "        <td>all measured values and status in CSV format</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/json\">http://" + myipaddress + "/get/json</a></td>\n"
         "        <td>all measured values and status in JSON format</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/txt\">http://" + myipaddress + "/get/txt</a></td>\n"
         "        <td>all measured values and status in TXT format</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/xml\">http://" + myipaddress + "/get/xml</a></td>\n"
         "        <td>all measured values and status in XML format</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>Data access with Modbus</b></td>\n"
         "      <tr><td colspan=\"3\"><i>Status: (read-only)</i></td>\n";
  for (int i = 0; i < 22; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 10001) + "</td>\n"
      "        <td>" + DI_DESC[i] + "</td>\n"
      "        <td>bit</td>\n"
      "      </tr>\n";
  }
  line += "      <tr><td colspan=\"3\"><i>Measured values: (read-only)</i></td>\n";
  for (int i = 0; i < 3; i++)
  {
    s = IR_DESC[i];
    if (i > 0) s += "K";
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 30001) + "</td>\n"
      "        <td>" + s + "</td>\n"
      "        <td>uint16</td>\n"
      "      </tr>\n";
  }
  line += "      <tr><td colspan=\"3\"><i>Software version: (read-only)</i></td>\n"
          "      <tr>\n"
          "        <td>40001-40008</td>\n"
          "        <td>device name</td>\n"
          "        <td>8 char</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40009-40011</td>\n"
          "        <td>software version</td>\n"
          "        <td>3 byte</td>\n"
          "      </tr>\n"
          "      <tr><td colspan=\"3\"><i>Network settings: (read-only)</i></td>\n"
          "      <tr>\n"
          "        <td>40012-40017</td>\n"
          "        <td>MAC address</td>\n"
          "        <td>6 byte</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40018-40021</td>\n"
          "        <td>IP address</td>\n"
          "        <td>4 byte</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40022</td>\n"
          "        <td>Modbus UID</td>\n"
          "        <td>1 byte</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40023-40028</td>\n"
          "        <td>serial port speed</td>\n"
          "        <td>6 char</td>\n"
          "      </tr>\n"
          "      <tr><td colspan=\"3\"><i>Settings: (read/write)</i></td>\n"
          "      <tr>\n"
          "        <td>40101</td>\n"
          "        <td>" + HR100_DESC[0] + "</td>\n"
          "        <td>3 bit</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40102-40103</td>\n"
          "        <td>hyphae - " + HR100_DESC[1] + "</td>\n"
          "        <td>24 bit</td>\n"
          "      </tr>\n";
  for (int i = 0; i < 6; i++)
  {
    s = HR100_DESC[i + 2];
    if (i < 4 ) s += "K";
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 40104) + "</td>\n"
      "        <td>hyphae - " + s + "</td>\n"
      "        <td>uint16</td>\n"
      "      </tr>\n";
  }
  line +=
    "      <tr>\n"
    "        <td>40110-40111</td>\n"
    "        <td>mushroom - " + HR100_DESC[1] + "</td>\n"
    "        <td>uint16</td>\n"
    "      </tr>\n";
  for (int i = 0; i < 3; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + String(2 * i + 40112) + "-" + String(2 * i + 40113) + "</td>\n"
      "        <td>mushroom -  " + HR100_DESC[i + 8] + "</td>\n"
      "        <td>24 bit</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 2; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 40118) + "</td>\n"
      "        <td>" + HR100_DESC[i + 11] + "K </td>\n"
      "        <td>uint16</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 6; i++)
  {
    s = HR100_DESC[i + 2];
    if (i < 4 ) s += "K";
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 40120) + "</td>\n"
      "        <td>mushroom - " + s + "</td>\n"
      "        <td>uint16</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 4; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 40126) + "</td>\n"
      "        <td>mushroom - " + HR100_DESC[i + 13] + "</td>\n"
      "        <td>uint16</td>\n"
      "      </tr>\n";
  }
  line += "    </table>\n"
          "    <br>\n"
          "    <hr>\n"
          "    <center>" + MSG[2] + " <a href=\"" + MSG[34] + "\">" + MSG[34] + "</a></center>\n"
          "    <br>\n"
          "  </body>\n"
          "</html>\n";
  httpserver.send(200, TEXTHTML, line);

  delay(100);
}

// summary page
void handleSummary()
{
  const int CC = 273;
  String s;
  int ii;
  writetosyslog(22);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | Summary</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n"
         "    " + MSG[8] + mymacaddress + "<br>\n"
         "    " + MSG[9] + myipaddress + "<br>\n"
         "    " + MSG[3] + "v" + SWVERSION + "<br>\n"
         "    " + MSG[15] + String(MB_UID) + "<br>\n"
         "    " + MSG[16] + String(COM_SPEED) + "<br>\n"
         "    <hr>\n"
         "    <h3>All settings</h3>\n"
         "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
         "      <tr>\n"
         "        <td>" + HR100_DESC[0] + "</td>\n"
         "        <td align=\"right\">" + uint16t2bs(mbtcp.Hreg(100), 3) + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td>hyphae - " + HR100_DESC[1] + "</td>\n"
         "        <td align=\"right\">" + uint16t2bs(mbtcp.Hreg(101), 8) + uint16t2bs(mbtcp.Hreg(102), 16) + "</td>\n"
         "      </tr>\n";
  for (int i = 0; i < 6; i++)
  {
    s = HR100_DESC[i + 2];
    ii = mbtcp.Hreg(i + 103);
    if (i < 4 )
    {
      s += "&deg;C";
      ii -= CC;
    }
    line +=
      "      <tr>\n"
      "        <td>hyphae - " + s + "</td>\n"
      "        <td align=\"right\">" + String(ii) + "</td>\n"
      "      </tr>\n";
  }
  line +=
    "      <tr>\n"
    "        <td>mushroom -  " + HR100_DESC[1] + "</td>\n"
    "        <td align=\"right\">" + uint16t2bs(mbtcp.Hreg(109), 8) + uint16t2bs(mbtcp.Hreg(110), 16) + "</td>\n"
    "      </tr>\n";
  for (int i = 0; i < 3; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>mushroom -  " + HR100_DESC[i + 8] + "</td>\n"
      "        <td align=\"right\">" + uint16t2bs(mbtcp.Hreg(i + 111), 8) + uint16t2bs(mbtcp.Hreg(2 * i + 112), 16) + "</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 2; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + HR100_DESC[i + 11] + "&deg;C </td>\n"
      "        <td align=\"right\">" + String(mbtcp.Hreg(i + 117) - CC) + "</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 6; i++)
  {
    s = HR100_DESC[i + 2];
    ii = mbtcp.Hreg(i + 119);
    if (i < 4 )
    {
      s += "&deg;C";
      ii -= CC;
    }
    line +=
      "      <tr>\n"
      "        <td>mushroom - " + s + "</td>\n"
      "        <td align=\"right\">" + String(ii) + "</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 4; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>mushroom - " + HR100_DESC[i + 13] + "</td>\n"
      "        <td align=\"right\">" + String(mbtcp.Hreg(i + 125)) + "</td>\n"
      "      </tr>\n";
  }
  line +=
    "    </table>\n"
    "    <br>\n"
    "    <h3>All measured values and status</h3>\n"
    "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
    "      <tr>\n"
    "        <td>system time</td>\n"
    "        <td align=\"right\">" + timeClient.getFormattedTime() + "</td>\n"
    "      </tr>\n";
  for (int i = 0; i < 3; i++)
  {
    s = IR_DESC[i];
    ii = mbtcp.Ireg(i);
    if (i > 0 )
    {
      s += "&deg;C";
      ii -= CC;
    }

    line +=
      "      <tr>\n"
      "        <td>" + s + "</td>\n"
      "        <td align=\"right\">" + String(ii) + "</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 22; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + DI_DESC[i] + "</td>\n"
      "        <td align=\"right\">" + String(mbtcp.Ists(i)) + "</td>\n"
      "      </tr>\n";
  }
  line +=
    "    </table>\n"
    "    <br>\n"
    "    <hr>\n"
    "    <div align=\"right\"><a href=\"/\">back</a></div>"
    "    <br>\n"
    "    <center>" + MSG[2] + " <a href=\"" + MSG[34] + "\">" + MSG[34] + "</a></center>\n"
    "    <br>\n"
    "  </body>\n"
    "</html>\n";
  httpserver.send(200, TEXTHTML, line);

  delay(100);
}

// log page
void handleLog()
{
  writetosyslog(23);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | Log</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n"
         "    " + MSG[8] + mymacaddress + "<br>\n"
         "    " + MSG[9] + myipaddress + "<br>\n"
         "    " + MSG[3] + "v" + SWVERSION + "<br>\n"
         "    " + MSG[15] + String(MB_UID) + "<br>\n"
         "    " + MSG[16] + String(COM_SPEED) + "<br>\n"
         "    <hr>\n"
         "    <h3>Last 64 lines of system log:</h3>\n"
         "    <table border=\"0\" cellpadding=\"3\" cellspacing=\"0\">\n";
  for (int i = 0; i < 64; i++)
    if (syslog[i] > 0)
      line += "      <tr><td align=right><b>" + String(i) + "</b></td><td>" + MSG[syslog[i]] + "</td></tr>\n";
  line +=
    "    </table>\n"
    "    <br>\n"
    "    <hr>\n"
    "    <div align=\"right\"><a href=\"/\">back</a></div>"
    "    <br>\n"
    "    <center>" + MSG[2] + " <a href=\"" + MSG[34] + "\">" + MSG[34] + "</a></center>\n"
    "    <br>\n"
    "  </body>\n"
    "</html>\n";
  httpserver.send(200, TEXTHTML, line);

  delay(100);
}

// get all measured data in CSV format
void handleGetCSV()
{
  writetosyslog(24);
  line = "\"" + HR_NAME[0] + "\",\"" + SWNAME + "\"\n"
         "\"" + HR_NAME[1] + "\",\"" + SWVERSION + "\"\n"
         "\"" + HR_NAME[2] + "\",\"" + mymacaddress + "\"\n"
         "\"" + HR_NAME[3] + "\",\"" + myipaddress + "\"\n"
         "\"" + HR_NAME[4] + "\",\"" + String(MB_UID) + "\"\n"
         "\"" + HR_NAME[5] + "\",\"" + String(COM_SPEED) + "\"\n";
  for (int i = 0; i < 29; i++)
    line += "\"" + HR100_NAME[i] + "\",\"" + String(mbtcp.Hreg(i + 100)) + "\"\n";
  for (int i = 0; i < 3; i++)
    line += "\"" + IR_NAME[i] + "\",\"" + String(mbtcp.Ireg(i)) + "\"\n";
  for (int i = 0; i < 22; i++)
    line += "\"" + DI_NAME[i] + "\",\"" + String(mbtcp.Ists(i)) + "\"\n";
  httpserver.send(200, TEXTPLAIN, line);

  delay(100);
}

// get all measured values in JSON format
void handleGetJSON()
{
  writetosyslog(24);
  line = "{\n"
         "  \"software\": {\n"
         "    \"" + HR_NAME[0] + "\": \"" + SWNAME + "\",\n"
         "    \"" + HR_NAME[1] + "\": \"" + SWVERSION + "\"\n"
         "  },\n"
         "  \"hardware\": {\n"
         "    \"" + HR_NAME[2] + "\": \"" + mymacaddress + "\",\n"
         "    \"" + HR_NAME[3] + "\": \"" + myipaddress + "\",\n"
         "    \"" + HR_NAME[4] + "\": \"" + String(MB_UID) + "\",\n"
         "    \"" + HR_NAME[5] + "\": \"" + String(COM_SPEED) + "\"\n"
         "  },\n"
         "  \"settings\": {\n"
         "    \"integer\": {\n";
  for (int i = 0; i < 29; i++)
  {
    line += "      \"" + HR100_NAME[i] + "\": \"" + String(mbtcp.Hreg(i + 100));
    if (i < 28 ) line += "\",\n"; else  line += "\"\n";
  }
  line +=
    "    }\n"
    "  },\n"
    "  \"data\": {\n"
    "    \"integer\": {\n";
  for (int i = 0; i < 3; i++)
  {
    line += "      \"" + IR_NAME[i] + "\": \"" + String(mbtcp.Ireg(i));
    if (i < 2 ) line += "\",\n"; else  line += "\"\n";
  }
  line +=
    "    },\n"
    "    \"bit\": {\n";
  for (int i = 0; i < 22; i++)
  {
    line += "      \"" + DI_NAME[i] + "\": \"" + String(mbtcp.Ists(i));
    if (i < 21 ) line += "\",\n"; else  line += "\"\n";
  }
  line +=
    "    }\n"
    "  }\n"
    "}\n";
  httpserver.send(200, TEXTPLAIN, line);

  delay(100);
}

// get all measured data in TXT format
void handleGetTXT()
{
  writetosyslog(24);
  line = SWNAME + "\n" +
         SWVERSION + "\n" +
         mymacaddress + "\n" +
         myipaddress + "\n" + \
         String(MB_UID) + "\n" + \
         String(COM_SPEED) + "\n";
  for (int i = 0; i < 29; i++)
    line += String(mbtcp.Hreg(i + 100)) + "\n";
  for (int i = 0; i < 3; i++)
    line += String(mbtcp.Ireg(i)) + "\n";
  for (int i = 0; i < 22; i++)
    line += String(mbtcp.Ists(i)) + "\n";
  httpserver.send(200, TEXTPLAIN, line);

  delay(100);
}

// get all measured values in XML format
void handleGetXML()
{
  writetosyslog(24);
  line = "<xml>\n"
         "  <software>\n"
         "    <" + HR_NAME[0] + ">" + SWNAME + "</" + HR_NAME[0] + ">\n"
         "    <" + HR_NAME[1] + ">" + SWVERSION + "</" + HR_NAME[1] + ">\n"
         "  </software>\n"
         "  <hardware>\n"
         "    <" + HR_NAME[2] + ">" + mymacaddress + "</" + HR_NAME[2] + ">\n"
         "    <" + HR_NAME[3] + ">" + myipaddress + "</" + HR_NAME[3] + ">\n"
         "    <" + HR_NAME[4] + ">" + String(MB_UID) + "</" + HR_NAME[4] + ">\n"
         "    <" + HR_NAME[5] + ">" + String(COM_SPEED) + "</" + HR_NAME[5] + ">\n"
         "  </hardware>\n"
         "  <settings>\n"
         "    <integer>\n";
  for (int i = 0; i < 29; i++)
    line += "        <" + HR100_NAME[i] + ">" + String(mbtcp.Hreg(i + 100)) + "</" + HR100_NAME[i] + ">\n";
  line += "    </integer>\n"
          "  </settings>\n"
          "  <data>\n"
          "    <integer>\n";
  for (int i = 0; i < 3; i++)
    line += "      <" + IR_NAME[i] + ">" + String(mbtcp.Ireg(i)) + "</" + IR_NAME[i] + ">\n";
  line +=
    "    </integer>\n"
    "    <bit>\n";
  for (int i = 0; i < 22; i++)
    line += "      <" + DI_NAME[i] + ">" + String(mbtcp.Ists(i)) + "</" + DI_NAME[i] + ">\n";
  line +=
    "    </bit>\n"
    "  </data>\n"
    " </xml> ";
  httpserver.send(200, TEXTPLAIN, line);

  delay(100);
}

// --- MAIN ---
// initializing function
void setup(void)
{
  // set serial port
  Serial.begin(COM_SPEED, SERIAL_8N1);
  // write program information
#ifdef SERIAL_CONSOLE
  Serial.println("");
  Serial.println("");
  Serial.println(MSG[1]);
  Serial.println(MSG[2]);
  Serial.println(MSG[3] + "v" + SWVERSION );
#endif
  writetosyslog(4);
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[4]);
#endif
  // initialize GPIO ports
  writetosyslog(5);
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[5]);
#endif
  pinMode(PRT_DO_BUZZER, OUTPUT);
  pinMode(PRT_DO_LEDBLUE, OUTPUT);
  pinMode(PRT_DO_LEDRED, OUTPUT);
  digitalWrite(PRT_DO_LEDBLUE, LOW);
  digitalWrite(PRT_DO_LEDRED, LOW);
  // connect to wireless network
  writetosyslog(6);
#ifdef SERIAL_CONSOLE
  Serial.print(MSG[6]);
#endif
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    blinkallleds();
#ifdef SERIAL_CONSOLE
    Serial.print(".");
#endif
  }
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[7]);
#endif
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  myipaddress = WiFi.localIP().toString();
  mymacaddress = WiFi.macAddress();
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[8] + mymacaddress);
  Serial.println(MSG[9] + myipaddress);
  Serial.println(MSG[10] + WiFi.subnetMask().toString());
  Serial.println(MSG[11] + WiFi.gatewayIP().toString());
#endif
  // start NTP client
  writetosyslog(12);
  timeClient.begin();
  timeClient.update();
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[12] + String(h) + ": " + String(m));
#endif
  // start Modbus/TCP server
  writetosyslog(13);
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[13]);
#endif
  mbtcp.server();
  // start Modbus/RTU master
  writetosyslog(14);
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[14]);
  Serial.println(MSG[15] + String(MB_UID));
  Serial.println(MSG[16] + String(COM_SPEED));
#endif
  mbrtu.begin(&Serial);
  mbrtu.setBaudrate(COM_SPEED);
  mbrtu.master();
  // set Modbus registers
  mbtcp.addIsts(0, false, 22);
  mbtcp.addIreg(0, 0, 3);
  mbtcp.addHreg(0, 0, 28);
  mbtcp.addHreg(100, 0, 29);
  fillholdingregisters();
  // start webserver
  writetosyslog(17);
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[17]);
#endif
  httpserver.onNotFound(handleNotFound);
  httpserver.on("/", handleHelp);
  httpserver.on("/summary", handleSummary);
  httpserver.on("/log", handleLog);
  httpserver.on("/get/csv", handleGetCSV);
  httpserver.on("/get/json", handleGetJSON);
  httpserver.on("/get/txt", handleGetTXT);
  httpserver.on("/get/xml", handleGetXML);
  httpserver.begin();
#ifdef SERIAL_CONSOLE
  Serial.println(MSG[18]);
#endif
  beep(1);
}

// loop function
void loop(void)
{
  httpserver.handleClient();
  unsigned long currtime = millis();
  if (currtime - prevtime >= INTERVAL)
  {
    prevtime = currtime;
    timeClient.update();
    mbtcp.Ists(3, ! timeClient.isTimeSet());
    if (mbtcp.Ists(3)) writetosyslog(25);
    mbtcp.Ists(4, ! readmm17d());
    if (mbtcp.Ists(4)) writetosyslog(26);
    getinputs();
    analise();
    setoutputs();
  }
  mbtcp.task();
  yield();
}
