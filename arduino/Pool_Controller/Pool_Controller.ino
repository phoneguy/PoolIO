
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

// I2C addresses for devices
#define HMC5883_ADDRESS 0x1E 
#define BMP085_ADDRESS  0x77
#define BLINKM_ADDRESS  0x09
#define ITG3200_ADDRESS 0x69
#define BMA180_ADDRESS  0x40

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

int relay1_state = 0;
int relay2_state = 0;
int baro_state = 0;
int compass_state = 0;
int blinkm_state = 0;

int pool_temp = 0;
int air_temp = 0;
int case_temp = 0;
int board_temp = 0;
int motor_temp = 0;
int solar_temp = 0;

int voltage = 0;
int current = 0;
int dc_volt = 0;
int dc_voltage = 0;
int dc_current = 0;
int dc_watts = 0;

int ac_volt = 0;
int ac_voltage = 0;
int ac_current = 0;
int ac_watts = 0;

float temperature = 0;
int altitude = 0;
int humidity = 0;
int air_pressure = 0;
float pressure = 0;

int x = 0;
int y = 0;
int z = 0;
int mag_z = 0;

unsigned long currentMillis= 0;
long previousMillis = 0;
long interval = 2000;
long fast_interval = 100;
long slow_interval = 1000;

// Hardware ID and Node ID
char hardware_id[7] = "POOLIO";
char node_id[3] = "99";

// Serial IO
char packet_one[32] = "";
char packet_two[32] = "";
char id[32];
String input_string = "";
boolean string_complete = false;
boolean relaystatus = false;

int debug = 0;

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

    // Start i2c bus
    Wire.begin();

    // Set up the LCD's number of columns and rows 
    lcd.begin(20, 4);
    start_screen();
    delay(1000);
    
    // Set the data rate for the SoftwareSerial port
    mySerial.begin(9600);
    sprintf(id, "Hardware ID: %s", hardware_id);
    mySerial.println(id);
    sprintf(id, "Node ID: %s", node_id);
    mySerial.println(id);
    
    // Start usb serial connections
    Serial.begin(9600);
    Serial.println("Connected To USB Serial Port");
    sprintf(id, "Hardware ID: %s", hardware_id);
    Serial.println(id);
    sprintf(id, "Node ID: %s", node_id);
    Serial.println(id);
    scan_i2cbus();
 
    // Start barometer
    bmp085_init();
    delay(100);
    
    // Start compass
    hmc5883_init();
    delay(100);
    
    // Start BlinkM
    blinkm_init();
    
    //BlinkM_begin();
    //BlinkM_off(0x09);
    //delay(100);
    //BlinkM_setFadeSpeed(BLINKM_ADDRESS, 25);
    
    // Setup OpenEnergyMonitor sensors
    emon1.voltage(AC_VOLT_PIN, 54.26, 1.7); // Voltage: input pin, calibration, phase_shift
    emon1.current(AC_CURRENT_PIN, 211.22);    // Current: input pin, calibration. 
    delay(100);
        
    // Start ds18b20 temperature sensors
    sensors.begin();
    
    // Start DHT22 temperature and humidity sensor
    dht.begin();
    
    // Print variables header info to usb serial port
    Serial.println("ND  PT  AT  ST  H  PR  V  C  W  R1  R2");
    
}

void loop() {
    
    softserial_event();

    emon1.calcVI(20,2000);                       // Calculate all. No.of half wavelengths (crossings), time-out
    float realPower       = emon1.realPower;     // extract Real Power into variable
    float apparentPower   = emon1.apparentPower; // extract Apparent Power into variable
    float powerFActor     = emon1.powerFactor;   // extract Power Factor into Variable
    float supplyVoltage   = emon1.Vrms;          // extract Vrms into Variable
    float Irms            = emon1.Irms;          // extract Irms into Variable
    
    ac_voltage = supplyVoltage; // integer
    ac_current = Irms;          // integer
    ac_watts   = supplyVoltage * Irms; // integer

    //float calc_dcvolt    = analogRead(DC_VOLT_PIN)    * 0.00488;
    //float calc_dccurrent = analogRead(DC_CURRENT_PIN) * 0.00488;
    //dc_volt    = calc_dcvolt     * 16;
    //dc_current = calc_dccurrent  * 2;
    sensors.requestTemperatures(); // Send the command to get temperature readings
    pool_temp  = ((sensors.getTempCByIndex(0)) * 1.8 + 32);
    air_temp   = ((sensors.getTempCByIndex(1)) * 1.8 + 32);
    solar_temp = ((sensors.getTempCByIndex(2)) * 1.8 + 32);
   
    dht.readHumidity();
    dht.readTemperature();
    humidity  = dht.humidity;
    case_temp = dht.temperature_F;
    
    if (baro_state == 1) {
        temperature  = bmp085GetTemperature(bmp085ReadUT()); //MUST be called first
        pressure     = bmp085GetPressure(bmp085ReadUP());
    }
    
    air_pressure = pressure * 0.01;
    board_temp   = temperature;
    float atm = pressure / 101325; // "standard atmosphere"
    float altitude = calcAltitude(pressure); //Uncompensated caculation - in Meters 
    
    currentMillis = millis();    
    if(currentMillis - previousMillis >= interval) {     
        // packet size is 32 bytes, split 40 bytes              1            2          3          4          5           6            7         8           9            10           11  
        // Print to hardware and software serial ports         SSS           SNNS      SNNS       SNNS       NNNS        NNNNS       NNNS        NNS       NNNNS          NS           NS
        // sprintf(packet_one, "%s %i %i %i %u %u %u ", node_id, pool_temp, air_temp, solar_temp, humidity, air_pressure, ac_voltage); 

        sprintf(packet_one, "%i %i %i %u %u %u ", pool_temp, air_temp, solar_temp, humidity, air_pressure, ac_voltage); 
        sprintf(packet_two, "%u %u %u %u ", ac_current, ac_watts, relay1_state, relay2_state);

        if (debug == 1) {  
            Serial.print(packet_one);        
            Serial.println(packet_two);
        }
          
        mySerial.print(packet_one);
        mySerial.println(packet_two);
        
        // Print to lcd
        screen_a();
        
        if (blinkm_state == 1) {
        blinkm_setrgb(BLINKM_ADDRESS, humidity, (ac_voltage * 2), (relay1_state *255));
        }
    
        previousMillis = millis();   
        
    }
    
    
}

void softserial_event() {
    while (mySerial.available()) {
        char inChar = (char)mySerial.read();
        input_string += inChar;

        if (inChar == '\n') {
        string_complete = true;
        }
    }
    if(input_string == "R10") {
        Serial.println("Relay 1 OFF"); // Print to USB port
        digitalWrite(RELAY1_PIN, RELAY_OFF);
        relay1_state = 0;
        }
    else if (input_string == "R11") {
        Serial.println("Relay 1 ON");
        digitalWrite(RELAY1_PIN, RELAY_ON);
        relay1_state = 1;
        }
    else if (input_string == "R20") {
        Serial.println("Relay 2 OFF");
        digitalWrite(RELAY2_PIN, RELAY_OFF);
        relay2_state = 0;
        }
    else if (input_string == "R21") {
        Serial.println("Relay 2 ON");
        digitalWrite(RELAY2_PIN, RELAY_ON);
        relay2_state = 1;
        }
    else if (input_string == "D0") {
        debug = 0;   
        Serial.println("DEBUG OFF TO USB SERIAL PORT");
        }
    else if (input_string == "D1") {
        debug = 1;  
        Serial.println("DEBUG ON TO USB SERIAL PORT");
        }   
      
    input_string = "";
    string_complete = false;
}

void scan_i2cbus() {
    Serial.begin (9600);
    Serial.println ();
    Serial.println ("I2C bus scanner...");
    byte count = 0;
 
    for (byte i = 1; i < 120; i++) {
        Wire.beginTransmission (i);
        if (Wire.endTransmission () == 0) {
            Serial.print ("Found address: ");
            if (i < 16) {
                Serial.print(" 0x0");
                Serial.print(i, HEX);
                Serial.print(", ");
                if (i == BLINKM_ADDRESS) {
                    Serial.print("blinkm");
                    }
                Serial.println(""); 
            }
            else {
            Serial.print(" 0x");
            Serial.print(i, HEX);
            Serial.print(", ");
                if (i == HMC5883_ADDRESS) {
                    Serial.print("hmc4883");
                    }
                else if (i == BMA180_ADDRESS) {
                    Serial.print("bma180");
                    }
                else if (i == ITG3200_ADDRESS) {
                    Serial.print("itg3200");
                    }
                else if (i == BMP085_ADDRESS) {
                    Serial.print("bmp085");
                    }
                                    
             Serial.println("");       
            }
        count++;
        delay (1);
        }
      
    } 
    Serial.println("Done.");
    Serial.print("Found ");
    Serial.print(count, DEC);
    Serial.println(" device(s).");
}




