/*
        HMC5883 Compass
*/

void init_compass(void) {
         
    // Initialize and set continuous mode
    Wire.begin();
    Wire.beginTransmission(HMC5883_ADDRESS);
    Wire.write(0x02);    
    Wire.write(0x00); 
    Wire.endTransmission();
    Serial.print("HMC5883 Init");
}

int read_compass(int z) {
  
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
    z = x - y - z;
    return z;
}


