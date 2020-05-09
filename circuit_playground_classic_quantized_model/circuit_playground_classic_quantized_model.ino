#include <SPI.h>
#include "Simple_LIS3DH.h"

#include "simpleml.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define LIS3DH_REG_WHOAMI        0x0F
Simple_LIS3DH lis = Simple_LIS3DH(8);

#define NUM_SAMPLES  50

int samplesRead = NUM_SAMPLES;

signed char dataSample[NUM_SAMPLES * 3];

extern int8_t bufferOut[];

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(10, 17, NEO_GRB + NEO_KHZ800);

void setup() {
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(5, 5, 5));
  pixels.show();

  Serial.begin(9600);

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

}

void loop() {

  while (samplesRead == NUM_SAMPLES) {
    lis.read();
    int sum = abs(lis.x) + abs(lis.y) + abs(lis.z);
    if (sum > 1200) {
      samplesRead = 0;
    }
  }


  while (samplesRead < NUM_SAMPLES) {
    lis.read();

    dataSample[samplesRead * 3 + 0] = constrain(lis.x / 32, -127, 127);
    dataSample[samplesRead * 3 + 1] = constrain(lis.y / 32, -127, 127);
    dataSample[samplesRead * 3 + 2] = constrain(lis.z / 32, -127, 127);

    samplesRead++;

    if (samplesRead == NUM_SAMPLES) {
      // add an empty line if it's the last sample
      Serial.println();
    }
    delay(20);
  }

  unsigned long startTime = micros();

  processNN(dataSample);

  unsigned long endTime = micros();

  Serial.println("Result:");
  Serial.print("punch:");
  Serial.print(bufferOut[0]);
  Serial.print("flex:");
  Serial.println(bufferOut[1]);

  Serial.print("Time:");
  Serial.println(endTime - startTime);

  if (bufferOut[0] > bufferOut[1]) {
    pixels.setPixelColor(0, pixels.Color(8, 0, 0));
  } else {
    pixels.setPixelColor(0, pixels.Color(0, 8, 0));
  }
  pixels.show();
  delay(500);
  pixels.setPixelColor(0, pixels.Color(1, 1, 1));
  pixels.show();
}
