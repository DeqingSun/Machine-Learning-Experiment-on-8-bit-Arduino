#include <SPI.h>
#include "Simple_LIS3DH.h"

#define LIS3DH_REG_WHOAMI        0x0F
Simple_LIS3DH lis = Simple_LIS3DH(8);

const int numSamples = 100;

int samplesRead = numSamples;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  if (! lis.begin()) {
    Serial.println("Couldnt start LIS3DH");
    while (1);
  }
  lis.writeRegister8(LIS3DH_REG_CTRL4, 0x08);
  lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G! In 4G, 8192->1G

  unsigned char reg;
  reg = lis.readRegister8(LIS3DH_REG_CTRL2);
  lis.writeRegister8(LIS3DH_REG_CTRL2, reg |= 0b00001001); //Filtered data selection

  reg = lis.readRegister8(LIS3DH_REG_CTRL5);
  lis.writeRegister8(LIS3DH_REG_CTRL5, reg |= (1 << 3)); //Latch interrupt request on INT1_SRC register

  delay(1000);
  Serial.println("aX,aY,aZ");
}

void loop() {

  while (samplesRead == numSamples) {
    lis.read();
    int sum = abs(lis.x) + abs(lis.y) + abs(lis.z);
    if (sum > 1200) {
      samplesRead = 0;
    }
  }


  while (samplesRead < numSamples) {
    lis.read();
    samplesRead++;
    // print the data in CSV format
    Serial.print(constrain(lis.x/32,-127,127));
    Serial.print(',');
    Serial.print(constrain(lis.y/32,-127,127));
    Serial.print(',');
    Serial.print(constrain(lis.z/32,-127,127));
    Serial.println();

    if (samplesRead == numSamples) {
      // add an empty line if it's the last sample
      Serial.println();
    }
    delay(10);
  }

}
