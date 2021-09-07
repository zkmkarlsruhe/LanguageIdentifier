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

#include <map>
#include <string>

typedef std::map<int, std::string> Labels;

#define USE_MODEL_V2

#ifdef USE_MODEL_V1
	static Labels labelsMap = {
		{0, "noise"},
		{1, "english"},
		{2, "french"},
		{3, "german"},
		{4, "spanish"}
	};
#else
	static Labels labelsMap = {
		{0, "noise"},
		{1, "chinese"},
		{2, "english"},
		{3, "french"},
		{4, "german"},
		{5, "italian"},
		{6, "russian"}
		{7, "spanish"},
	};
#endif