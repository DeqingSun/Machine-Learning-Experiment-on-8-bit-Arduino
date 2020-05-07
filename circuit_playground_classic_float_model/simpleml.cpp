#include "simpleml.h"
#include "simpleml_data.h"
#include <Arduino.h>

float bufferIn[BUFFER_IN_SIZE];
float bufferOut[BUFFER_OUT_SIZE];


void loadData(signed char *dataPtr) {
  float *bufferPtr = bufferIn;

  for (uint16_t i = 0; i < BUFFER_IN_SIZE; i++) {
    *bufferPtr++ = (((signed int)(*dataPtr++))+127)/256.0;
  }

}

void processNN() {
  Serial.println("Process!!!");
  for  (uint16_t layerIndex = 0; layerIndex < LAYER_COUNT; layerIndex++) {
    uint16_t bufferInSize = pgm_read_word(&layerSize[layerIndex]);
    uint16_t bufferOutSize = pgm_read_word(&layerSize[layerIndex + 1]);
    const PROGMEM float *layerWeightArray = (const PROGMEM float *)pgm_read_word(&(layerWeightArrays[layerIndex]));
    const PROGMEM float *layerBaisArray = (const PROGMEM float *)pgm_read_word(&(layerBaisArrays[layerIndex]));

    uint16_t weightCounter = 0;
    for (uint16_t neuronIndex = 0; neuronIndex < bufferOutSize; neuronIndex++) {
      float s = 0;
      for (uint16_t i = 0; i < bufferInSize; i++) {
        s += bufferIn[i] * pgm_read_float(&layerWeightArray[weightCounter]);
        weightCounter += 1;
      }
      s = s + pgm_read_float(&layerBaisArray[neuronIndex]);
      bufferOut[neuronIndex] = s;
    }
    uint8_t activationFuncOnLayer = pgm_read_byte(&activationFunc[layerIndex]);
    if (activationFuncOnLayer == AF_relu) {
      for (uint16_t neuronIndex = 0; neuronIndex < bufferOutSize; neuronIndex++) {
        if (bufferOut[neuronIndex] < 0) {
          bufferOut[neuronIndex] = 0;
        }
      }
    } else if (activationFuncOnLayer == AF_softmax) {
      float s = 0;
      for (uint16_t neuronIndex = 0; neuronIndex < bufferOutSize; neuronIndex++) {
        float expOfValue = exp(bufferOut[neuronIndex]);
        s += expOfValue;
        bufferOut[neuronIndex] = expOfValue;
      }
      s = 1.0 / s;
      for (uint16_t neuronIndex = 0; neuronIndex < bufferOutSize; neuronIndex++) {
        bufferOut[neuronIndex] = bufferOut[neuronIndex] * s;
      }
    }

    for (uint16_t neuronIndex = 0; neuronIndex < bufferOutSize; neuronIndex++) {
      bufferIn[neuronIndex] = bufferOut[neuronIndex];
    }

  }



}
