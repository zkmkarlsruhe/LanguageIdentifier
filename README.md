Language Identifier
===================

![screenshot](media/screenshot.png)

Identification of chosen languages from 5s long audio snippets
 
This code base has been developed by [ZKM | Hertz-Lab](https://zkm.de/en/about-the-zkm/organization/hertz-lab) as part of the project [»The Intelligent Museum«](#the-intelligent-museum). 

Please raise issues, ask questions, throw in ideas or submit code, as this repository is intended to be an open platform to collaboratively improve langugae identification.

Copyright (c) 2021 ZKM | Karlsruhe.  
Copyright (c) 2021 Paul Bethge.  
Copyright (c) 2021 Dan Wilcox.  

BSD Simplified License.

Dependencies
------------

* [openFrameworks](https://openframeworks.cc/download/)
* openFrameworks addons:
  - ofxOSC (included with oF)
  - [ofxTensorFlow2](https://github.com/zkmkarlsruhe/ofxTensorFlow2)
* Pre-trained language id model(s) placed in `bin/data` (included in this repo)

Structure
---------

* src/: contains the openFrameworks C++ code
* bin/data/model_*: contains the SavedModels trained with TensorFlow2

Installation & Build
--------------------

Overview:

1. Follow the steps in the ofxTensorFlow2 "Installation & Build" section for you platform
2. Generate the project files for this folder using the OF ProjectGenerator
3. Build for your platform

### Generating Project Files

Project files are not included so you will need to generate the project files for your operating system and development environment using the OF ProjectGenerator which is included with the openFrameworks distribution.

To (re)generate project files for an existing project:

1. Click the "Import" button in the ProjectGenerator
2. Navigate to the project's parent folder ie. "apps/myApps", select the base folder for the example project ie. "LanguageIdentifier", and click the Open button
3. Click the "Update" button

If everything went Ok, you should now be able to open the generated project and build/run the example.

### macOS

Open the Xcode project, select the "LanguageIdentifier Debug" scheme, and hit "Run".

For a Makefile build, build and run an example on the terminal:

```shell
cd LanguageIdentifier
make ReleaseTF2
make RunRelease
```
### Linux

For a Makefile build, build and run an example on the terminal:

```shell
cd LanguageIdentifier
make Release
make RunReleaseTF2
```

Usage
-----

The openFrameworks application runs the language identification model using audio input. The detection status and detected language is sent out using OSC (Open Sound Control) messages.

### OSC Communication

Sends to:
* address: `localhost` ie. `127.0.0.1`
* port: `9999`

Message specification:

* **/detected status**: detection status
  - status: float, boolean 1 found - 0 lost
* **/lang index name confidence**: detected language
  - index: int, language map index
  - name: string, language map name
  - confidence: float, confidence percentage 0 - 100

Tested Platforms
----------------

* MacBook Pro 2017, macOS 10.15 & openFrameworks 0.11.2
* MacBook Pro 2018, macOS 11.3.1 & openFrameworks 0.11.2

The Intelligent Museum
----------------------

An artistic-curatorial field of experimentation for deep learning and visitor participation

The [ZKM | Center for Art and Media](https://zkm.de/en) and the [Deutsches Museum Nuremberg](https://www.deutsches-museum.de/en/nuernberg/information/) cooperate with the goal of implementing an AI-supported exhibition. Together with researchers and international artists, new AI-based works of art will be realized during the next four years (2020-2023).  They will be embedded in the AI-supported exhibition in both houses. The Project „The Intelligent Museum“ is funded by the Digital Culture Programme of the [Kulturstiftung des Bundes](https://www.kulturstiftung-des-bundes.de/en) (German Federal Cultural Foundation).

As part of the project, digital curating will be critically examined using various approaches of digital art. Experimenting with new digital aesthetics and forms of expression enables new museum experiences and thus new ways of museum communication and visitor participation. The museum is transformed to a place of experience and critical exchange.

![Logo](media/Logo_ZKM_DMN_KSB.png)
