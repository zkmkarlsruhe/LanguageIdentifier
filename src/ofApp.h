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

#include "ofMain.h"
#include "ofxTensorFlow2.h"
#include "ofxOsc.h"

#include "AudioClassifier.h"
#include "Labels.h"

// autotools-style config.h defines
#define PACKAGE "LanguageIdentifier"
#define VERSION "0.3.0"
#define DESCRIPTION "identifies spoken language from audio stream"

class ofApp : public ofBaseApp {

	public:

		void setup();
		void update();
		void exit();
		void draw();

		void audioIn(ofSoundBuffer & input);

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		/// start listening
		void startListening();

		/// stop listening
		void stopListening();

		/// osc receiver callback
		void oscReceived(const ofxOscMessage &message);

		// audio 
		ofSoundStream soundStream;
		int inputDevice = -1;
		int inputChannel = 0;
		std::vector<float> monoBuffer; //< mono inputChannel stream buffer
		bool listening = true;

		// neural network input parameters
		// for ease of use:
		// we want to keep the buffersize a multiple of the downsampling factor
		// downsamplingFactor = sampleRate / modelSampleRate
		// downsampling is required for microphones that do not have 16kHz sampling
		std::size_t bufferSize = 1024;
		std::size_t sampleRate = 48000;
		std::size_t downsamplingFactor = 3;

		static const std::size_t modelSampleRate; //< sample rate expected by model
		
		// since volume detection has some latency we keep a history of buffers
		AudioBufferFifo previousBuffers;
		std::size_t numPreviousBuffers = 10; // how many buffers to save before trigger happens
		// sampleBuffers acts as a buffer for recording (could be fused)
		AudioBufferFifo sampleBuffers;
		std::size_t numBuffers;
		
		// volume
		float curVol = 0.0;
		float smoothedVol = 0.0;
		float scaledVol = 0.0;
		float volThreshold = 25;

		// display
		std::vector<float> volHistory;
		std::string displayLabel = " ";

		// neural network	
		AudioClassifier model;
		cppflow::tensor output;
		std::size_t inputSeconds = 5;
		const std::size_t inputSamplingRate = 16000; // AI was trained on 16kHz
		std::size_t inputSize;
		float minConfidence = 0.75;

		// neural network control logic
		std::size_t recordingCounter = 0;
		bool trigger = false;
		bool enable = true;
		bool autostop = false;
		bool recording = false;
		bool blink = true; // recording blink state
		float blinkTimestamp = 0; // blink timestamp

		// osc
		typedef struct OscHost {
			std::string address;
			int port;
			OscHost(std::string a, int p) : address(a), port(p) {}
		} OscHost;
		std::vector<OscHost> hosts = {};
		std::vector<ofxOscSender*> senders;
		ofxOscReceiver receiver;
		int port = 6006;
		bool recordingStarted = false;
};
