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

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofSetWindowTitle("Language Identifier");
	ofSetCircleResolution(80);
	ofBackground(54, 54, 54);

	// load the model, bail out on error
	//std::string modelName = "model_4lang";  // model v1
	std::string modelName = "model_attrnn"; // model v2
	if(!model.load(modelName)) {
		std::exit(EXIT_FAILURE);
	}

	// audio stream settings
	bufferSize = 1023;
	samplingRate = 48000; // take 16kHz if available then set downsamplingFactor to 1

	// Neural network input parameters
	// downsamplingFactor must be an integer of samplingRate / inputSamplingeRate
	// downsampling is required for microphones that do not have 16kHz sampling
	downsamplingFactor = 3;	
	inputSeconds = 5;
	inputSamplingRate = 16000; // shall not be changed, AI was trained on 16kHz

	// recording settings
	numPreviousBuffers = 10; // how many buffers to save before trigger happens
	numBuffers = samplingRate * inputSeconds / bufferSize;
	previousBuffers.setMaxLen(numPreviousBuffers);
	sampleBuffers.setMaxLen(numBuffers);

	// display 
	volHistory.assign(400, 0.0);

	// apply settings to soundStream 
	soundStream.printDeviceList();
	ofSoundStreamSettings settings;
	auto devices = soundStream.getMatchingDevices("default");
	if(!devices.empty()) {
		ofLog() << "input device: " << devices[0].name;
		settings.setInDevice(devices[0]);
	}
	settings.setInListener(this);
	settings.sampleRate = samplingRate;
	settings.numOutputChannels = 0;
	settings.numInputChannels = 1;
	settings.bufferSize = bufferSize;
	soundStream.setup(settings);

	// print language labels we know
	ofLog() << "From src/Labels.h:";
	ofLog() << "----> detected languages";
	for(const auto & label : labelsMap) {
		ofLog() << label.second;
	}
	ofLog() << "<---- detected languages";

	// warm up: inital inference involves initalization (takes longer)
	auto test = cppflow::fill({1, 80000, 1}, 1.0f);
	output = model.runModel(test);
	ofLog() << "Setup done";
	ofLog() << "============================";

	// osc
	for(auto host : hosts) {
		ofxOscSender *sender = new ofxOscSender;
		sender->setup(host.address, host.port);
		senders.push_back(sender);
	}
}

//--------------------------------------------------------------
void ofApp::update() {

	// lets scale the vol up to a 0-1 range 
	scaledVol = ofMap(smoothedVol, 0.0, 0.17, 0.0, 1.0, true);
	// lets record the volume into an array
	volHistory.push_back(scaledVol);
	// if we are bigger than the size we want to record - lets drop the oldest value
	if(volHistory.size() >= 400) {
		volHistory.erase(volHistory.begin(), volHistory.begin()+1);
	}

	if(trigger) {
		// inference, sets argMax and prob after running model
		int argMax;
		float prob;
		model.classify(sampleBuffers, downsamplingFactor, argMax, prob);

		// only send & display label when probabilty is high enough
		if(prob >= minConfidence) {
			displayLabel = labelsMap[argMax];
			ofxOscMessage message;
			message.setAddress("/lang");
			message.addIntArg(argMax);
			message.addStringArg(labelsMap[argMax]);
			message.addFloatArg(prob * 100);
			for(auto sender: senders) {sender->sendMessage(message);}
		}
		else {
			displayLabel = " ";
		}

		// look up label
		ofLog() << "Label: " << labelsMap[argMax];
		ofLog() << "Probabilty: " << prob;
		ofLog() << "============================";

		// release the trigger signal and emit enable
		trigger = false;
		enable = true;

		// detection stopped
		ofxOscMessage message;
		message.setAddress("/detecting");
		message.addIntArg(0);
		for(auto sender: senders) {sender->sendMessage(message);}
	}

	if(recordingStarted) {
		// detection started
		ofxOscMessage message;
		message.setAddress("/detecting");
		message.addIntArg(1);
		for(auto sender: senders) {sender->sendMessage(message);}
		recordingStarted = false;
	}
}

//--------------------------------------------------------------
void ofApp::draw() {

	std::size_t historyWidth = 400;
	std::size_t historyHeight = 150;

	// draw current label status
	ofSetColor(64, 245, 221);
	ofNoFill();
	ofDrawBitmapString(displayLabel, 50, 50);

	// draw the average volume
	ofPushStyle();
		ofPushMatrix();
		ofTranslate(50, 50);

		// draw the threshold line
		ofDrawLine(0, historyHeight - volThreshold, 
		              historyWidth, historyHeight - volThreshold);

		ofSetColor(255);
		
		// lets draw the volume history as a graph
		ofBeginShape();
		for(unsigned int i = 0; i < volHistory.size(); i++) {
			if(i == 0) {
				ofVertex(i, historyHeight);
			}
			ofVertex(i, historyHeight - volHistory[i] * 100);
			if(i == volHistory.size() - 1) {
				ofVertex(i, historyHeight);
			}
		}
		ofEndShape(false);
			
		ofPopMatrix();
	ofPopStyle();

	// draw recording status
	if(recording) {
		if(ofGetElapsedTimef() - blinkTimestamp >= 0.5) {
			blink = !blink;
			blinkTimestamp = ofGetElapsedTimef();
		}
		if(blink) {
			ofSetColor(245, 64, 64);
			ofFill();
			ofDrawCircle(ofGetWidth() - 50, 50, 6);
		}
	}
}

//--------------------------------------------------------------
void ofApp::exit() {
	ofxOscMessage message;
	message.setAddress("/detecting");
	message.addIntArg(0);
	for(auto sender: senders) {
		sender->sendMessage(message);
		delete sender;
	}
	senders.clear();
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer & input) {

	// calculate the root mean square which is a rough way to calculate volume
	float sumVol = 0.0;
	for(size_t i = 0; i < input.getNumFrames(); i++) {
		float vol = input[i];
		sumVol += vol * vol;
	}
	curVol = sumVol / (float)input.getNumFrames();
	curVol = sqrt(curVol);
	// smoothen the volume
	smoothedVol *= 0.5;
	smoothedVol += 0.5 * curVol;

	// trigger recording if the smoothed volume is high enough
	if(ofMap(smoothedVol, 0.0, 0.17, 0.0, 1.0, true) * 100 >= volThreshold && enable) {
		enable = false;
		ofLog() << "Start recording...";
		// copy previous buffers to the recording
		sampleBuffers = previousBuffers;
		sampleBuffers.setMaxLen(numBuffers); // just to make sure (not tested)
		recordingCounter = sampleBuffers.size();
		// trigger recording in the next function call
		recording = true;
		recordingStarted = true;
		blink = true;
		blinkTimestamp = ofGetElapsedTimef();
	}
	// if we didnt just trigger
	else { 
		// if recording: save the incoming buffer to the recording
		// then trigger the neural network
		if(recording) {
			sampleBuffers.push(input.getBuffer());
			recordingCounter++;
			if(recordingCounter >= numBuffers) {
				recording = false;
				trigger = true;
				ofLog() << "Done!";
			}
		}
		// if not recording: save the incoming buffer to the previous buffer fifo
		else {
			previousBuffers.push(input.getBuffer());
		}

	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
