# SigFox Arduino Sensor   

A low power Arduino with a BME280 sensor that sends data over SigFox and finally push the data to an MQTT server  

## Getting Started

### Prerequisites

- FTDI adapter
- 3.3V Arduino Pro Mini (modified)
- Pololu 3.3V Step-Up/Step-Down Voltage regulator (S7V8F3)
- battery holder. I used a holder for two LR20 batteries
- SigFox Modem (Wisol) https://arduino-shop.eu/arduino-platform/1584-iot-lpwan-sigfox-node-uart-modem-868mhz-1493197325.html
- a place to run a phyton3 script
- access to a MQTT server 

### Installing

i) modify your Arduino Pro Mini according to this instruction http://www.home-automation-community.com/arduino-low-power-how-to-run-atmega328p-for-a-year-on-coin-cell-battery/

ii) wire the Step-Up/Step-Down converter to the battery and Arduino pins + add the SigFox modem

iii) transfer the sketch with the FTDI adapter from your PC to the Arduino and open the serial console on your PC

iv)  use the modem ID, PAC to register at SigFox https://backend.sigfox.com/auth/login

v) after that login to Sigfox and set up the message forwarding by clicking on "Device Type" -> on "Name" in the list (last column) -> callbacks -> new

vi) add the message forwarding (callback). The images shows my setting which work with the python script. 
	https://1drv.ms/i/s!AoRZPGhUhShFjN4a9s6Ksq1HG2tkHg

vii) modify the python script and add the data for your mail provider and MQTT server. Add it to cron so it runs every 5 minutes
	*/5 * * * * /usr/bin/python3 /home/thomas/sigfox/sigfox.py
	
viii) you probably want to change the hard coded MQTT channels in line 46-49
	
that's it


## Python Script

The python script is a very dirty hack but works now here since a week. I am not a programmer and this was my frist python program. Feel free to improve it or avoid it by using GET/POST SigFox callbacks
