#include "simpleml.h"
#include "simpleml_data.h"


int8_t bufferIn[BUFFER_IN_SIZE];
int8_t bufferOut[BUFFER_OUT_SIZE];

void printBufferIn(uint16_t len) {
  Serial.print("Input Data:");
  for (uint16_t i = 0; i < len; i++) {
    Serial.print(bufferIn[i]);
    Serial.print(",");
  }
  Serial.println();
}

void processNN(int8_t *dataPtr) {
  Serial.println("Process!!!");
  int8_t *bufferPtr = bufferIn;

  for (uint16_t i = 0; i < BUFFER_IN_SIZE; i++) {
    *bufferPtr++ = *dataPtr++;
  }

  printBufferIn(BUFFER_IN_SIZE);//Test

  if ( (QUANTIZE_SCALE < 0.95) || (QUANTIZE_SCALE > 1.05) || (QUANTIZE_ZERO_POINT < -5) || (QUANTIZE_ZERO_POINT > 5) || (1)) { //skip if Quantization doesn't do much
    //Quantization process
    for (uint16_t i = 0; i < BUFFER_IN_SIZE; i++) {
      int16_t n = ((float)bufferIn[i]) * (1.0 / QUANTIZE_SCALE) + QUANTIZE_ZERO_POINT;
      if (n < -128)
        n = -128;
      else if (n > 127)
        n = 127;
      bufferIn[i] = n;
    }
  }

  printBufferIn(BUFFER_IN_SIZE);//Test

  for  (uint16_t layerIndex = 0; layerIndex < LAYER_COUNT; layerIndex++) {
    uint16_t bufferInSize = pgm_read_word(&layerSize[layerIndex]);
    uint16_t bufferOutSize = pgm_read_word(&layerSize[layerIndex + 1]);
    const PROGMEM int8_t *layerWeightArray = (const PROGMEM int8_t *)pgm_read_word(&(layerWeightArrays[layerIndex]));
    const PROGMEM int8_t *layerBaisArray = (const PROGMEM int8_t *)pgm_read_word(&(layerBaisArrays[layerIndex]));

    //The datatype can be smaller, here I just follow the TFlite lib
    int32_t inputZeroPoint = pgm_read_dword(&inputZeroPoints[layerIndex]);
    int32_t weightZeroPoint = pgm_read_dword(&weightZeroPoints[layerIndex]);
    int32_t outputZeroPoint = pgm_read_dword(&outputZeroPoints[layerIndex]);
    int32_t reduced_multiplier = pgm_read_dword(&reduced_multipliers[layerIndex]);
    uint8_t total_shift = pgm_read_byte(&total_shifts[layerIndex]);

    uint16_t weightCounter = 0;
    for (uint16_t neuronIndex = 0; neuronIndex < bufferOutSize; neuronIndex++) {
      int32_t s = 0;
      for (uint16_t i = 0; i < bufferInSize; i++) {
        s += (bufferIn[i] - inputZeroPoint) * ((int8_t)pgm_read_byte(&layerWeightArray[weightCounter]) - weightZeroPoint);
        weightCounter += 1;
        
      }
      s = s + (int8_t)pgm_read_byte(&layerBaisArray[neuronIndex]);

      s = (s * reduced_multiplier) + (1 << (total_shift - 1));
      s = s >> total_shift;

      s += outputZeroPoint;

      if (s < -128)
        s = -128;
      else if (s > 127)
        s = 127;

      bufferOut[neuronIndex] = s;
    }


    for (uint16_t neuronIndex = 0; neuronIndex < bufferOutSize; neuronIndex++) {
      bufferIn[neuronIndex] = bufferOut[neuronIndex];
    }

    { //For Test
      Serial.println("Layer info");
      Serial.println(bufferInSize);
      Serial.println(bufferOutSize);
      for (uint16_t i = 0; i < bufferOutSize; i++) {
        Serial.print(bufferOut[i]);
        Serial.print(",");
      }
      Serial.println();

    }
  }



}
