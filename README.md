# Machine Learning Experiment on 8-bit Arduino

This is an experiment about running the Tensorflow Lite model on an 8-bit Arduino, such as Arduino Uno or Arduino Leonardo. There is no official support for 8-bit Arduinos and those low-end Arduinos are not capable to run Tensorflow Lite interpreter. So this project tries to implement a neural network directly in native C code. 

![Arduino distinguish punch and flex](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/ML_mega32u4.gif)

This experiment uses an Arduino Leonardo to run a fully connected model with 150 inputs and 1577 params. The float model takes about 60ms to run and the 8-bit quantized model takes about 14ms to run. I guess further assembly optimization may improve the performance by 30%.

Disclaimer: This is a project I did as a learning process of machine learning. As a beginner, I can not guarantee this project or this document is accurate. If you find something wrong with the project or document, please submit an issue and let me know. Also, the model provided in this repo is a quick and dirty one.

## How to run a neural network on 8-bit Arduino, or any other low end microcontroller.

As an 8-bit microcontroller, Arduino Uno/Leonardo is not a good candidate for the neural network. There is no float point unit (FPU), which means the float number calculation is slow. The microcontroller has an 8-bit architecture, which means calculation with 32bit or 64bit numbers is slow, too. Also, both the flash space and RAM are limited, you can not fit in a larger model.

There is no surprise that the official Tensorflow Lite interpreter only supports 32-bit Arduino such as Zero or Nano 33 BLE Sense. However, if the application uses a simple model and has a lot of time to wait for the result, then why not just use a simpler microcontroller, without a lot of libraries, and hassle compiling them? 

Since we can not use the official TensorFlow library, so we just put all parameters into the Arduino and do the calculation in our code. The code in ```circuit_playground_classic_float_model/simpleml.cpp``` is an example of doing all calculations with less than 50 lines of code. And that code supports the Dense (fully connected) layer, and three activation functions, which are linear, relu and softmax ones.

## Implement the Arduino gesture classifier example

We are using the official [Arduino gesture classifier example
](https://github.com/arduino/ArduinoTensorFlowLiteTutorials/tree/master/GestureToEmoji) as an example. Sandeep Mistry & Dominic Pajak did a great job in their tutorial talking about how to train a model in Colab and Tensorflow, so I'll skip that part. 

We are going to use a motion sensor to take input of hand movement, then use a classifier to decide which gesture we just did. The original tutorial used a Nano 33 BLE Sense board with a 6-axis sensor. And I'm using a Circuit Playground Classic board with a 3-axis sensor. The code will continue monitoring the accelerometer, once the value goes beyond a certain threshold, the Arduino will start to record data for a certain time, and use that data for machine learning.

The official example includes all details about how to train the model. After some data collection and python coding, we will get a working Keras model. The next step is to move the Keras model to the Arduino code.

![simple network](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/NNs_2_variables.png) 

The Keras model we get has 3 dense layers. For each neuron in a certain layer, the value is determined by all neuron's value, the weight on the synapses, and the bias value. For the network shown above, ```y = x1*w1 + x2*w2 + b```.

And each layer can have an activation function. There are 3 most basic ones: Linear, which does nothing to neuron's output. Relu, which clamps the output with a zero minimum value. Softmax, coverts neuron's values to a probability distribution.

These calculations are all we need for this simple model. In Keras, it is pretty simple to get weights, biases, and activation functions of a model. There is a ```Machine_Learning_Experiment_on_8_bit_Arduino.ipynb``` jupyter notebook file in this repo, and you may refer code from cell 11 for codes.

The next step is to make code for Arduino to run the network. How should we confirm the code is working properly? I fed Keras with one sample, get the output of each layer. Then I used pure python to do the calculation with the same sample, without using any library, to get the same result. (code in cell 13, below comment "#to implement in Arduino")

The last step is to port the python code we just wrote to Arduino code, with all necessary parameters in a C header file format, printed in colab. The final code is in ```sketchForTesting/testFloatModelWithFixedSample/```.

## Run a quantized model for faster speed

The float model is simple to build and run, but slow. If higher speed is desired, we can use TFLiteConverter to convert a float Keras model into an 8-bit integer TFLite model (cell 16). And Arduino can run it much faster.

However, Tensorflow Lite is not as friendly as Keras. You can load a model, you can check each layer(tensor), you can set input and invoke the interpreter, you can get the parameter of each layer. However, the layer is shuffled and there is no API telling you how tensor flows. 

Here I used a tool called [Netron](https://github.com/lutzroeder/netron) to view the structure of the model. Also, Netron can tell us the name and parameters of each tensor, so we can use the name we get from Netron to grab parameters from Tensorflow interpreter.

![simple network](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/netronViewQuantize.png) 

Here we can see the structure of the TFlite file. The input is quantized first, and then it is calculated through 3 fully connected layers, one softmax activation function, and finally dequantized. On Arduino, we just want to compare which of the 2 output is larger, so we skip the softmax and dequantize process. 

Netron tells us the input of the quantizing process is tensor ```dense_input ``` and output is tensor ```dense_input_int8 ```. There are some quantization parameters, we can get the parameters in TensorFlow with ```interpreter.get_tensor_details()```. A part of the output is:

```
{'dtype': numpy.int8,
  'index': 0,
  'name': 'dense_input_int8',
  'quantization': (0.9960784316062927, -1),
  'quantization_parameters': {'quantized_dimension': 0,
   'scales': array([0.99607843], dtype=float32),
   'zero_points': array([-1], dtype=int32)},
  'shape': array([  1, 150], dtype=int32),
  'shape_signature': array([  1, 150], dtype=int32),
  'sparsity_parameters': {}}
```

The quantizing process is mapping float numbers into an int8 type to accelerate data access and computation. This is a linear process and we can reverse the process after all computations to get float numbers again. There will be some accurate loss but doesn't matter much.

![quantize map](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/quantize.png) 

The above image shows some general concepts of the quantizing process. We are not going to map the entire float number range. Instead, we are mapping a certain range, from rmin to rmax. The range is decided by Tensorflow lite, all we need to do is conversion.

![r=\frac{r_{max}-r_{min}}{(2^8-1)-0}\times(q-z)=S\times(q-z)](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/quantizeFunc1.gif)

or

![real\_value = scale \times ( int8\_value - zero\_point)](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/quantizeFunc2.gif) 

TODO: How quantization works, how to calculate quantized values, how to get parameters from the TFLite model.
 



