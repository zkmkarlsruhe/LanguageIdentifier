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
#include "ThreadPool.h"

const std::size_t ofApp::modelSampleRate = 16000;

// command worker task
void executeCommand(std::string command) {
	ofLogVerbose(PACKAGE) << command;
	ofSystem(command);
}

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofSetWindowTitle(PACKAGE);
	ofSetCircleResolution(80);
	ofBackground(54, 54, 54);

	// load the model, bail out on error
	#ifdef USE_MODEL_V1
		std::string modelName = "model_4lang";  // model v1
	#else
		std::string modelName = "model_7lang";  // model v2
	#endif
	if(!model.load(modelName)) {
		std::exit(EXIT_FAILURE);
	}

	// recording settings
	numBuffers = sampleRate * inputSeconds / bufferSize;
	previousBuffers.setMaxLen(numPreviousBuffers);
	sampleBuffers.setMaxLen(numBuffers);
	ofLogVerbose(PACKAGE) << "Looking " << std::to_string(numPreviousBuffers) << " into the past"
					<< " and recording a total of " << std::to_string(numBuffers) << " buffers"
					<< " each with " << std::to_string(bufferSize) << " samples"; 

	// apply settings to soundStream
	ofSoundStreamSettings settings;
	if(inputDevice < 0) {
		// find default input device
		auto devices = soundStream.getDeviceList();
		for(int i = 0; i < devices.size(); ++i) {
			auto device = devices[i];
			if(device.isDefaultInput) {
				inputDevice = i;
				break;
			}
		}
		if(inputDevice < 0) {
			ofLogError(PACKAGE) << "no audio input device";
			std::exit(EXIT_FAILURE);
		}
	}
	auto devices = soundStream.getDeviceList();
	ofSoundDevice &device = devices[inputDevice];
	ofLogNotice(PACKAGE) << "audio input device: " << inputDevice << " " << device.name;
	if(inputChannel >= device.inputChannels) {
		ofLogWarning(PACKAGE) << "audio input device does not have enough input channels";
		inputChannel = 0;
	}
	ofLogNotice(PACKAGE) << "audio input channel: " << inputChannel;
	ofLogNotice(PACKAGE) << "audio input samplerate: " << sampleRate;
	ofLogNotice(PACKAGE) << "audio input buffer size: " << bufferSize;
	settings.setInDevice(device);
	settings.setInListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = 0;
	settings.numInputChannels = inputChannel + 1;
	settings.bufferSize = bufferSize * (inputChannel + 1);
	if(!soundStream.setup(settings)) {
		ofLogError(PACKAGE) << "audio input device " << inputDevice << " setup failed";
		ofLogError(PACKAGE) << "perhaps try a different device or samplerate?";
		std::exit(EXIT_FAILURE);
	}
	monoBuffer.resize(bufferSize);
	if(!listening) {
		soundStream.stop();
	}
	if(soundStream.getSampleRate() == 44100) {
		ofLogWarning(PACKAGE) << "treating sample rate of 44100 as 48000, may or may not affect detection";
	}

	// display
	volHistory.assign(400, 0.0);

	// print language labels we know
	ofLogVerbose(PACKAGE) << "----> detected languages";
	for(const auto & label : labelsMap) {
		ofLogVerbose(PACKAGE) << label.second;
	}
	ofLogVerbose(PACKAGE) << "<---- detected languages";

	// warm up: inital inference involves initalization (takes longer)
	auto test = cppflow::fill({1, 80000, 1}, 1.0f);
	output = model.runModel(test);

	// osc
	ofLogNotice(PACKAGE) << hosts.size() << " osc sender host(s)";
	for(auto host : hosts) {
		ofxOscSender *sender = new ofxOscSender;
		if(sender->setup(host.address, host.port)) {
			senders.push_back(sender);
			ofLogNotice(PACKAGE) << "  " << host.address << " " << host.port;
		}
	}
	ofLogNotice(PACKAGE) << "osc receiver port " << port;
	receiver.setup(port);

	// behavior
	if(!listening) {
		ofLogNotice(PACKAGE) << "no listen: true";
	}
	if(autostop) {
		ofLogNotice(PACKAGE) << "auto stop: true";
	}

	// command?
	if(command != "") {
		commandPool = new ThreadPool();
	}

	ofLogVerbose(PACKAGE) << "setup done";
	ofLogVerbose(PACKAGE) << "============================";
}

//--------------------------------------------------------------
void ofApp::update() {

	// process received osc events
	while(receiver.hasWaitingMessages()) {
		ofxOscMessage message;
		if(receiver.getNextMessage(message)) {
			oscReceived(message);
		}
	}

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
		std::vector<float> outputVector;
		model.classify(sampleBuffers, downsamplingFactor, argMax, prob, outputVector);

		// only send & display label when probabilty is high enough
		bool detected = false;
		if(prob >= minConfidence) {
			displayLabel = labelsMap[argMax];

			// send osc
			ofxOscMessage message;
			message.setAddress("/lang");
			message.addIntArg(argMax);
			message.addStringArg(displayLabel);
			message.addFloatArg(prob * 100);
			for(auto sender: senders) {sender->sendMessage(message);}

			// execute command in worker thread?
			if(command != "") {
				std::string exec = command + " selected=" + displayLabel +
				                   " " + resultToString(outputVector);
				commandPool->schedule(std::bind(executeCommand, exec));
			}

			detected = true;
		}
		else {
			displayLabel = " ";
		}

		// look up label
		ofLogVerbose(PACKAGE) << "label: " << labelsMap[argMax];
		ofLogVerbose(PACKAGE) << "confidence: " << ofToString(prob * 100, 2);
		ofLogVerbose(PACKAGE) << "============================";

		// release the trigger signal and emit enable
		trigger = false;
		enable = true;

		// detection stopped
		ofxOscMessage message;
		message.setAddress("/detecting");
		message.addIntArg(0);
		for(auto sender: senders) {sender->sendMessage(message);}

		// stop after (successful) detection?
		if(autostop && detected) {
			stopListening();
		}
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
		
		// lets draw the volume history as a graph
		ofSetColor(255);
		ofBeginShape();
		for(unsigned int i = 0; i < volHistory.size(); i++) {
			float y = historyHeight - volHistory[i] * 100;
			ofVertex(i, y);
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
	if(commandPool) {
		delete commandPool;
		commandPool = nullptr;
	}
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer & input) {
	// beh, ofSoundBuffer::getNumFrames() actually returns the buffer size?
	std::size_t numFrames = input.getNumFrames() / input.getNumChannels();

	// copy desired channel out of interleaved stream into mono buffer,
	// assume input stream has enough channels...
	for(std::size_t i = 0; i < numFrames; i++) {
		monoBuffer[i] = input[(i*input.getNumChannels())+inputChannel];
	}

	// calculate the root mean square which is a rough way to calculate volume
	float sumVol = 0.0;
	for(std::size_t i = 0; i < monoBuffer.size(); i++) {
		float vol = monoBuffer[i];
		sumVol += vol * vol;
	}
	curVol = sumVol / (float)monoBuffer.size();
	curVol = sqrt(curVol);
	// smooth the volume
	smoothedVol *= 0.5;
	smoothedVol += 0.5 * curVol;

	// trigger recording if the smoothed volume is high enough
	if(ofMap(smoothedVol, 0.0, 0.17, 0.0, 1.0, true) * 100 >= volThreshold && enable) {
		enable = false;
		ofLogVerbose(PACKAGE) << "start recording...";
		// copy previous buffers to the recording
		sampleBuffers = previousBuffers;
		sampleBuffers.setMaxLen(numBuffers);		// hacky: last step overwrites maxLen
		recordingCounter = sampleBuffers.size();	// we already have the previous buffer
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
			sampleBuffers.push(monoBuffer);
			recordingCounter++;
			if(recordingCounter >= numBuffers) {
				recording = false;
				trigger = true;
				ofLogVerbose(PACKAGE) << "done!";
			}
		}
		// if not recording: save the incoming buffer to the previous buffer fifo
		else {
			previousBuffers.push(monoBuffer);
		}
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch(key) {
		case 'l':
			if(listening) {
				stopListening();
			}
			else {
				startListening();
			}
			break;
		case 'a':
			if(autostop) {
				disableAutostop();
			}
			else {
				enableAutostop();
			}
			break;
	}
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

//--------------------------------------------------------------
void ofApp::startListening() {
	enable = true;
	soundStream.start();
	listening = true;
	ofLogVerbose(PACKAGE) << "listening " << listening;
}

//--------------------------------------------------------------
void ofApp::stopListening() {
	soundStream.stop();
	previousBuffers.clear();
	sampleBuffers.clear();
	smoothedVol = 0;
	enable = false;
	if(recording) {
		recording = false;
		// detection stopped
		ofxOscMessage message;
		message.setAddress("/detecting");
		message.addIntArg(0);
		for(auto sender: senders) {sender->sendMessage(message);}
	}
	trigger = false;
	listening = false;
	ofLogVerbose(PACKAGE) << "listening " << listening;
}

//--------------------------------------------------------------
void ofApp::enableAutostop() {
	if(!autostop) {
		ofLogVerbose(PACKAGE) << "autostop " << autostop;
	}
	autostop = true;
}

//--------------------------------------------------------------
void ofApp::disableAutostop() {
	if(autostop) {
		ofLogVerbose(PACKAGE) << "autostop " << autostop;
	}
	autostop = false;
}

//--------------------------------------------------------------
void ofApp::oscReceived(const ofxOscMessage &message) {
	if(message.getAddress() == "/listen") {
		if(message.getNumArgs() == 0) {
			if(!listening) {
				startListening();
			}
		}
		else if(message.getNumArgs() == 1) {
			if(message.getArgAsBool(0)) {
				startListening();
			}
			else {
				stopListening();
			}
		}
	}
	else if(message.getAddress() == "/autostop") {
		if(message.getNumArgs() == 0) {
			enableAutostop();
		}
		else if(message.getNumArgs() == 1) {
			if(message.getArgAsBool(0)) {
				enableAutostop();
			}
			else {
				disableAutostop();
			}
		}
	}
}

//--------------------------------------------------------------
std::string ofApp::resultToString(std::vector<float> outputVector) {
	std::string result = "";
	for(size_t i = 0; i < outputVector.size(); i++) {
		result += labelsMap[i] + "=" + std::to_string(outputVector[i]);
		if(i < outputVector.size()-1) {
			result += " ";
		}
	}
	return result;
}
