Language Identifier
===================

Identification of chosen languages from 5s long audio snippets and sent over OSC (Open Sound Control) messages.
 
This code base has been developed by [ZKM | Hertz-Lab](https://zkm.de/en/about-the-zkm/organization/hertz-lab) as part of the project [»The Intelligent Museum«](#the-intelligent-museum). 
Please raise issues, ask questions, throw in ideas or submit code, as this repository is intended to be an open platform to collaboratively improve langugae identification.

Copyright (c) 2021 ZKM | Karlsruhe.

BSD Simplified License.

Tested Platforms
----------------

* MacBook Pro 2017, macOS 10.15 & openFrameworks 0.11.2
* MacBook Pro 2018, macOS 11.3.1 & openFrameworks 0.11.2

Structure
---------

* src/: contains the C++ Code that interfaces with OpenFrameworks
* bin/: contains the SavedModels trained with TensorFlow2

Installation
------------

As with all OpenFrameworks examples put this project to a folder where '../../../'
is the root folder of your OpenFrameworks installation.

Run this code
-------------

### Compile

```shell
cd $ROOT_OF_THIS_PROJECT
make
```

### Execute

```
make RunRelease
```

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

The Intelligent Museum
----------------------

An artistic-curatorial field of experimentation for deep learning and visitor participation

The [ZKM | Center for Art and Media](https://zkm.de/en) and the [Deutsches Museum Nuremberg](https://www.deutsches-museum.de/en/nuernberg/information/) cooperate with the goal of implementing an AI-supported exhibition. Together with researchers and international artists, new AI-based works of art will be realized during the next four years (2020-2023).  They will be embedded in the AI-supported exhibition in both houses. The Project „The Intelligent Museum“ is funded by the Digital Culture Programme of the [Kulturstiftung des Bundes](https://www.kulturstiftung-des-bundes.de/en) (German Federal Cultural Foundation).

As part of the project, digital curating will be critically examined using various approaches of digital art. Experimenting with new digital aesthetics and forms of expression enables new museum experiences and thus new ways of museum communication and visitor participation. The museum is transformed to a place of experience and critical exchange.

![Logo](media/Logo_ZKM_DMN_KSB.png)
