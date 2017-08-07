/*
        HMC5883 Compass
*/

void hmc5883_init(void) {
         
    // Initialize and set continuous mode

    //Wire.begin();
    int n = Wire.requestFrom(HMC5883_ADDRESS, 2);
    if (n == 2) // two bytes received
    { 
    Wire.beginTransmission(HMC5883_ADDRESS);
    Wire.write(0x02);    
    Wire.write(0x00);
    Wire.endTransmission();
    compass_state = 1;
    Serial.println("hmc5883 init");
    } else {
    compass_state = 0;
    //Serial.println("hmc5883 not found");
    }
    
}

//int read_compass(int z, int x, int y) {
  int read_compass() {

    Wire.beginTransmission(HMC5883_ADDRESS);
    Wire.write(0x03); // Select register 3, X MSB register
    Wire.endTransmission();
   
    // Read data from each axis, 2 registers per axis
    Wire.requestFrom(HMC5883_ADDRESS, 6);
    if(6<=Wire.available()) {
    x = Wire.read() << 8; // X msb
    x |= Wire.read();     // X lsb
    y = Wire.read() << 8; // Z msb
    y |= Wire.read();     // Z lsb
    z = Wire.read() << 8; // Y msb
    z |= Wire.read();     // Y lsb
  }
  
    // Print out values of each axis
   // Serial.print("x: ");
   // Serial.print(x);
   // Serial.print("  y: ");
   // Serial.print(y);
   // Serial.print("  z: ");
   // Serial.println(z);
    return x,y,z;
}


