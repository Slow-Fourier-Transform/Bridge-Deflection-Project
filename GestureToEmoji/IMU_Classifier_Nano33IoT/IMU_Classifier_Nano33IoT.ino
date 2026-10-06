/*
  IMU Classifier for the Arduino Nano 33 IoT

  This example uses the on-board IMU to start reading acceleration and gyroscope
  data, once enough samples are read it runs the trained gesture model on them and
  prints a score for each gesture to the Serial Monitor.

  The Nano 33 IoT is too small for the TensorFlow Lite Micro library, so the model
  is not run by TensorFlow. The notebook writes the weights of the three dense
  layers (relu, relu, softmax) into model_weights.h, and this sketch does the
  arithmetic itself.

  Put model_weights.h, downloaded from the notebook, in the same folder as this sketch.

  The circuit:
  - Arduino Nano 33 IoT board.

  Based on the IMU Classifier example by Don Coleman, Sandeep Mistry and Dominic Pajak.
  This example code is in the public domain.
*/

#include <Arduino_LSM6DS3.h>
#include <math.h>

#include "model_weights.h"

const float accelerationThreshold = 2.5; // threshold of significant in G's
const int numSamples = 119;

int samplesRead = numSamples;

// the model's input is numSamples samples of aX, aY, aZ, gX, gY, gZ
static_assert(N_IN == numSamples * 6, "model_weights.h was made for a different number of samples");
static_assert(sizeof(GESTURES) / sizeof(GESTURES[0]) == N_OUT, "model_weights.h has a different number of gestures than outputs");

float input[N_IN];
float hidden1[N_H1];
float hidden2[N_H2];
float scores[N_OUT];

// out = in * weights + bias, with weights stored as [input][output], optionally followed by relu
void dense(const float* in, int numIn, const float* weights, const float* bias, float* out, int numOut, bool relu) {
  for (int j = 0; j < numOut; j++) {
    out[j] = bias[j];
  }
  for (int i = 0; i < numIn; i++) {
    const float x = in[i];
    const float* row = weights + i * numOut;
    for (int j = 0; j < numOut; j++) {
      out[j] += x * row[j];
    }
  }
  if (relu) {
    for (int j = 0; j < numOut; j++) {
      if (out[j] < 0) {
        out[j] = 0;
      }
    }
  }
}

// turns the raw values into scores that are between 0 and 1 and add up to 1
void softmax(float* values, int count) {
  float largest = values[0];
  for (int i = 1; i < count; i++) {
    if (values[i] > largest) {
      largest = values[i];
    }
  }
  float sum = 0;
  for (int i = 0; i < count; i++) {
    values[i] = expf(values[i] - largest);
    sum += values[i];
  }
  for (int i = 0; i < count; i++) {
    values[i] /= sum;
  }
}

void classify() {
  dense(input, N_IN, layer1_weights, layer1_bias, hidden1, N_H1, true);
  dense(hidden1, N_H1, layer2_weights, layer2_bias, hidden2, N_H2, true);
  dense(hidden2, N_H2, layer3_weights, layer3_bias, scores, N_OUT, false);
  softmax(scores, N_OUT);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // print out the samples rates of the IMUs
  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  Serial.println();
}

void loop() {
  float aX, aY, aZ, gX, gY, gZ;

  // wait for significant motion
  while (samplesRead == numSamples) {
    if (IMU.accelerationAvailable()) {
      // read the acceleration data
      IMU.readAcceleration(aX, aY, aZ);

      // sum up the absolutes
      float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

      // check if it's above the threshold
      if (aSum >= accelerationThreshold) {
        // reset the sample read count
        samplesRead = 0;
        break;
      }
    }
  }

  // check if the all the required samples have been read since
  // the last time the significant motion was detected
  while (samplesRead < numSamples) {
    // check if new acceleration AND gyroscope data is available
    if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
      // read the acceleration and gyroscope data
      IMU.readAcceleration(aX, aY, aZ);
      IMU.readGyroscope(gX, gY, gZ);

      // normalize the IMU data between 0 to 1 and store in the model's input
      input[samplesRead * 6 + 0] = (aX + 4.0) / 8.0;
      input[samplesRead * 6 + 1] = (aY + 4.0) / 8.0;
      input[samplesRead * 6 + 2] = (aZ + 4.0) / 8.0;
      input[samplesRead * 6 + 3] = (gX + 2000.0) / 4000.0;
      input[samplesRead * 6 + 4] = (gY + 2000.0) / 4000.0;
      input[samplesRead * 6 + 5] = (gZ + 2000.0) / 4000.0;

      samplesRead++;

      if (samplesRead == numSamples) {
        // Run inferencing
        classify();

        // Loop through the scores from the model
        for (int i = 0; i < N_OUT; i++) {
          Serial.print(GESTURES[i]);
          Serial.print(": ");
          Serial.println(scores[i], 6);
        }
        Serial.println();
      }
    }
  }
}
