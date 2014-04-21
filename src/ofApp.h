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
    
    void exit();
    
    bool hasMovement();
    bool checkForLoop();
    void getBestLoop(cv::Mat start);
    bool ditchSimilarLoop();
    void populateLoopEnds();
    void initEnds();
    void getMatFromFrameNum(cv::Mat *gray, int frameNum);
    void saveGif(int i);
    void onGifSaved(string &fileName);
    
    ofImage im;
    
    ofVideoPlayer vidPlayer;
    
    ofxCv::FlowPyrLK flows;
    ofxCv::FlowFarneback flow;
    ofMesh mesh;
    int stepSize, xSteps, ySteps;
    
    string fileName;
    string fileExt;
    
    vector< vector<ofImage> > loops;
    vector< vector<ofImage> > displayLoops;
    vector<int> matchIndeces;
    vector<int> loopLengths;
    vector<int> loopPlayIdx;
    vector<ofRectangle> loopDrawRects;
    vector<float> loopQuality;
    
    int loopIdx;
    
    int numFrames;
    int minPeriod;
    float minPeriodSecs;
    int maxPeriod;
    float maxPeriodSecs;
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
    bool minMovementBool;
    bool maxMovementBool;
    
    bool loopFound;
    int loopNum;
    
    int gifNum;
    vector<ofxGifEncoder *> gifses;
    
    bool pausePlayback;
    
    int numLoopsInRow;
    
    //GUI STUFF
    void drawGrid(float x, float y);
    
	void setGuiMatch();
	
	ofxUISuperCanvas *guiMatch;
    
    ofxUITextInput *textInput;
    ofxUISlider *minMove;
    ofxUISlider *maxMove;
    
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
