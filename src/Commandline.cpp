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

#include "Commandline.h"

Commandline::Commandline(ofApp *app) : app(app) {
	parser.description(DESCRIPTION);
}

bool Commandline::parse(int argc, char **argv) {

	// local options, the rest are ofApp instance variables
	std::vector<std::string> senders;
	bool list = false;
	int inputNum = -1;
	std::string inputName = "";
	int inputChannel = 0;
	int sampleRate = 0;
	bool verbose = false;
	bool version = false;

	parser.add_option("-s,--senders", senders,
		"OSC sender addr:port host pairs, ex. \"192.168.0.100:5555\" "
		"or multicast \"239.200.200.200:6666\", default \"localhost:9999\"")->expected(-1);
	parser.add_option("-c,--confidence", app->minConfidence,
		"min confidence, default " + ofToString(app->minConfidence))->transform(CLI::Bound(0.0, 1.0));
	parser.add_option("-t,--threshold", app->volThreshold,
	    "volume threshold, default " + ofToString(app->volThreshold))->transform(CLI::Bound(0, 100));
	parser.add_flag(  "-l,--list", list, "list audio input devices and exit");
	parser.add_option("--inputdev", inputNum, "audio input device number");
	parser.add_option("--inputname", inputName, "audio input device name, can do partial match, ex. \"Microphone\"");
	parser.add_option("--inputchan", inputChannel, "audio input device channel, default 1");
	parser.add_option("-r,--samplerate", sampleRate, "audio input device samplerate, can be 441000 or a multiple of " +
		ofToString(ofApp::modelSampleRate) +  ", default " + ofToString(app->sampleRate));
	parser.add_flag(  "-v,--verbose", verbose, "verbose printing");
	parser.add_flag(  "--version", version, "print version and exit");

	try {
		parser.parse(argc, argv);
	}
	catch(const CLI::ParseError &e) {
		error = e;
		return false;
	}

	// verbose printing?
	ofSetLogLevel(PACKAGE, (verbose ? OF_LOG_VERBOSE : OF_LOG_NOTICE));

	// print version
	if(version) {
		std::cout << VERSION << std::endl;
		return false;
	}

	// list audio input devices
	if(list) {
		auto devices = app->soundStream.getDeviceList();
		std::size_t count = 0;
		std::cout << "input devices (# NAME [CHANNELS]):" << std::endl;
		for(std::size_t i = 0; i < devices.size(); i++) {
			auto device = devices[i];
			if(device.inputChannels > 0) {
				std::cout << i << " " << device.name
						  << " [" << device.inputChannels << "]" << std::endl;
				count++;
			}
		}
		if(count == 0) {
			std::cout << "none" << std::endl;
		}
		return false;
	}

	// set audio input from device number
	if(inputNum >= 0) {
		auto devices = app->soundStream.getDeviceList();
		if(inputNum >= devices.size()) {
			ofLogError(PACKAGE) << "invalid audio device number: " << inputNum;
			error = CLI::RuntimeError("invalid audio device number", EXIT_FAILURE);
			return false;
		}
		ofSoundDevice &device = devices[inputNum];
		if(device.inputChannels == 0) {
			ofLogError(PACKAGE) << "audio device " << inputNum << " has no input channels";
			error = CLI::RuntimeError("audio device has no input channels", EXIT_FAILURE);
			return false;
		}
		app->inputDevice = inputNum;
	}

	// set audio input from device name
	if(inputName != "") {
		inputNum = -1;
		auto devices = app->soundStream.getDeviceList();
		for(std::size_t i = 0; i < devices.size(); ++i) {
			auto device = devices[i];
			if(device.name.find(inputName) != std::string::npos && device.inputChannels > 0) {
				inputNum = i;
				break;
			}
		}
		if(inputNum >= 0) {
			app->inputDevice = inputNum;
		}
		else {
			ofLogWarning(PACKAGE) << "audio input name not found: " << inputName;
		}
	}

	// set audio input channel
	if(inputChannel > 0) {
		app->inputChannel = inputChannel-1; // 1-index to 0-index
	}

	// set audio input rate
	if(sampleRate > 0) {
		bool set = true;
		if(sampleRate == 44100) {
			// treat as 48k default, pitch change is minimal enough to not affect detection
			// and we don't handle non-integer downsampling factors
			app->sampleRate = sampleRate;
			app->downsamplingFactor = 3;
			set = false;
		}
		else if(sampleRate % ofApp::modelSampleRate != 0) {
			ofLogWarning(PACKAGE) << "ignoring input sample rate which is not a multiple of "
			                      << ofApp::modelSampleRate << ": " << sampleRate;
		}
		if(set) {
			app->sampleRate = sampleRate;
			app->downsamplingFactor = sampleRate / ofApp::modelSampleRate;
		}
	}

	// parse sender host strings
	// split string by last : to get address & port pair,
	// handle bracketed IPv6 hostnames: [::1]:8081
	for(auto host : senders) {
		std::size_t found = host.find_last_of(":");
		if(found == std::string::npos) {
			ofLogWarning(PACKAGE) << "ignoring sender host without port: " << host;
			continue;
		}
		std::string addr = host.substr(0, found);
		std::string port = host.substr(found+1);
		if(addr.size() == 0 || port.size() == 0) {
			ofLogWarning(PACKAGE) << "ignoring sender host with empty address or port: " << host;
			continue;
		}
		if(addr[0] == '[' && addr[addr.size()-1] == ']') {
			addr = addr.substr(1, addr.size()-2);
		}
		int p = ofToInt(port);
		if(p <= 1024) {
			ofLogWarning(PACKAGE) << "ignoring sender host with invalid port or system port: " << host;
			continue;
		}
		app->hosts.push_back(ofApp::OscHost(addr, p));
	}
	if(app->hosts.empty()) { // default
		app->hosts.push_back(ofApp::OscHost("localhost", 9999));
	}

	return true;
}

int Commandline::exit() {
	return parser.exit(error);
}
