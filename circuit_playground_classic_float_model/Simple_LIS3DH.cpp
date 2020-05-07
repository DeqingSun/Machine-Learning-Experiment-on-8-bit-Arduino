
#include "Arduino.h"

#include "Simple_LIS3DH.h"

#include <SPI.h>
/**************************************************************************/
/*!
    @brief  Instantiates a new LIS3DH class in I2C or SPI mode
*/
/**************************************************************************/


Simple_LIS3DH::Simple_LIS3DH(int8_t cspin)
  : _cs(cspin)
{ }



/**************************************************************************/
/*!
    @brief  Setups the HW (reads coefficients values, etc.)
*/
/**************************************************************************/
bool Simple_LIS3DH::begin() {

  digitalWrite(_cs, HIGH);
  pinMode(_cs, OUTPUT);
  SPI.begin();
  /*
    Serial.println("Debug");

    for (uint8_t i=0; i<0x30; i++) {
    Serial.print("$");
    Serial.print(i, HEX); Serial.print(" = 0x");
    Serial.println(readRegister8(i), HEX);
    }
  */

  /* Check connection */
  uint8_t deviceid = readRegister8(LIS3DH_REG_WHOAMI);
  if (deviceid != 0x33)
  {
    /* No LIS3DH detected ... return false */
    //Serial.println(deviceid, HEX);
    return false;
  }

  // enable all axes, normal mode
  writeRegister8(LIS3DH_REG_CTRL1, 0x07);
  // 100Hz rate
  setDataRate(LIS3DH_DATARATE_100_HZ);

  // High res & BDU enabled
  writeRegister8(LIS3DH_REG_CTRL4, 0x88);

  // DRDY on INT1
  //  writeRegister8(LIS3DH_REG_CTRL3, 0x10);

  // Turn on orientation config
  //writeRegister8(LIS3DH_REG_PL_CFG, 0x40);

  // disable adcs
  writeRegister8(LIS3DH_REG_TEMPCFG, 0x00);

  /*
    for (uint8_t i=0; i<0x30; i++) {
    Serial.print("$");
    Serial.print(i, HEX); Serial.print(" = 0x");
    Serial.println(readRegister8(i), HEX);
    }
  */

  return true;
}

void Simple_LIS3DH::setupFifo(void) {
  //set fifo
  uint8_t dataToWrite = 0;  //Temporary variable

  //Build LIS3DH_FIFO_CTRL_REG
  dataToWrite = readRegister8(LIS3DH_REG_FIFOCTRL); //Start with existing data
  dataToWrite &= 0x20;//clear all but bit 5
  dataToWrite |= ((0x01) << 6) | (20 & 0x1F); //apply mode & watermark threshold
  writeRegister8(LIS3DH_REG_FIFOCTRL, dataToWrite);

  //Build CTRL_REG5
  dataToWrite = readRegister8(LIS3DH_REG_CTRL5); //Start with existing data
  dataToWrite &= 0xBF;//clear bit 6 //FIFO enable
  dataToWrite |= (0x01) << 6;
  //Now, write the patched together data
  writeRegister8(LIS3DH_REG_CTRL5, dataToWrite);

  //fifoClear
  while ( (fifoGetStatus() & 0x20 ) == 0 ) {  // EMPTY flag
    read();
  }

  resetFifo();
}

void Simple_LIS3DH::resetFifo(void) {
  //Turn off...
  uint8_t dataToWrite = readRegister8(LIS3DH_REG_FIFOCTRL);
  dataToWrite &= 0x3F;//clear mode
  writeRegister8(LIS3DH_REG_FIFOCTRL, dataToWrite);
  dataToWrite |= (0x01 & 0x03) << 6; //apply mode
  //Now, write the patched together data
  writeRegister8(LIS3DH_REG_FIFOCTRL, dataToWrite);

}

void Simple_LIS3DH::read(void) {
  // read x y z at once

  {
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
    digitalWrite(_cs, LOW);
    SPI.transfer(LIS3DH_REG_OUT_X_L | 0x80 | 0x40); // read multiple, bit 7&6 high

    x = SPI.transfer(0xFF); x |= ((uint16_t)SPI.transfer(0xFF)) << 8;
    y = SPI.transfer(0xFF); y |= ((uint16_t)SPI.transfer(0xFF)) << 8;
    z = SPI.transfer(0xFF); z |= ((uint16_t)SPI.transfer(0xFF)) << 8;

    digitalWrite(_cs, HIGH);

    SPI.endTransaction();              // release the SPI bus

  }
}

void Simple_LIS3DH::setClick(uint8_t clickthresh, uint8_t timelimit, uint8_t timelatency, uint8_t timewindow) {
  uint8_t dataToWrite = 0;

  writeRegister8(LIS3DH_REG_CTRL3, 0x80); // turn on int1 click
  dataToWrite = readRegister8(LIS3DH_REG_CTRL5); //Start with existing data
  dataToWrite |= 0x08;
  writeRegister8(LIS3DH_REG_CTRL5, dataToWrite); // latch interrupt on int1

  writeRegister8(LIS3DH_REG_CLICKCFG, 0x15); // turn on all axes & singletap

  writeRegister8(LIS3DH_REG_CLICKTHS, clickthresh); // arbitrary
  writeRegister8(LIS3DH_REG_TIMELIMIT, timelimit); // arbitrary
  writeRegister8(LIS3DH_REG_TIMELATENCY, timelatency); // arbitrary
  writeRegister8(LIS3DH_REG_TIMEWINDOW, timewindow); // arbitrary
}

uint8_t Simple_LIS3DH::getClick(void) {
  return readRegister8(LIS3DH_REG_CLICKSRC);
}

/**************************************************************************/
/*!
    @brief  Sets the g range for the accelerometer
*/
/**************************************************************************/
void Simple_LIS3DH::setRange(lis3dh_range_t range)
{
  uint8_t r = readRegister8(LIS3DH_REG_CTRL4);
  r &= ~(0x30);
  r |= range << 4;
  writeRegister8(LIS3DH_REG_CTRL4, r);
}

/**************************************************************************/
/*!
    @brief  Sets the g range for the accelerometer
*/
/**************************************************************************/
lis3dh_range_t Simple_LIS3DH::getRange(void)
{
  /* Read the data format register to preserve bits */
  return (lis3dh_range_t)((readRegister8(LIS3DH_REG_CTRL4) >> 4) & 0x03);
}

/**************************************************************************/
/*!
    @brief  Sets the data rate for the LIS3DH (controls power consumption)
*/
/**************************************************************************/
void Simple_LIS3DH::setDataRate(lis3dh_dataRate_t dataRate)
{
  uint8_t ctl1 = readRegister8(LIS3DH_REG_CTRL1);
  ctl1 &= ~(0xF0); // mask off bits
  ctl1 |= (dataRate << 4);
  writeRegister8(LIS3DH_REG_CTRL1, ctl1);
}

/**************************************************************************/
/*!
    @brief  Sets the data rate for the LIS3DH (controls power consumption)
*/
/**************************************************************************/
lis3dh_dataRate_t Simple_LIS3DH::getDataRate(void)
{
  return (lis3dh_dataRate_t)((readRegister8(LIS3DH_REG_CTRL1) >> 4) & 0x0F);
}

/**************************************************************************/
/*!
    @brief  Gets the most recent sensor event
*/



/**************************************************************************/
/*!
    @brief  Writes 8-bits to the specified destination register
*/
/**************************************************************************/
void Simple_LIS3DH::writeRegister8(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
  digitalWrite(_cs, LOW);
  SPI.transfer(reg & ~0x80); // write, bit 7 low
  SPI.transfer(value);
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();              // release the SPI bus
}

/**************************************************************************/
/*!
    @brief  Reads 8-bits from the specified register
*/
/**************************************************************************/
uint8_t Simple_LIS3DH::readRegister8(uint8_t reg) {
  uint8_t value;

  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
  digitalWrite(_cs, LOW);
  SPI.transfer(reg | 0x80); // read, bit 7 high
  value = SPI.transfer(0);
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();              // release the SPI bus

  return value;
}

uint8_t Simple_LIS3DH::fifoGetStatus( void )
{
  //Return some data on the state of the fifo
  uint8_t tempReadByte = 0;
  tempReadByte = readRegister8(LIS3DH_REG_FIFOSRC);
  return tempReadByte;
}

