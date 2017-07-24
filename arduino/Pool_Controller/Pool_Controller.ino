
/*
        PoolController.ino by stevenharsanyi@gmail.com
        rev 1.0 June 11, 2017 
*/
#include <SoftwareSerial.h>
#include <EmonLib.h>
#include <Wire.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include "cactus_io_DHT22.h"
#include "BlinkM_funcs.h"

// I2C addresses for devices
#define HMC5883_ADDRESS 0x1E 
#define BMP085_ADDRESS 0x77
#define BLINKM_ADDRESS 0x09

// Arduino NANO digital pins
#define RELAY1_PIN     2
#define RELAY2_PIN     3
#define DHT22_PIN      4   
#define ONEWIREBUS_PIN 5
#define RS_PIN         11
#define EN_PIN         10
#define D4_PIN         9
#define D5_PIN         8
#define D6_PIN         7
#define D7_PIN         6
#define SERIALRX_PIN   12
#define SERIALTX_PIN   13

// Arduino NANO analog pins
#define DC_VOLT_PIN    A0
#define DC_CURRENT_PIN A1
#define AC_VOLT_PIN    A2
#define AC_CURRENT_PIN A3
#define I2C_SDA_PIN    A4
#define I2C_SCL_PIN    A5
#define LED_1_PIN      A6 
#define LED_2_PIN      A7

// Number of relay channels in use
#define RELAY_CHANNELS 2
#define RELAY_ON LOW  
#define RELAY_OFF HIGH

// Variables
int pool_temp = 0;
int air_temp = 0;
int case_temp = 0;
int motor_temp = 0;
int solar_temp = 0;
int voltage = 0;
int current = 0;
int dc_volt = 0;
int dc_current = 0;
int ac_volt = 0;
int ac_voltage = 0;
int ac_current = 0;
int ac_kwhr = 0;
int ac_watts =0;
float temperature = 0;
int altitude = 0;
int humidity = 0;
int air_pressure = 0;
float pressure = 0;
int x = 0;
int y = 0;
int z = 0;
int mag_z = 0;
int relay = 0;
int state = 0;
int control_msg = 0;
char buffer[50];
int relay1_state = 0;
int relay2_state = 0;

// Serial input
//int cmd = 0;
String inputString = "";
boolean stringComplete = false;
boolean relaystatus = false;

SoftwareSerial mySerial(SERIALRX_PIN, SERIALTX_PIN); // RX, TX
EnergyMonitor emon1;                   // Create an instance

// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONEWIREBUS_PIN); 

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Setup DHT22 sensor
DHT22 dht(DHT22_PIN);

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

void setup() {
    
    // Setup relay pins
    pinMode(RELAY1_PIN,      OUTPUT);
    pinMode(RELAY2_PIN,      OUTPUT);
    digitalWrite(RELAY1_PIN, HIGH);
    digitalWrite(RELAY2_PIN, HIGH);

    // Start usb serial connections
    Serial.begin(9600);
    Serial.println("Poolcontrol_USB");

    // set the data rate for the SoftwareSerial port
    mySerial.begin(9600);
    mySerial.println("Poolcontrol");
  
    // Set up the LCD's number of columns and rows 
    lcd.begin(20, 4);
    start_screen();
    delay(1000);
    
    // Setup OpenEnergyMonitor sensors
    emon1.voltage(AC_VOLT_PIN, 54.26, 1.7); // Voltage: input pin, calibration, phase_shift
    emon1.current(AC_CURRENT_PIN, 211.22);    // Current: input pin, calibration. 
    delay(100);
    
    // Start barometer
    bmp085Calibration();
    delay(100);
    
    // Start compass
    init_compass();
    
    // Start BlinkM
    BlinkM_begin();
    BlinkM_off(0x09);
    delay(10);
    BlinkM_setFadeSpeed(BLINKM_ADDRESS, 25);
    
    // Start ds18b20 temperature sensors
    sensors.begin();
    
    // Start DHT22 temperature and humidity sensor
    dht.begin();
    
}

void loop() {
    z=read_compass(z);
    
    softserial_event();

    sensors.requestTemperatures(); // Send the command to get temperature readings
    pool_temp = ((sensors.getTempCByIndex(0)) * 1.8 + 32);
    motor_temp = ((sensors.getTempCByIndex(1)) * 1.8 + 32);
    solar_temp = ((sensors.getTempCByIndex(2)) * 1.8 + 32);
    
    emon1.calcVI(20,2000);                   // Calculate all. No.of half wavelengths (crossings), time-out
    //emon1.serialprint();                   // Print out all variables (realpower, apparent power, Vrms, Irms, power factor)
    float realPower       = emon1.realPower; //extract Real Power into variable
    float apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
    float powerFActor     = emon1.powerFactor;      //extract Power Factor into Variable
    float supplyVoltage   = emon1.Vrms;      //extract Vrms into Variable
    float Irms            = emon1.Irms;      //extract Irms into Variable
    ac_voltage = supplyVoltage; // integer
    ac_current = Irms;          // integer
    ac_watts   = ac_voltage * ac_current; // integer
        
    float calc_dcvolt    = analogRead(DC_VOLT_PIN)    * 0.00488;
    float calc_dccurrent = analogRead(DC_CURRENT_PIN) * 0.00488;
    dc_volt    = calc_dcvolt     * 16;
    dc_current = calc_dccurrent  * 2;

    temperature  = bmp085GetTemperature(bmp085ReadUT()); //MUST be called first
    case_temp    = temperature;
    pressure     = bmp085GetPressure(bmp085ReadUP());
    air_pressure = pressure * 0.01;
    //float atm = pressure / 101325; // "standard atmosphere"
    //float altitude = calcAltitude(pressure); //Uncompensated caculation - in Meters 
        
    dht.readHumidity();
    dht.readTemperature();
    humidity = dht.humidity;
    air_temp = dht.temperature_F;
    
    // Print to hardware and software serial ports
    sprintf(buffer, "%i %i %u %u %u %u %u %u %u %u", pool_temp, air_temp, humidity, air_pressure, dc_volt, dc_current, ac_voltage, ac_current, relay1_state, relay2_state);

    Serial.println(buffer);
    mySerial.println(buffer);
    
    // Print to lcd
    screen_a();
    
    BlinkM_fadeToRGB(BLINKM_ADDRESS, pool_temp, air_temp, ac_current);
    delay(100);
}

void softserial_event() {
    while (mySerial.available()) {
        char inChar = (char)mySerial.read();
        inputString += inChar;

        if (inChar == '\n') {
        stringComplete = true;
        }
    }
    if(inputString == "R10"){
        Serial.println("Relay 1 OFF"); // Print to USB port
        digitalWrite(RELAY1_PIN, RELAY_OFF);
        relay1_state = 0;
      }
    else if (inputString == "R11"){
        Serial.println("Relay 1 ON");
        digitalWrite(RELAY1_PIN, RELAY_ON);
        relay1_state = 1;
      }
    else if (inputString == "R20"){
        Serial.println("Relay 2 OFF");
        digitalWrite(RELAY2_PIN, RELAY_OFF);
        relay2_state = 0;
      }
    else if (inputString == "R21"){
        Serial.println("Relay 2 ON");
        digitalWrite(RELAY2_PIN, RELAY_ON);
        relay2_state = 1;
      }
    inputString = "";
    stringComplete = false;
}







