#include "ofMain.h"
#include "ofApp.h"

#include "CLI11.hpp"

//========================================================================
int main(int argc, char **argv) {
	ofApp *app = new ofApp();

	// parse commandline arguments
	CLI::App parser{"Listens to audio stream, identifies spoken language, and sends over OSC"};
	std::vector<std::string> senders;
	bool list = false;
	int inputNum = -1;
	std::string inputName = "";
	bool verbose = false;
	parser.add_option("-s,--senders", senders,
		"OSC sender addr:port host pairs, ex. \"192.168.0.100:5555\" "
		"or multicast \"239.200.200.200:6666\", default \"localhost:9999\""
	)->expected(-1);
	parser.add_option("-c,--confidence", app->minConfidence,
		"min confidence, default " + ofToString(app->minConfidence)
	)->transform(CLI::Bound(0.0, 1.0));
	parser.add_option("-t,--threshold", app->volThreshold,
		"volume threshold, default " + ofToString(app->volThreshold)
	)->transform(CLI::Bound(0, 100));
	parser.add_flag("-l,--list", list, "list audio input devices");
	parser.add_option("--inputdev", inputNum, "audio input device number");
	parser.add_option("--inputname", inputName,
		"audio input device name, can do partial match ie. \"Microphone\"");
	parser.add_flag("-v,--verbose", verbose, "verbose printing");
	CLI11_PARSE(parser, argc, argv);

	// verbose printing?
	ofSetLogLevel("LangID", (verbose ? OF_LOG_VERBOSE : OF_LOG_NOTICE));

	// list audio input devices
	if(list) {
		auto devices = app->soundStream.getDeviceList();
		size_t count = 0;
		std::cout << "input devices (# NAME [CHAN]):" << std::endl;
		for(size_t i = 0; i < devices.size(); i++) {
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
		return EXIT_SUCCESS;
	}

	// set audio input from device number
	if(inputNum >= 0) {
		auto devices = app->soundStream.getDeviceList();
		if(inputNum >= devices.size()) {
			ofLogError() << "invalid audio device number: " << inputNum;
			return EXIT_FAILURE;
		}
		ofSoundDevice &device = devices[inputNum];
		if(device.inputChannels == 0) {
			ofLogError("LangID") << "audio device " << inputNum << " has no input channels";
			return EXIT_FAILURE;
		}
		app->inputDevice = inputNum;
	}

	// set audio input from device name
	if(inputName != "") {
		inputNum = -1;
		auto devices = app->soundStream.getDeviceList();
		for(size_t i = 0; i < devices.size(); ++i) {
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
			ofLogWarning("LangID") << "audio input name not found: " << inputName;
		}
	}

	// parse sender host strings
	// split string by last : to get address & port pair,
	// handle bracketed IPv6 hostnames: [::1]:8081
	for(auto host : senders) {
		size_t found = host.find_last_of(":");
		if(found == std::string::npos) {
			ofLogWarning("LangID") << "ignoring sender host without port: " << host;
			continue;
		}
		std::string addr = host.substr(0, found);
		std::string port = host.substr(found+1);
		if(addr.size() == 0 || port.size() == 0) {
			ofLogWarning("LangID") << "ignoring sender host with empty address or port: " << host;
			continue;
		}
		if(addr[0] == '[' && addr[addr.size()-1] == ']') {
			addr = addr.substr(1, addr.size()-2);
		}
		int p = ofToInt(port);
		if(p <= 1024) {
			ofLogWarning("LangID") << "ignoring sender host with invalid port or system port: " << host;
			continue;
		}
		app->hosts.push_back(ofApp::OscHost(addr, p));
	}
	if(app->hosts.empty()) {
		app->hosts.push_back(ofApp::OscHost("localhost", 9999));
	}

	// run app
	ofSetupOpenGL(500, 260, OF_WINDOW);
	ofRunApp(app);

	return EXIT_SUCCESS;
}
