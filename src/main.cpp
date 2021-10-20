#include "ofMain.h"
#include "ofApp.h"

#include "Commandline.h"

//========================================================================
int main(int argc, char **argv) {
	ofApp *app = new ofApp();

	// parse commandline
	Commandline *parser = new Commandline(app);
	if(!parser->parse(argc, argv)) {
		return parser->exit();
	}
	delete parser; // done

	// run app
	ofSetupOpenGL(500, 260, OF_WINDOW);
	ofRunApp(app);

	return EXIT_SUCCESS;
}
