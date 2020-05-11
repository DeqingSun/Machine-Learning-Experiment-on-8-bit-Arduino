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

The float model is simple to build and run, but slow. If higher speed is desired, we can use TFLiteConverter to convert a float Keras model into an 8-bit integer TFLite model (cell 16). And Arduino can run it much faster. The algorithm mentioned below is ported from the Tensorflow Lite library.

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

### Quantization

The quantizing process is mapping float numbers into an int8 type to accelerate data access and computation. This is a linear process and we can reverse the process after all computations to get float numbers again. There will be some accurate loss but doesn't matter much.

![quantize map](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/quantize.png) 

The above image shows some general concepts of the quantizing process. We are not going to map the entire float number range. Instead, we are mapping a certain range, from rmin to rmax. The range is decided by Tensorflow lite, all we need to do is conversion.

![r=\frac{r_{max}-r_{min}}{(2^8-1)-0}\times(q-z)=S\times(q-z)](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/quantizeFunc1.gif)

or

![real\_value = scale \times ( int8\_value - zero\_point)](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/quantizeFunc2.gif) 

In Arduino, this process does need float calculation. And we need to constrain result into [-128,127] range to prevent overflow. If the scale value is close to 1.0 and the zero point is close to 0.0, we can also skip this step.

### Fully Connected

Let's take a look at how FullyConnected layer works in Netron:

![fully connected layer](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/netronViewQuantize2.png) 

So we can see the FullyConnected layer involves 4 tensors: the input ```dense_input_int8```, the weights ```sequential/dense/MatMul```, the bais ```sequential/dense/BiasAdd/ReadVariableOp``` and the output ```sequential/dense/Relu```. Their details are:

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
  'sparsity_parameters': {}},
 {'dtype': numpy.int32,
  'index': 1,
  'name': 'sequential/dense/BiasAdd/ReadVariableOp',
  'quantization': (0.0040348549373447895, 0),
  'quantization_parameters': {'quantized_dimension': 0,
   'scales': array([0.00403485], dtype=float32),
   'zero_points': array([0], dtype=int32)},
  'shape': array([10], dtype=int32),
  'shape_signature': array([10], dtype=int32),
  'sparsity_parameters': {}},
 {'dtype': numpy.int8,
  'index': 4,
  'name': 'sequential/dense/MatMul',
  'quantization': (0.004050740040838718, 0),
  'quantization_parameters': {'quantized_dimension': 0,
   'scales': array([0.00405074], dtype=float32),
   'zero_points': array([0], dtype=int32)},
  'shape': array([ 10, 150], dtype=int32),
  'shape_signature': array([ 10, 150], dtype=int32),
  'sparsity_parameters': {}},
 {'dtype': numpy.int8,
  'index': 7,
  'name': 'sequential/dense/Relu',
  'quantization': (1.188883662223816, -128),
  'quantization_parameters': {'quantized_dimension': 0,
   'scales': array([1.1888837], dtype=float32),
   'zero_points': array([-128], dtype=int32)},
  'shape': array([ 1, 10], dtype=int32),
  'shape_signature': array([ 1, 10], dtype=int32),
  'sparsity_parameters': {}}
```

We can see each tensor has it's own scale and zero point to represent real values. 

For a fully connected layer, each neuron still calculates the value in the same way. 

![Output_{real} = Input_{real0}*Weight_{real0} + Input_{real1}*Weight_{real1} + ... + Input_{realN}*Weight_{realN} + Bias_{real}](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/fullyconnectFunc1.gif)

or

![O_{r} = I_{r0}*W_{r0} + I_{r1}*W_{r1} + ... + I_{rN}*W_{rN} + B_{r}
](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/fullyconnectFunc2.gif)

We want the real values, but we only have quantized values now. If we replace every real value with the quantization equation, we get:

![s_{o}(Q_{o}-z_{o}) =s_{i}(Q_{i0}-z_{i})*s_{w}(Q_{w0}-z_{w}) + s_{i}(Q_{i1}-z_{i})*s_{w}(Q_{w1}-z_{w}) + ... + s_{i}(Q_{iN}-z_{i})*s_{w}(Q_{wN}-z_{w}) + s_{b}(Q_{b}-z_{b}) \newline = s_is_w((Q_{i0}-z_{i})(Q_{w0}-z_{w})+(Q_{i1}-z_{i})(Q_{w1}-z_{w})+...+(Q_{iN}-z_{i})(Q_{wN}-z_{w}))+s_{b}(Q_{b}-z_{b}) \newline = s_is_w\sum_{n=0}^{N}(Q_{in}-z_{i})(Q_{wn}-z_{w})+s_{b}(Q_{b}-z_{b})](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/fullyconnectFunc3.gif)

I don't quite like the Sigma symbol but it does make the equation shorter. 

Referring to ```GetQuantizedConvolutionMultipler``` function in ```kernel_util.cc```, it mentioned *"input_product_scale & bias_scale the same. ... guaranteed by the training pipeline"*. Also, the zero point of bias tensor seems always be zero.

In Tensorflow Lite, the scale of weight tensor is the same as the scale of bias tensor, and the zero point of bias tensor is always zero. 

![s_{b}=s_is_w](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/fullyconnectFunc4.gif)

So we can simplify the equation to:

![s_{o}(Q_{o}-z_{o}) = s_is_w\sum_{n=0}^{N}(Q_{in}-z_{i})(Q_{wn}-z_{w})+s_is_w(Q_{b}-z_{b})
 \newline = s_is_w(\sum_{n=0}^{N}(Q_{in}-z_{i})(Q_{wn}-z_{w})+(Q_{b}-z_{b}))](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/fullyconnectFunc5.gif)

In a quantized calculation, we need to get Qo as the quantized value of a neuron.

![Q_{o}= \frac{s_is_w}{s_{o}}(\sum_{n=0}^{N}(Q_{in}-z_{i})(Q_{wn}-z_{w})+(Q_{b}-z_{b}))+z_{o}](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/fullyconnectFunc6.gif)

Everything is quite straight forward in integer calculation, except for the ```si*sw/so``` part. That is called a "QuantizedConvolutionMultipler" in the Tensorflow Lite code. For any layer in any given model, these scale numbers are constants. So we can convert this float calculation into multiply and shift operations for faster speed. In Tensorflow Lite, the interpreter does the conversion in the preparation stage. And we can do it in python, then export multiplier and shift values for Arduino to use.

![\frac{s_is_w}{s_{o}}=m\times2^n=\frac{m\times2^{15}}{2^{15-n}}=(m\times2^{15})>>(15-n)
](https://raw.githubusercontent.com/DeqingSun/Machine-Learning-Experiment-on-8-bit-Arduino/master/images/fullyconnectFunc7.gif)

We can use ```frexp``` function to convert a float number into a normalized fraction (mantissa * 2 ^ exponent). Just make sure ```m``` is larger or equal to 1/2 and less than 1, then ```m*2^15``` will be less or equal to 32767, which fits into an int16 type variable. That's how we use integer multiplication and shift to replace a float multiplication.



 



