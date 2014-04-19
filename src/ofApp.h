#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxVideoRecorder.h"
#include "ofxGifEncoder.h"
#include "ofxUI.h"

class ofApp : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void keyPressed(int key);
    void keyReleased(int key);
    
    ofxCv::FlowPyrLK flows;
    
    void audioIn(float *input, int bufferSize, int nChannels);
    void exit();
    
    bool hasMovement();
    bool checkForLoop();
    void populateLoopEnds();
    void initEnds();
    void getMatFromFrameNum(cv::Mat *gray, int frameNum);
    void saveGif();
    void onGifSaved(string &fileName);
    
    ofImage im;
    
    ofVideoPlayer vidPlayer;
    
    ofxCv::FlowFarneback flow;
    ofMesh mesh;
    int stepSize, xSteps, ySteps;
    
    ofxVideoRecorder    vidRecorder;
    ofSoundStream       soundStream;
    bool bRecording;
    int sampleRate;
    int channels;
    string fileName;
    string fileExt;
    
    ofFbo recordFbo;
    ofPixels recordPixels;
    
    vector<ofImage> loop;
    int loopIdx;
    
    int numFrames;
    int minPeriod;
    int maxPeriod;
    float fps;
    int frameStart;
    int potentialEndIdx;
    
    cv::Mat firstFrame;
    vector< cv::Mat > potentialLoopEnds;
    vector<cv::Mat> bestMatches;
    
    cv::Size imResize;
    int scale;
    
    float minMovementThresh;
    float maxMovementThresh;
    float loopThresh;
    float minChangeRatio;
    
    bool loopFound;
    int loopNum;
    
    int gifNum;
    vector<ofxGifEncoder *> gifses;
    
    //GUI STUFF
    void drawGrid(float x, float y);
    
	void setGuiMatch();
	void setGuiMovement();
	void setGUI3();
	void setGUI4();
	void setGUI5();
	
	ofxUISuperCanvas *guiMatch;
	ofxUISuperCanvas *guiMovement;
	ofxUISuperCanvas *gui3;
    ofxUISuperCanvas *gui4;
    ofxUISuperCanvas *gui5;
    
    ofxUITextInput *textInput;
    
	bool hideGUI;
	
	float red, green, blue;
	bool bdrawGrid;
	bool bdrawPadding;
	
	void guiEvent(ofxUIEventArgs &e);
    
    ofxUIMovingGraph *mg;
    ofxUIDropDownList *ddl;
    ofxUIToggleMatrix *tm;
    
    float *buffer;
    ofImage *img;
    ofxUIEnvelope *env;

};
