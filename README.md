# ParadoxSerialToTelnet_Esp8266

This document the serial to wifi (telnet) hardware setup for paradox SP6000 control panel using ESP8266 devkit module; Allowing the alarm panel to interface with Paradox-Alarm-Interface (PAI) via serial over IP. Thus enable wifi connection of SP6000 to Home Assistant.

Note: The serial to wifi code is generic, can be used for other application to telnet remotely to the serial ports.

Ref: https://github.com/ParadoxAlarmInterface/pai/


## SP6000 Panel Serial
- RX TX is 5v level
- Baud Rate is 9600
- AUX+ is ~13v

## Parts 
- Level converter module 
- Step down to 5v conveter (to power everyting from AUX)
- ESP8266 kit (logic level is 3v3)

```
┌───────┐               
│ Tx   ┌╵   == > HV1   [level converter] LV1    == > RX   [ESP8266]
│ Rx   │    == > HV2   [level converter] LV1    == > TX   [ESP8266]
│ GND  │    == > IN(-) [step down 5v]    OUT(-) == > HGND [level converter] & GND [ESP8266] GND == > LGND [level converter]
│ AUX+ └╷   == > IN(+) [step down 5v]    OUT(+) == > HV   [level converter] & VIN [ESP8266] 3v3 == > LV   [level converter]
└───────┘
```

### **Refer prototype folder for prototype daughter board use to connect all the parts


## PAI configuration yaml
```
CONNECTION_TYPE: 'IP'
IP_CONNECTION_HOST: '<IP Address>'
IP_CONNECTION_PORT: 23
IP_CONNECTION_PASSWORD: 'paradox' # IP Module password. "paradox" is default.
IP_CONNECTION_BARE: True
PASSWORD: '0000'
```

## Paradox password gotcha
- default likely to be '0000' or ''
- however, if '0000' dont work, try 'aaaa'; this is due to encoding. 
- password can be set via keypad programming mode; steps: (Enter) -> (installer code> -> (911) -> set new code

## ESP8266 wifi problem connecting to certain wifi router 
- router that shared same ssid for both 2.4ghz and 5ghz
- router that uses newer than b/g/n standard like AC/AX (some setting affecting the esp8266). changing the setting affect router peak performance.)
- EG: some assus router
- fixes - use different wifi AP for iot devices (tested work)
        - on problemmatic asus, filter esp mac address for wifi 5ghz, ESP set to use static ip