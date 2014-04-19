#include "ofApp.h"

using namespace cv;
using namespace ofxCv;

void ofApp::setup(){
    
    fileName = "danceConv";
    fileExt = ".mp4";
    vidPlayer.loadMovie(fileName + fileExt);
    
    numFrames = vidPlayer.getTotalNumFrames();
    cout << "number of frames: " << numFrames << endl;
    fps = (float)vidPlayer.getTotalNumFrames()/vidPlayer.getDuration();
    minPeriod = (int)fps*1;
    maxPeriod = (int)fps*3;
    cout<< "min period: " << minPeriod << endl;
    cout << "max period: " << maxPeriod << endl;
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    scale = 10;
    imResize = cv::Size(vidPlayer.getWidth()/scale,vidPlayer.getHeight()/scale);
    
    minMovementThresh = 1;//MAXFLOAT;
    maxMovementThresh = 20;
    
    loopThresh = 0.07;
    minChangeRatio = MAXFLOAT;
    
    loopFound = false;
    loopIdx = 0;
    
    frameStart = 0;
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
    initEnds();
    
    ofSetWindowShape(vidPlayer.getWidth()*2, vidPlayer.getHeight()*2);
    
    gifNum = 0;
    ofAddListener(ofxGifEncoder::OFX_GIF_SAVE_FINISHED, this, &ofApp::onGifSaved);
    //ofSetFrameRate(25); //CHANGE THIS
    
    
    //GUI STUFF
    hideGUI = false;
    bdrawGrid = false;
	bdrawPadding = false;
    
    ddl = NULL;
    textInput = NULL;
    //img = new ofImage();
    //img->loadImage("nerd_me.png");
    //buffer = new float[256];
    //for(int i = 0; i < 256; i++) { buffer[i] = ofNoise(i/100.0); }
    
	setGuiMatch();
	setGuiMovement();
    setGUI3();
    setGUI4();
    setGUI5();
    
    guiMatch->loadSettings("guiMatchSettings.xml");
    guiMovement->loadSettings("guiMovmentSettings.xml");
    gui3->loadSettings("gui3Settings.xml");
    gui4->loadSettings("gui4Settings.xml");
    gui5->loadSettings("gui5Settings.xml");

}


void ofApp::initEnds(){
    if (potentialLoopEnds.size() > 0){
        potentialLoopEnds.clear();
    }
    vidPlayer.setFrame(frameStart + minPeriod);
    vidPlayer.update();
    while(vidPlayer.getCurrentFrame() <= vidPlayer.getTotalNumFrames() && vidPlayer.getCurrentFrame() < frameStart + maxPeriod) {
        cv::Mat currGray;
        cv::cvtColor(toCv(vidPlayer), currGray, CV_RGB2GRAY);
        cv::Mat currFrame;
        cv::resize(currGray, currFrame, imResize);
        
        potentialLoopEnds.push_back(currFrame);
        vidPlayer.nextFrame();
        vidPlayer.update();
    }
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
}

void ofApp::populateLoopEnds(){
    if (potentialLoopEnds.size() > 0){
        potentialLoopEnds.erase(potentialLoopEnds.begin());
        int endIdx = frameStart + maxPeriod;
        //cout << "endIdx: " << endIdx << endl;
        if (endIdx < vidPlayer.getTotalNumFrames()){
            vidPlayer.setFrame(endIdx);
            vidPlayer.update();
            
            cv::Mat endGray;
            cv::cvtColor(toCv(vidPlayer), endGray, CV_RGB2GRAY);
            cv::Mat endFrame;
            cv::resize(endGray, endFrame, imResize);
            
            potentialLoopEnds.push_back(endFrame);
        }
    }
    else{
        initEnds();
    }
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
}


bool ofApp::checkForLoop(){
    cv::Mat startGray;
    cv::cvtColor(firstFrame, startGray, CV_RGB2GRAY);
    cv::Mat start;
    cv::resize(startGray, start, imResize);
    
    float startSum = cv::sum(start)[0] + 1; //add one to get rid of multiplication by 0 if screen is black
    for (int i = 0; i < potentialLoopEnds.size(); i++) {
        cv::Mat currEnd = potentialLoopEnds.at(i);
        cv::Mat diff;
        cv::absdiff(start, currEnd, diff);
        float endDiff = cv::sum(diff)[0] + 1;
        float changeRatio = endDiff/startSum;
        //cout << "changeRatio: " << changeRatio << endl;
        
        if (changeRatio < minChangeRatio) {
            minChangeRatio = changeRatio;
            cout << "min Ratio: " << minChangeRatio << endl;
        }
        
        if (changeRatio < loopThresh) {
            potentialEndIdx = frameStart + minPeriod + i;
            if (hasMovement()){
                //cout << "MOVED" << endl;
                if (loopFound)
                    loop.clear();
                for (int j = frameStart; j <= frameStart + minPeriod + i; j++) {
                    vidPlayer.setFrame(j);
                    ofImage loopee;
                    loopee.setFromPixels(vidPlayer.getPixels(), vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
                    loop.push_back(loopee);
                }
                loopFound = true;
                vidPlayer.setFrame(frameStart);
                vidPlayer.update();
                
                return true;
            }
            else{
                frameStart = potentialEndIdx - 1;
                vidPlayer.setFrame(frameStart);
                vidPlayer.update();
                initEnds();
                return false;
            }
        }
    }
    
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
    return false;
}

bool ofApp::hasMovement(){
    return true;
    float sumDiff = 0;
    /*cv::Mat startGray;
    cv::cvtColor(firstFrame, startGray, CV_RGB2GRAY);
    cv::Mat prevFrame;
    cv::resize(startGray, prevFrame, imResize);*/
    
    cv::Mat *prevFrame = new cv::Mat();
    getMatFromFrameNum(prevFrame, frameStart);
    //cv::Mat prevFrame = *startGray;
    
    float prevSum = cv::sum(*prevFrame)[0] + 1; //add one to get rid of multiplication by 0 if screen is black
    
    /*vidPlayer.setFrame(frameStart);
    cout << "checking " << potentialEndIdx - frameStart << " frames for movement" << endl;
    while(vidPlayer.getCurrentFrame() <= vidPlayer.getTotalNumFrames() && vidPlayer.getCurrentFrame() < frameStart + potentialEndIdx){
        vidPlayer.nextFrame();
        cout << "frame num: " << vidPlayer.getCurrentFrame() << endl;
        cv::Mat currGray;
        cv::cvtColor(toCv(vidPlayer), currGray, CV_RGB2GRAY);
        cv::Mat currFrame;
        cv::resize(currGray, currFrame, imResize);
        
        cv::Mat diff;
        cv::absdiff(currFrame,prevFrame,diff);
        
        float endDiff = cv::sum(diff)[0];
        
        float changeRatio = endDiff/prevSum;
        cout << "movement Change Ratio: " << changeRatio << endl;
        sumDiff += changeRatio;
        
        //currFrame.copyTo(prevFrame);
        prevFrame = currFrame;
        prevSum = cv::sum(prevFrame)[0] + 1;
    }*/
    
    cv::Mat *currFrame = new cv::Mat();
    for (int i = frameStart+1; i <= frameStart + potentialEndIdx && i <= vidPlayer.getTotalNumFrames(); i++) {
        //cv::Mat *currGray = new cv::Mat();
        getMatFromFrameNum(currFrame, i);
        //cv::Mat currFrame = *currGray;
        
        cv::Mat diff;
        cv::absdiff(*currFrame,*prevFrame,diff);
        
        
        float endDiff = cv::sum(diff)[0] + 1;
        
        float changeRatio = endDiff/prevSum;
        //cout << "movement Change Ratio: " << changeRatio << endl;
        sumDiff += changeRatio;
        
        //currFrame.copyTo(prevFrame);
        prevFrame = currFrame;
        prevSum = cv::sum(*prevFrame)[0] + 1;
        
    }
    free(prevFrame);
    //free(currFrame);
    //cout << "sum Diff: " << sumDiff << endl;
    //cout << "threshold: " << movementThresh*(potentialEndIdx - frameStart) << endl;
    if(sumDiff > minMovementThresh*(potentialEndIdx - frameStart) ){//&& sumDiff < maxMovementThresh*(potentialEndIdx - frameStart)){
        //cout << "frameNum: " << frameStart << endl;
        return true;
    }
    
    return false;
}


void ofApp::getMatFromFrameNum(cv::Mat *gray, int frameNum){
    int prevFrameNum = vidPlayer.getCurrentFrame();
    
    vidPlayer.setFrame(frameNum);
    cv::Mat currGray;
    cv::cvtColor(toCv(vidPlayer), currGray, CV_RGB2GRAY);
    cv::Mat currFrame;
    cv::resize(currGray, currFrame, imResize);

    currFrame.copyTo(*gray);
    
    vidPlayer.setFrame(prevFrameNum);
}

void ofApp::update(){
    if (frameStart >= vidPlayer.getTotalNumFrames())
        frameStart = 0;
    
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
    firstFrame = toCv(vidPlayer);
    
    if(checkForLoop()){
        cout << "Loop Found" << endl;
    }
    
    //flow.calcOpticalFlow(vidPlayer.getPixelsRef());
    loopIdx ++;
    populateLoopEnds();
    frameStart ++;
}


void ofApp::draw(){
    
    vidPlayer.setFrame(frameStart - 1);
    vidPlayer.update();
    vidPlayer.draw(vidPlayer.getWidth(), vidPlayer.getHeight());
    
    if(loopIdx >= loop.size())
        loopIdx = 0;
    if (loop.size() > 0) {
        loop.at(loopIdx).draw(0,0);
    }
    
    
    //GUI STUFF
    ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
    
	if(bdrawGrid)
	{
		ofSetColor(255, 255, 255, 25);
		drawGrid(8,8);
	}
    
	ofPopStyle();
    
    ofSetRectMode(OF_RECTMODE_CENTER);
    
    /*
    if (ofGetFrameNum()%60 == 0) {
        flow.resetFeaturesToTrack();
    }
    else{
        ofBackground(0);
        if(vidPlayer.getCurrentFrame() <= vidPlayer.getTotalNumFrames()) {
            vidPlayer.nextFrame();
            flow.calcOpticalFlow(vidPlayer.getPixelsRef());
            ofPixelsRef pix = vidPlayer.getPixelsRef();
            //im.setFromPixels(pix.getPixels(), pix.getWidth(), pix.getHeight(), OF_IMAGE_COLOR);
            //im.draw(0, 0);
            
            //skinMat = skinDetect.getSkin(toCv(pix));
            
            ofImage im;
            im.allocate(ofGetWidth(), ofGetHeight(), OF_IMAGE_GRAYSCALE);
            //toOf(skinMat, im);
            im.update();
            ofColor backG = getContourAvgColor(0, pix, im.getPixelsRef());
            ofBackground(backG.r, backG.g, backG.b);
            
            //im.update();
            //im.draw(0, 0);
            
            flow.draw();
        }
    }
    
    ofImage grab;
    grab.allocate(ofGetWidth(),ofGetHeight(),OF_IMAGE_COLOR);
    grab.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
    
    vidRecorder.addFrame(grab.getPixelsRef());
*/
    
}

void ofApp::mouseDragged(int x, int y, int button){

}

void ofApp::mousePressed(int x, int y, int button){

}

void ofApp::mouseReleased(int x, int y, int button){
    saveGif();
}

void ofApp::saveGif(){
    ofxGifEncoder *giffy = new ofxGifEncoder();
    (*giffy).setup(vidPlayer.getWidth(), vidPlayer.getHeight(), 1/fps, 256);
    ofImage save;
    for (int i = 0; i < loop.size(); i++) {
        save.setFromPixels(loop.at(i).getPixels(), vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
        (*giffy).addFrame(save);
    }
    
    string gifFileName = fileName + "/" + fileName + "_" + ofGetTimestampString() + ".gif";
    ofDirectory dir(gifFileName);
    if(!dir.exists()){
        dir.create(true);
    }
    cout << "gifFileName: " << gifFileName << endl;
    (*giffy).save(gifFileName);
    gifNum++;
    gifses.push_back(giffy);
}



void ofApp::onGifSaved(string &fileName) {
    cout << "gif saved as " << fileName << endl;
}

void ofApp::audioIn(float *input, int bufferSize, int nChannels){
    if(bRecording)
        vidRecorder.addAudioSamples(input, bufferSize, nChannels);
}

void ofApp::exit() {
    vidRecorder.close();
    for (int i = 0; i < gifses.size(); i++) {
        (*gifses.at(i)).exit();
    }
    
    guiMatch->saveSettings("guiMatchSettings.xml");
    guiMovement->saveSettings("guiMovementSettings.xml");
    gui3->saveSettings("gui3Settings.xml");
    gui4->saveSettings("gui4Settings.xml");
    gui5->saveSettings("gui5Settings.xml");
    
	delete guiMatch;
	delete guiMovement;
    delete gui3;
    delete gui4;
    delete gui5;
}

void ofApp::keyPressed(int key){
    if(guiMovement->hasKeyboardFocus())
    {
        return;
    }
	switch (key)
	{
		case 't':
        {
            if(textInput != NULL)
            {
                textInput->setTextString(ofGetTimestampString());
            }
        }
			break;
            
		case 'T':
        {
            if(tm != NULL)
            {
                int cols = tm->getColumnCount();
                int rows = tm->getRowCount();
                for(int row = 0; row < rows; row++)
                {
                    for(int col = 0; col < cols; col++)
                    {
                        cout << tm->getState(row, col) << "\t";
                    }
                    cout << endl;
                }
            }
        }
			break;
            
		case 'd':
        {
            if(ddl != NULL)
            {
                vector<ofxUIWidget *> selected = ddl->getSelected();
                for(vector<ofxUIWidget *>::iterator it = selected.begin(); it != selected.end(); ++it)
                {
                    ofxUILabelToggle *lt = (ofxUILabelToggle *) (*it);
                    cout << lt->getName() << endl;
                }
            }
        }
			break;
            
        case 'D':
        {
            if(ddl != NULL)
            {
                vector<string> names = ddl->getSelectedNames();
                for(vector<string>::iterator it = names.begin(); it != names.end(); ++it)
                {
                    cout << (*it) << endl;
                }
            }
        }
			break;
            
		case 'r':
        {
            if(textInput != NULL)
            {
                textInput->setFocus(!textInput->isFocused());
            }
        }
			break;
            
		case 'f':
			ofToggleFullscreen();
			break;
            
        case 'F':
        {
            if(tm != NULL)
            {
                tm->setDrawOutlineHighLight(!tm->getDrawOutlineHighLight());
                //                tm->setDrawPaddingOutline(!tm->getDrawPaddingOutline());
            }
        }
			break;
            
		case 'h':
            guiMatch->toggleVisible();
            guiMovement->toggleVisible();
            gui3->toggleVisible();
            gui4->toggleVisible();
            gui5->toggleVisible();
			break;
            
		case 'p':
			bdrawPadding = !bdrawPadding;
			guiMatch->setDrawWidgetPaddingOutline(bdrawPadding);
			guiMovement->setDrawWidgetPaddingOutline(bdrawPadding);
			gui3->setDrawWidgetPaddingOutline(bdrawPadding);
			gui4->setDrawWidgetPaddingOutline(bdrawPadding);
			gui5->setDrawWidgetPaddingOutline(bdrawPadding);
			break;
            
		case '[':
			guiMatch->setDrawWidgetPadding(false);
			guiMovement->setDrawWidgetPadding(false);
			gui3->setDrawWidgetPadding(false);
			gui4->setDrawWidgetPadding(false);
			gui5->setDrawWidgetPadding(false);
			break;
            
		case ']':
			guiMatch->setDrawWidgetPadding(true);
			guiMovement->setDrawWidgetPadding(true);
			gui3->setDrawWidgetPadding(true);
			gui4->setDrawWidgetPadding(true);
			gui5->setDrawWidgetPadding(true);
			break;
			
        case '1':
            guiMatch->toggleVisible();
            break;
            
        case '2':
            guiMovement->toggleVisible();
            break;
            
        case '3':
            gui3->toggleVisible();
            break;
            
        case '4':
            gui4->toggleVisible();
            break;
            
        case '5':
            gui5->toggleVisible();
            break;
            
		default:
			break;
	}


}

void ofApp::keyReleased(int key){
}


//GUI STUFF
void ofApp::guiEvent(ofxUIEventArgs &e)
{
	string name = e.getName();
	int kind = e.getKind();
	cout << "got event from: " << name << endl;
    if(kind == OFX_UI_WIDGET_NUMBERDIALER)
    {
        ofxUINumberDialer *n = (ofxUINumberDialer *) e.widget;
        cout << n->getValue() << endl;
    }
	
    if(name == "SAMPLER")
    {
        ofxUIImageSampler *is = (ofxUIImageSampler *) e.widget;
        ofColor clr = is->getColor();
        red = clr.r;
        blue = clr.b;
        green = clr.g;
    }
	else if(name == "BUTTON")
	{
		ofxUIButton *button = (ofxUIButton *) e.getButton();
		bdrawGrid = button->getValue();
	}
	else if(name == "TOGGLE")
	{
		ofxUIToggle *toggle = (ofxUIToggle *) e.getToggle();
		bdrawGrid = toggle->getValue();
        if(textInput != NULL)
        {
            textInput->setFocus(bdrawGrid);
        }
	}
    else if(name == "RADIO VERTICAL")
    {
        ofxUIRadio *radio = (ofxUIRadio *) e.widget;
        cout << radio->getName() << " value: " << radio->getValue() << " active name: " << radio->getActiveName() << endl;
    }
    else if(name == "TEXT INPUT")
    {
        ofxUITextInput *ti = (ofxUITextInput *) e.widget;
        if(ti->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_ENTER)
        {
            cout << "ON ENTER: ";
        }
        else if(ti->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_FOCUS)
        {
            cout << "ON FOCUS: ";
        }
        else if(ti->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_UNFOCUS)
        {
            cout << "ON BLUR: ";
        }
        string output = ti->getTextString();
        cout << output << endl;
    }
}


void ofApp::drawGrid(float x, float y)
{
    float w = ofGetWidth();
    float h = ofGetHeight();
    
    for(int i = 0; i < h; i+=y)
    {
        ofLine(0,i,w,i);
    }
    
    for(int j = 0; j < w; j+=x)
    {
        ofLine(j,0,j,h);
    }
}

void ofApp::setGuiMatch()
{
    /*vector<string> names;
	names.push_back("RAD1");
	names.push_back("RAD2");
	names.push_back("RAD3");
	
	guiMatch = new ofxUISuperCanvas("PANEL 1: BASICS");
    guiMatch->addSpacer();
    guiMatch->addLabel("Press 'h' to Hide GUIs", OFX_UI_FONT_SMALL);
    
    guiMatch->addSpacer();
	guiMatch->addLabel("H SLIDERS");
	guiMatch->addSlider("RED", 0.0, 255.0, &red)->setTriggerType(OFX_UI_TRIGGER_ALL);
	guiMatch->addSlider("GREEN", 0.0, 255.0, &green)->setTriggerType(OFX_UI_TRIGGER_BEGIN|OFX_UI_TRIGGER_CHANGE|OFX_UI_TRIGGER_END);
	guiMatch->addSlider("BLUE", 0.0, 255.0, &blue)->setTriggerType(OFX_UI_TRIGGER_BEGIN|OFX_UI_TRIGGER_CHANGE);
    
    guiMatch->addSpacer();
    guiMatch->addLabel("V SLIDERS");
	guiMatch->addSlider("0", 0.0, 255.0, 150, 17, 160);
	guiMatch->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
	guiMatch->addSlider("1", 0.0, 255.0, 150, 17, 160);
	guiMatch->addSlider("2", 0.0, 255.0, 150, 17, 160);
	guiMatch->addSlider("3", 0.0, 255.0, 150, 17, 160);
	guiMatch->addSlider("4", 0.0, 255.0, 150, 17, 160);
	guiMatch->addSlider("5", 0.0, 255.0, 150, 17, 160);
	guiMatch->addSlider("6", 0.0, 255.0, 150, 17, 160);
	guiMatch->addSlider("7", 0.0, 255.0, 150, 17, 160);
	guiMatch->addSlider("8", 0.0, 255.0, 150, 17, 160);
	guiMatch->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    
    guiMatch->addSpacer();
	guiMatch->addRadio("RADIO HORIZONTAL", names, OFX_UI_ORIENTATION_HORIZONTAL);
	guiMatch->addRadio("RADIO VERTICAL", names, OFX_UI_ORIENTATION_VERTICAL);
    
    guiMatch->addSpacer();
    guiMatch->setWidgetFontSize(OFX_UI_FONT_SMALL);
	guiMatch->addButton("BUTTON", false);
	guiMatch->addToggle( "TOGGLE", false);
    
    guiMatch->addSpacer();
    guiMatch->addLabel("RANGE SLIDER");
	guiMatch->addRangeSlider("RSLIDER", 0.0, 255.0, 50.0, 100.0);
    
    string textString = "This widget is a text area widget. Use this when you need to display a paragraph of text. It takes care of formatting the text to fit the block.";
    guiMatch->addSpacer();
    guiMatch->addTextArea("textarea", textString, OFX_UI_FONT_SMALL);
    
    guiMatch->autoSizeToFitWidgets();
	ofAddListener(guiMatch->newGUIEvent,this,&ofApp::guiEvent);*/
}

void ofApp::setGuiMovement()
{
    /*guiMovement = new ofxUISuperCanvas("PANEL 2: ADVANCED");
    
    guiMovement->addSpacer();
	guiMovement->setWidgetFontSize(OFX_UI_FONT_MEDIUM);
    textInput = guiMovement->addTextInput("TEXT INPUT", "Input Text");
    textInput->setAutoUnfocus(false);
    guiMovement->addLabel("AUTO CLEAR DISABLED", OFX_UI_FONT_SMALL);
    guiMovement->addTextInput("TEXT INPUT2", "Input Text")->setAutoClear(false);
	guiMovement->setWidgetFontSize(OFX_UI_FONT_MEDIUM);
    
    guiMovement->addSpacer();
    guiMovement->addLabel("WAVEFORM DISPLAY");
	guiMovement->addWaveform("WAVEFORM", buffer, 256, 0.0, 1.0);
    guiMovement->addLabel("SPECTRUM DISPLAY");
    guiMovement->addSpectrum("SPECTRUM", buffer, 256, 0.0, 1.0);
    
    vector<float> buffer;
    for(int i = 0; i < 256; i++)
    {
        buffer.push_back(0.0);
    }
    
    guiMovement->addLabel("MOVING GRAPH", OFX_UI_FONT_MEDIUM);
    mg = guiMovement->addMovingGraph("MOVING", buffer, 256, 0.0, 1.0);
    
    guiMovement->addSpacer();
    guiMovement->addLabel("IMAGE DISPLAY");
	guiMovement->addImage("IMAGE CAPTION", img);
    
    guiMovement->addSpacer();
    guiMovement->addLabel("FPS LABEL");
    guiMovement->addFPS();
    
    guiMovement->setWidgetFontSize(OFX_UI_FONT_SMALL);
    guiMovement->addSpacer();
    guiMovement->addLabel("NUMBER DIALER");
    guiMovement->addNumberDialer("DIALER", -10000, 10000, 5000, 3);
    
    guiMovement->addSpacer();
    guiMovement->addLabel("LABEL BUTTON", OFX_UI_FONT_MEDIUM);
    guiMovement->addLabelButton("LABEL BTN", false);
    
    guiMovement->addSpacer();
    guiMovement->addLabel("LABEL TOGGLES", OFX_UI_FONT_MEDIUM);
    guiMovement->addLabelToggle("LABEL TGL", false)->getLabelWidget()->setColorFill(ofColor(255, 0, 0));
    
    guiMovement->setPosition(212, 0);
    guiMovement->autoSizeToFitWidgets();
    
	ofAddListener(guiMovement->newGUIEvent,this,&ofApp::guiEvent);*/
}

void ofApp::setGUI3()
{
	/*gui3 = new ofxUISuperCanvas("PANEL 3: ADVANCED");
    
    gui3->addSpacer();
    gui3->setGlobalButtonDimension(24);
    gui3->addLabel("MATRIX", OFX_UI_FONT_MEDIUM);
    gui3->addToggleMatrix("MATRIX1", 3, 3);
    tm = gui3->addToggleMatrix("MATRIX2", 3, 6);
    gui3->addToggleMatrix("MATRIX3", 1, 4);
    
    gui3->addSpacer();
    gui3->setGlobalButtonDimension(64);
    gui3->addImageButton("IMAGEBTN", "GUI/images/App.png", false);
    gui3->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    gui3->addImageToggle("IMAGETGL", "GUI/images/Preview.png", false);
    gui3->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    
    gui3->addSpacer();
    env = new ofxUIEnvelope();
    for(float i = 0; i <= 5; i++)
    {
        env->addPoint(i/5.0, i/5.0);
    }
    
    gui3->addWidgetDown(new ofxUIEnvelopeEditor("ENV", env, 200, 128));
    
    vector<string> items;
    items.push_back("FIRST ITEM"); items.push_back("SECOND ITEM"); items.push_back("THIRD ITEM");
    items.push_back("FOURTH ITEM"); items.push_back("FIFTH ITEM"); items.push_back("SIXTH ITEM");
    
    gui3->addSpacer();
    gui3->setWidgetFontSize(OFX_UI_FONT_SMALL);
    gui3->addSortableList("SORTABLE LIST", items);
    
    gui3->addSpacer();
    gui3->setWidgetFontSize(OFX_UI_FONT_MEDIUM);
    ddl = gui3->addDropDownList("DROP DOWN LIST", items);
    ddl->setAllowMultiple(true);
    
    gui3->setGlobalButtonDimension(OFX_UI_GLOBAL_BUTTON_DIMENSION);
    
    gui3->setPosition(212*2, 0);
    gui3->autoSizeToFitWidgets();
    
	ofAddListener(gui3->newGUIEvent,this,&ofApp::guiEvent);*/
}

void ofApp::setGUI4()
{
	/*gui4 = new ofxUISuperCanvas("PANEL 4: ADVANCED");
    
    gui4->addSpacer();
    gui4->addLabel("BILABEL SLIDER");
    gui4->addBiLabelSlider("BILABEL", "HOT", "COLD", 0, 100, 50);
    
    gui4->addLabel("MINIMAL SLIDER");
    gui4->addMinimalSlider("MINIMAL", 0, 100, 50.0)->getLabelWidget()->setColorFill(ofColor(255, 255, 0));
    
    gui4->addSpacer();
    gui4->addLabel("FPS SLIDER", OFX_UI_FONT_MEDIUM);
    gui4->addFPSSlider("FPS SLIDER");
    
    gui4->addSpacer();
    gui4->addLabel("IMAGE SAMPLER", OFX_UI_FONT_MEDIUM);
    gui4->addImageSampler("SAMPLER", img);
    gui4->setGlobalButtonDimension(64);
    gui4->addMultiImageButton("IMAGE BUTTON", "GUI/toggle.png", false);
    gui4->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    gui4->addMultiImageToggle("IMAGE TOGGLE", "GUI/toggle.png", false);
    gui4->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    
    gui4->addBaseDraws("BASE DRAW", img, true);
    
    gui4->addSpacer();
    gui4->setGlobalButtonDimension(32);
    gui4->addButton("BTN", false)->setLabelVisible(false);
    gui4->addToggle("TGL", false)->setLabelVisible(false);
    
    gui4->setPosition(212*3,0);
    gui4->autoSizeToFitWidgets();
    
	ofAddListener(gui4->newGUIEvent,this,&ofApp::guiEvent);*/
}

void ofApp::setGUI5()
{
	/*gui5 = new ofxUISuperCanvas("PANEL 5: ADVANCED");
    gui5->addSpacer();
    
	gui5->addLabel("2D PAD");
	gui5->add2DPad("PAD", ofPoint(-100, 100), ofPoint(-100,100), ofPoint(0,0));
    
    gui5->addSpacer();
    gui5->addLabel("ROTARY SLIDER", OFX_UI_FONT_MEDIUM);
    gui5->addRotarySlider("R2SLIDER", 0, 100, 50);
    
    gui5->addSpacer();
    gui5->addLabel("CIRCLE SLIDER", OFX_UI_FONT_MEDIUM);
    gui5->addCircleSlider("NORTH SOUTH", 0, 100, 50.0);
    
    gui5->setPosition(212*4,0);
    gui5->autoSizeToFitWidgets();
    
	ofAddListener(gui5->newGUIEvent,this,&ofApp::guiEvent);*/
}


