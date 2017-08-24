
// RocketScream's LowPower Library for putting the device into sleep
// https://github.com/rocketscream/Low-Power
#include <LowPower.h>

// standard SoftSerial library is not reliable to read from the SigFox module due to low speed
#include <NeoSWSerial.h>

//Library to use I2C devices
#include <Wire.h>

//following two libraries are needed for the BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// number of seconds between two transmits. Keep in mind max is 140 uplink & 4 downlink messages per day
#define TX_Interval 900

// PINs used to connect to the WISOL breakout board 
#define RxPin 10
#define TxPin 11

//if you want debug info on serial console
#define DEBUG true

//I2C address of the BME280 can be 0x77 or 0x76
#define BMEaddress 0x76

// software serial connection PINs
NeoSWSerial WISOL(RxPin,TxPin); // RX, TX

//construct BME280 object
Adafruit_BME280 bme;

// counts the number of messages that have been sent
unsigned int uplinkCounter = 0x00;

char lpp[12]; // Array to convert sensor data 4 integer values with 4 digits each 

// used for response from the Sigfox device
// reading byte/char by byte/char into char 'c' 
// and combining the repsonse to String 'response'
char c;
String response;

//function to print BME readings to serial
void printSensorValues();

void create_lpp();


void printStars()
{
	Serial.println F("**********************************");
}

void checkResponse()
{
  //variable to combine the single chars from the response
	response = "";
  
  //wait a while as long as the Sigfox modem has nothing to tell us
	while(WISOL.available()==0);
  
  //if it has something to tell, listen byte/char by byte/char and add it to the result string
	while(WISOL.available()>0)
	{
		delay(5);
		c = WISOL.read();
		response = response + c;
	}
	//let's see what we have received
	if (DEBUG) Serial.println("Response received : " + response);

  //we are interested if the modem responded OK, so the first two chars
	response = response.substring(0,2);
}

// Check if there is a valid response from the SigFox modem
// It should respond with 'OK' if you send an 'AT' + \n
void checkTRX()
{
	response = "";
	//c=0x00;
	while(WISOL.available()>0)
	{
		WISOL.read();
		delay(5);
	}
	while(response.substring(0,2) !="OK")
	{
		WISOL.println("AT");
		delay(5);
		response = "";
		while(WISOL.available()>0)
		{
			delay(5);
			c = WISOL.read();
			response = response + c;
		}
		//Serial.println("WISOL response:");
		//Serial.print(response);
	}
}

void getID()
{
  // x = (val == 10) ? 20 : 15;
  if (DEBUG) printStars();
	WISOL.println("AT$I=10");
	delay(5);
	response = "";
	while(WISOL.available()>0)
	{
		delay(5);
		c = WISOL.read();
		response = response + c;
	}
	Serial.print("ID:" + response);
	WISOL.println("AT$I=11");
	delay(5);
	response = "";
	while(WISOL.available()>0)
	{
		delay(5);
		c = WISOL.read();
		response = response + c;
	}
	Serial.print("PAC:" + response);
	printStars();
}

void getVersion()
{
	WISOL.println("AT$I=0");
	delay(5);
	response = "";
	while(WISOL.available()>0)
	{
		delay(5);
		c = WISOL.read();
		response = response + c;
	}
	Serial.print("TRX version:" + response);
}

void send()
{
	Serial.println("Sending payload ...");
  response="";
	
	//AT command for sending payload followed by a println (i.e. \n) to submit
	WISOL.print("AT$SF=");
	WISOL.println(lpp);
  Serial.println(lpp);

  //wait a second....
	delay(1000);
  
	//let's check what the response from the modem was
	checkResponse();

  //it should be 'OK'...anything else would be bad
	if (response == "OK") Serial.println("Payload sent!");
	else Serial.println("Sending Error!");
}

// receiver is set to deepsleep after transmission and needs to wake up before
void transceiverWakeUp()
{
	checkTRX();
	Serial.println("TRX ready!");
}

void setup()
{
  //adding a delay to prevent device from sending immediately after pwoering on ...in case we want to upload new sketch
  if (DEBUG) Serial.println("waiting 5 seconds before starting");
  delay(5000);
  
  //open serial communication to serial communication for debugging and to the WISOL sigfox modem
  if (DEBUG) Serial.begin(115200);
  WISOL.begin(9600);
  
  pinMode(TxPin,OUTPUT);

  //software reset
  if (DEBUG) Serial.println ("Resetting SIgfox modem");
  WISOL.println("AT$P=0");
  // if (DEBUG) checkResponse();
  
  delay(10);
  
  printStars();
  if (DEBUG) Serial.println F("********SIGFOX WISOL Frame********");
  
  // check if Sigfox modem responds to "AT" with an "OK"
  checkTRX();
  if (DEBUG) Serial.println("WISOL detected!");
  
  //getting Sigfox modem ID, PAC and device name + version and putting it to serial
  if (DEBUG) getVersion();
  if (DEBUG) getID();
  
  response = "";

  if (! bme.begin(BMEaddress)) {
    if (DEBUG) Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  if (DEBUG) Serial.println("forced mode, 1x temperature / 1x humidity / 1x pressure oversampling,");
  if (DEBUG) Serial.println("filter off");
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );
}

void loop()
{
	if (DEBUG) Serial.println F("*********Periodic mode************");
	
	//the PINs might be Input before as they are set as Input at the end of this loop. Change that for the Tx Pin
	pinMode(TxPin,OUTPUT);

    //force BME to make one measurement
    bme.takeForcedMeasurement();
  	
	//wake the transceiver up
	transceiverWakeUp();

    //read sensor data and convert it to a low power/low byte payload
    create_lpp();
	
	//Sending a message and increment the uplink counter
	send();

    //read sensor data and print it to serial
    if (DEBUG) printSensorValues();
	
	//counts the number of messages sent
	uplinkCounter++;
	
	if (DEBUG) Serial.println("Uplink counter:" + String(uplinkCounter));
	
	/* set SigFox transceiver power mode to 
	* 			0=Software reset (settings will be reset to values in flash)
	*			1=Sleep (send a break to wake up)
	*			2=Deep sleep (toggle GPIO9 or RESET_N pin to wake up; the AX-SFEU is not running and all settings will be reset!)
	*/
	WISOL.println("AT$P=1");
	delay(10);
	
	//Changing the Pin to input is assumed to save some power
	pinMode(TxPin,INPUT);

	if (DEBUG) Serial.println("Sleeping...");
	delay(100);
		
	//going to sleep for 8 seconds for TX_Interval/8-times turning analog digital converter and brown-out detection off
	for (int x = 0; x < (TX_Interval)/8; x++ ){
		LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	}

}
// just for debugging print the data to serial
void printSensorValues() {
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}

void create_lpp(){
  // coverting the payload to 4 unsigned integers 
  unsigned int itemp =  ( int ( (bme.readTemperature() * 10+500) ) ); //-22.8°C => -228 ⇒ 272 ⇒ BIN 00000001 00010000 (HEX 0x01 0x10)
  unsigned int ihum  =  ( int (bme.readHumidity() * 10)  ); // 51.6% ⇒ 516 ⇒ BIN 00000010 00000100 (HEX 0x02 0x04)
  unsigned int ipress=  ( int ( (bme.readPressure()/100 * 10   ) ) );  //1014 hpa ⇒ 1014 ⇒ BIN 00000011 11110110 (HEX 0x03 0xF6)
  //formating the payload
  sprintf(lpp, "%04X%04X%04X", itemp, ihum, ipress);
      
}




