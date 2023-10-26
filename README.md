## MM16D * Grow house control device
Copyright (C) 2023 Pozsár Zsolt <pozsarzs@gmail.com>  
Homepage: <http://www.pozsarzs.hu>  
GitHub: <https://github.com/pozsarzs/mm16d-sw>

#### Hardware
ESP8266 Huzzah Feather microcontroller

#### Software
|features              |                                             |
|:---------------------|---------------------------------------------|
|architecture          |xtensa                                       |
|operation system      |none                                         |
|version               |v0.1                                         |
|language              |en                                           |
|licence               |EUPL v1.2                                    |
|local user interface  |none                                         |
|remote user interface |RS-232 TTL: serial console                   |
|                      |WLAN: web interface                          |
|remote data query     |RS-232 TTL: Modbus/RTU                       |
|remote data access    |WLAN: HTTP (CSV, JSON, TXT, XML), Modbus/TCP |

#### External libraries in the package
 - [Modbus-ESP8266](https://github.com/emelianov/modbus-esp8266) library v4.1.0 by Andre Sarmento Barbosa, Alexander Emelianov
 - [ESP8266WebServer](https://github.com/esp8266/Arduino) library v1.0 by Ivan Grokhotkov
 - [ESP8266WiFi](https://github.com/esp8266/Arduino) library v1.0 by Ivan Grokhotkov
 - [NTPClient](https://github.com/arduino-libraries/NTPClient) library v3.2.1 by Fabrice Weinberg
