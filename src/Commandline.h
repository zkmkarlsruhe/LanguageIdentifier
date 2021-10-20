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

#include "ofApp.h"
#include "CLI11.hpp"

/// commandline option parser
class Commandline {

	public:

		/// constructor with required app instance
		Commandline(ofApp *app);

		/// parse commandline options
		/// returns true if program should continue or false if it should exit
		bool parse(int argc, char **argv);

		/// print parser error and return exit code
		int exit();

		ofApp *app = nullptr;              //< required app instance
		CLI::App parser;                   //< parser instance
		CLI::Error error = CLI::Success(); //< parse error if program should exit
};
