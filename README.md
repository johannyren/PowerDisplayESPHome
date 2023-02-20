# PowerDisplayESPHome

This is a small display that shows the current electricity consumption, together with a graph of the today's electricity price, using either NordPool or Tibber. The software pulls the data from a Home Assistant instance, so all sources must be available there.

This is a port of the previous repo [PowerDisplayHomeAssistant](https://github.com/johannyren/PowerDisplayHomeAssistant) to ESPHome. The ESPHome version allows for much more flexibility and ease of setup than the previous version, and gets its time directly from Home Assistant rather than ntp. This simplifies handling of Daylight Savings Time etc. It also allows for more fonts, as it uses Google fonts that supports non-English characters, and can be easily replaced as desired.

**Note:** The ESPHome version requires an ESP32 microcontroller, as the ESPHome ILI9341 library seems to require more memory than is available in a Wemos D1 mini.

<p align="center">
  <img src="(https://github.com/johannyren/PowerDisplayHomeAssistant/blob/main/Images/Display1.jpg?raw=true)" width="350" title="Display">
</p>

![alt text](https://github.com/johannyren/PowerDisplayESPHome/blob/main/images/Display1.jpg?raw=true)

The hardware consists of an ESP32 and an ILI9341 display.

## Wiring of the ILI9341:

```
ILI9341   -> ESP32
VCC       -> 3.3V
GND       -> GND
CS        -> 13
RESET     -> 33
D/C       -> 14
SDI(MOSI) -> 27
SCK       -> 26
LED       -> 25
```
<p align="left">
  <img src="(https://github.com/johannyren/PowerDisplayESPHome/blob/main/images/Wiring_ILI9341.jpg?raw=true)" title="ILI9341">
</p>

![alt text](https://github.com/johannyren/PowerDisplayESPHome/blob/main/images/Wiring_ILI9341.jpg?raw=true)


![alt text](https://github.com/johannyren/PowerDisplayESPHome/blob/main/images/Wiring_ESP32.jpg?raw=true)

## Files required

Copy the following files into your Home Assistant ESPHome folder, for example /homeassistant/config/esphome:

```
power_display.h
electrical_tower32.png
solar_energy32.png
```
Use the file power-display-esphome.yaml to create your ESPHome entity.

## Datasources and connection to Home Assistant
Data sources from Home Assistant are defined in power-display-esphome.yaml

It will require the HomeAssistant integration with NordPool (https://github.com/custom-components/nordpool), as well as a device that can read the current power usage from the power meter. 

### Popular ESPHome implementations are:

- Home Assistant Glow pulse counter. (https://klaasnicolaas.github.io/home-assistant-glow/)
- ESPHome HAN port reader  (https://github.com/psvanstrom/esphome-p1reader)

The implementation also expects a Utility Meter entity in Home Assistant. The following is an example to put in configuration.yaml:
```
 utility_meter:
 produktion_huset_per_dag:
   source: sensor.cumulative_active_export
   cycle: daily
```

<!--
## Casing
STL files are available for 3D printing a casing for the display. Two variants - one straight for putting on a wall, and one tilted, optimal for a desktop display.
-->

## Known issue
ESPHome has a tendency to reboot whenever it loses WiFi connection, which seems to happen often on an ESP32. According to ESPHome documentation, this is a known issue with ESP32 devices. This will cause the display to blink every 15 minutes or so. The current values are saved to NVM (Non Volatile Memory) every minute, so that it can recover very quickly after reboot. 

    
