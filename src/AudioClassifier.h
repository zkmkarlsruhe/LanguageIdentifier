/*
 * Language Identifier
 *
 * Copyright (c) 2021 ZKM | Hertz-Lab
 * Paul Bethge <bethge@zkm.de>
 * Dan Wilcox <dan.wilcox@zkm.de>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * This code has been developed at ZKM | Hertz-Lab as part of „The Intelligent 
 * Museum“ generously funded by the German Federal Cultural Foundation.
 */

#pragma once

#include <queue>
#include <deque>
#include <iostream>

#include "ofxTensorFlow2.h"
#include "ofFileUtils.h"

// uncomment to write recorded audio samples to bin/data/test.wav
//#define DEBUG_WAVE
#ifdef DEBUG_WAVE
#include "WavFileWriterBeta.h"
#endif

/// a simple Fifo with adjustable max length
template <typename T, typename Container=std::deque<T>>
class FixedFifo : public std::queue<T, Container> {

	public:

		FixedFifo(const std::size_t maxLength=10) : maxLen(maxLength) {}

		void push(const T& value) {
			if(this->size() == maxLen) {
				this->c.pop_front();
			}
			std::queue<T, Container>::push(value);
		}

		void clear() {
			while(!this->empty()) {
				this->pop();
			}
		}

		void setMaxLen(const std::size_t maxLength) {
			maxLen = maxLength;
		}

	private:

		std::size_t maxLen;
};

typedef std::vector<float> SimpleAudioBuffer;
typedef FixedFifo<SimpleAudioBuffer> AudioBufferFifo;

/// custom ofxTF2::Model implementation to handle audio sample conversion, etc
class AudioClassifier : public ofxTF2::Model {

	public:

		void classify(AudioBufferFifo & bufferFifo, const std::size_t downsamplingFactor,
					  int & argMax, float & prob, std::vector<float>  & outputVector) {

			SimpleAudioBuffer sample;

			// downsample and empty the incoming Fifo
			downsample(bufferFifo, sample, downsamplingFactor);
			normalize(sample);

#ifdef DEBUG_WAVE
			sakado::WavFileWriterBeta wfw(ofToDataPath("test.wav"), 1, 16000, 2, sample.size());
			int16_t buf;
			for(int i = 0; i < sample.size(); i++) {
				buf = sample[i] * 25500; // scale data to int16 range
				wfw.write(&buf, 2, 1);
			}
			wfw.close();
#endif

			// convert recorded sample to a batch of size one
			ofxTF2::shapeVector tensorShape {1, static_cast<ofxTF2::shape_t>(sample.size()), 1};
			auto input = ofxTF2::vectorToTensor(sample, tensorShape);

			// inference
			auto output = runModel(input);

			// convert the output to std::vector
			ofxTF2::tensorToVector(output, outputVector);

			// get element with highest probabilty
			auto maxIt = std::max_element(outputVector.begin(), outputVector.end());
			argMax = std::distance(outputVector.begin(), maxIt);
			prob = *maxIt;
		}

	private:

		// inplace normalization
		void normalize(SimpleAudioBuffer & sample) {
			// find absolute maximum value
			float max = 0.0;
			for(const auto& s : sample) {
				if(abs(s) > max) {
					max = abs(s);
				}
			}
			if(max == 0.0) {
				return;
			}
			for(auto&& s : sample) {
				s /= max;
			}
		}

		// downsample by an integer
		void downsample(AudioBufferFifo & bufferFifo, SimpleAudioBuffer & sample,
						const std::size_t downsamplingFactor) {

			// get the size of an element
			const std::size_t bufferSize = bufferFifo.front().size();
			const std::size_t bufferSizeDownsampled = bufferSize / downsamplingFactor;

			// allocate memory if neccessary
			sample.resize(bufferFifo.size() * bufferSizeDownsampled);

			// pop elements from the bufferFifo, downsample and save to flat buffer
			std::size_t i = 0;
			while(!bufferFifo.empty()) {

				// get a buffer from fifo
				const SimpleAudioBuffer & buffer = bufferFifo.front();

				// downsample by integer
				for(std::size_t j = 0; j < bufferSizeDownsampled; j++) {
					std::size_t offset = j * downsamplingFactor;
					float sum = 0.0;
					for(std::size_t k = 0; k < downsamplingFactor; k++) {
						sum += buffer[offset+k];
					}
					sample[i*bufferSizeDownsampled + j] = sum / downsamplingFactor;
				}
				// remove buffer from fifo
				bufferFifo.pop();
				i++;
			}
		}
};
