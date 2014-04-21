#include "ofApp.h"

using namespace cv;
using namespace ofxCv;

void ofApp::setup(){
    
    //Video stuff
    fileName = "walken";
    fileExt = ".mp4";
    vidPlayer.loadMovie(fileName + fileExt);
    //vidPlayer.setUseTexture(true);
    
    numFrames = vidPlayer.getTotalNumFrames();
    cout << "number of frames: " << numFrames << endl;
    fps = (float)vidPlayer.getTotalNumFrames()/vidPlayer.getDuration();
    minPeriodSecs = 1.0;
    maxPeriodSecs = 3.0;
    minPeriod = (int)(fps*minPeriodSecs);
    maxPeriod = (int)(fps*maxPeriodSecs);
    cout<< "min period: " << minPeriod << endl;
    cout << "max period: " << maxPeriod << endl;
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    scale = 10;
    imResize = cv::Size(vidPlayer.getWidth()/scale,vidPlayer.getHeight()/scale);
    
    numLoopsInRow = 5;
    
    minMovementThresh = 1;//MAXFLOAT;
    minMovementBool = true;
    maxMovementThresh = 20;
    maxMovementBool = false;
    
    loopThresh = 0.07;
    minChangeRatio = MAXFLOAT;
    
    //loopFound = false;
    loopIdx = 0;
    
    frameStart = 0;
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
    initEnds();
    
    gifNum = 0;
    ofAddListener(ofxGifEncoder::OFX_GIF_SAVE_FINISHED, this, &ofApp::onGifSaved);
    //ofSetFrameRate(fps); //CHANGE THIS
    
    //GUI STUFF
    hideGUI = false;
    bdrawGrid = false;
	bdrawPadding = false;
    
    ddl = NULL;
    textInput = NULL;
    
	setGuiMatch();

    guiMatch->loadSettings("guiMatchSettings.xml");
    ofSetWindowShape(guiMatch->getGlobalCanvasWidth() + vidPlayer.getWidth(), vidPlayer.getHeight()*2);
    ofSetBackgroundColor(127);
    
}


void ofApp::initEnds(){
    if (potentialLoopEnds.size() > 0){
        potentialLoopEnds.clear();
    }
    //vidPlayer.setFrame(frameStart + minPeriod);
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
    while(vidPlayer.getCurrentFrame() <= vidPlayer.getTotalNumFrames() && vidPlayer.getCurrentFrame() <= frameStart + maxPeriod) {
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
    cv::Mat start = potentialLoopEnds.at(0);
    
    float startSum = cv::sum(start)[0] + 1; //add one to get rid of division by 0 if screen is black
    cv::Mat currEnd;
    //cout << "minPeriod: " << minPeriod << endl;
    //cout << "maxPeriod: " << maxPeriod << endl;
    for (int i = minPeriod; i < potentialLoopEnds.size(); i++) {
        //cout << "i: " << i << endl;
        potentialLoopEnds.at(i).copyTo(currEnd);
        cv::Mat diff;
        cv::absdiff(start, currEnd, diff);
        float endDiff = cv::sum(diff)[0] + 1;
        float changeRatio = endDiff/startSum;
        
        if (changeRatio < minChangeRatio) {
            minChangeRatio = changeRatio;
            //cout << "min Ratio: " << minChangeRatio << endl;
        }
        
        if (changeRatio < 1 - loopThresh/100){//set to change percentage
            //cout << "change Ratio: " << changeRatio << endl;
            //cout << "test Thresh: " << 1 - loopThresh/100 << endl;
            potentialEndIdx = frameStart + i;
            //cout << "potential End Idx: " << potentialEndIdx << endl;
            if (minMovementBool || maxMovementBool){
                if  (hasMovement()){
                    matchIndeces.push_back(potentialEndIdx);
                    bestMatches.push_back(currEnd);
                }
                else{
                    frameStart = potentialEndIdx;
                    vidPlayer.setFrame(frameStart);
                    vidPlayer.update();
                    potentialLoopEnds.clear();
                    return false;
                }
            }
            else{
                matchIndeces.push_back(potentialEndIdx);
                bestMatches.push_back(currEnd);
            }
        }
    }
    
    if (bestMatches.size() > 0) {
        vidPlayer.setFrame(frameStart);
        vidPlayer.update();
        getBestLoop(start);
        vidPlayer.setFrame(frameStart);
        vidPlayer.update();
        return true;
    }
        
    vidPlayer.setFrame(frameStart);
    vidPlayer.update();
    return false;
}


bool ofApp::hasMovement(){
    //return true;
    float sumDiff = 0;
    
    cv::Mat prevFrame = potentialLoopEnds.at(0);
    float prevSum = cv::sum(prevFrame)[0] + 1; //add one to get rid of division by 0 if screen is black

    cv::Mat currFrame;
    for (int i = 1; i < potentialEndIdx - frameStart; i++) {
        cv::Mat currFrame = potentialLoopEnds.at(i);
        cv::Mat diff;
        cv::absdiff(currFrame,prevFrame,diff);
        
        float endDiff = cv::sum(diff)[0] + 1;
        
        float changeRatio = endDiff;///prevSum;
        //cout << "movement Change Ratio: " << changeRatio << endl;
        sumDiff += changeRatio;
        
        prevFrame = currFrame;
        //prevSum = cv::sum(prevFrame)[0] + 1;
    }
    sumDiff /= prevSum;
    
    cout << "sum Diff: " << sumDiff << endl;
    cout << "minThreshold: " << minMovementThresh*(potentialEndIdx - frameStart)/100 << endl;
    //if(maxMovementBool)
        //cout << "maxThreshold: " << maxMovementThresh*(potentialEndIdx - frameStart)/100 << endl;
    
    if (minMovementBool) {
        //if(sumDiff < minMovementThresh*(potentialEndIdx - frameStart)/100 ){
        if(sumDiff < minMovementThresh/100 ){
            cout << "not enough movement" << endl;
            return false;
        }
    }
    else if (maxMovementBool){
        //if(sumDiff > maxMovementThresh*(potentialEndIdx - frameStart)/100 ){
        if(sumDiff > maxMovementThresh/100 ){
            cout << "too much movement" << endl;
            return false;
        }
    }
    else if (minMovementBool && maxMovementBool){
        //if(sumDiff > maxMovementThresh*(potentialEndIdx - frameStart)/100 && sumDiff < minMovementThresh*(potentialEndIdx - frameStart)/100){
        if(sumDiff > maxMovementThresh/100 && sumDiff < minMovementThresh/100){
            cout << "not enough movement or too much" << endl;
            return false;
        }
    }
    
    return true;
}


void ofApp::getBestLoop(cv::Mat start){
    float minChange = MAXFLOAT;
    int bestEnd = -1;
    float startSum = cv::sum(start)[0] + 1; //add one to get rid of division by 0 if screen is black
    
    for (int i = 0; i < bestMatches.size(); i++) {
        cv::Mat currEnd = potentialLoopEnds.at(i);
        cv::Mat diff;
        cv::absdiff(start, currEnd, diff);
        float endDiff = cv::sum(diff)[0] + 1;
        float changeRatio = endDiff/startSum;
        
        //cout << "end of loop frame: " << matchIndeces.at(i) << endl;
        //add flow test here
        if (changeRatio <= minChange) {
            minChange = changeRatio;
            bestEnd = i;
        }
    }
    
    if (bestEnd >= 0) {
        vector<ofImage> loop;
        vector< ofImage> display;
        for (int j = frameStart; j < matchIndeces.at(bestEnd); j++) {
            vidPlayer.setFrame(j);
            vidPlayer.update();
            ofImage loopee;
            loopee.setFromPixels(vidPlayer.getPixels(), vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
            loop.push_back(loopee);
            ofImage displayIm;
            displayIm.clone(loopee);
            displayIm.resize(vidPlayer.getWidth()/numLoopsInRow, vidPlayer.getHeight()/numLoopsInRow);
            display.push_back(displayIm);
        }
        loopLengths.push_back(loop.size());
        loops.push_back(loop);
        displayLoops.push_back(display);
        //loopQuality.push_back(minChange);
        loopQuality.push_back(1 - (minChange/100));
    }
    
    loopPlayIdx.push_back(0);
    
    if (!ditchSimilarLoop()) {
        int newHeight = ceil((double)loops.size()/numLoopsInRow);
        cout << "newHeight: " << newHeight << endl;
        float rectTop = vidPlayer.getHeight() + (newHeight - 1)*vidPlayer.getHeight()/numLoopsInRow;
        float rectLeft = guiMatch->getGlobalCanvasWidth() + (vidPlayer.getWidth()/numLoopsInRow)*((loops.size() - 1)%numLoopsInRow);
        cout << "rectTop: " << rectTop << endl;
        cout << "rectLeft: " << rectLeft << endl;
        ofRectangle drawRect = ofRectangle(rectLeft, rectTop, vidPlayer.getWidth()/numLoopsInRow, vidPlayer.getHeight()/numLoopsInRow);
        loopDrawRects.push_back(drawRect);
        //cout << "num Loops: " << loopLengths.at(loopLengths.size()-1) << endl;
        //cout << newHeight << endl;
        //cout << vidPlayer.getHeight() + newHeight*vidPlayer.getHeight() << endl;
        ofSetWindowShape(ofGetWidth(), vidPlayer.getHeight() + newHeight*vidPlayer.getHeight()/numLoopsInRow);
    }
    bestMatches.clear();
    matchIndeces.clear();
}



bool ofApp::ditchSimilarLoop(){
    if (loops.size() == 1)
        return false;
    int numCheckPoints = 10;
    int prevArray [numCheckPoints]; //= {0,prevQuarter,prevHalf,prevThreeQuart,prevLength - 1};
    int newArray [numCheckPoints];// = {0,newQuarter,newHalf,newThreeQuart,newLength - 1};
    int prevLength = loops.at(loops.size() - 2).size();
    int newLength = loops.at(loops.size() - 1).size();
    for (int i = 0; i < numCheckPoints; i++) {
        prevArray[i] = (int)(i*prevLength/numCheckPoints);
        newArray[i] = (int)(i*newLength/numCheckPoints);
    }

    float changeRatio [numCheckPoints];
    //for (int i = 0; i < numCheckPoints; i++){
        cv::Mat prevLoopGray;
        cv::Mat newLoopGray;
        //cv::cvtColor(toCv(loops.at(loops.size()-2).at(prevArray[i])), prevLoopGray, CV_RGB2GRAY);
        //cv::cvtColor(toCv(loops.at(loops.size()-1).at(newArray[i])), newLoopGray, CV_RGB2GRAY);
    cv::cvtColor(toCv(loops.at(loops.size()-2).at(0)), prevLoopGray, CV_RGB2GRAY);
    cv::cvtColor(toCv(loops.at(loops.size()-1).at(0)), newLoopGray, CV_RGB2GRAY);
        cv::Mat prevLoopFrame;
        cv::Mat newLoopFrame;
        cv::resize(prevLoopGray, prevLoopFrame, imResize);
        cv::resize(newLoopGray, newLoopFrame, imResize);
        
        float prevLoopSum = cv::sum(prevLoopFrame)[0] + 1;
        //float newLoopSum = cv::sum(newLoopGray)[0] + 1;
        
        cv::Mat diff;
        cv::absdiff(prevLoopFrame, newLoopFrame, diff);
        float endDiff = cv::sum(diff)[0] + 1;
        //changeRatio[i] = endDiff/prevLoopSum;
    changeRatio[0] = endDiff/prevLoopSum;
    //}
    
    cout << "loopSimilarity: " << changeRatio[0] << endl;
    cout << "sim threshhold: " << (1 - loopThresh/100)*3 << endl;
    int simCount;
    /*for (int i = 0; i < numCheckPoints; i++){
        if(changeRatio[i] <= (1-loopThresh/100)*0.75)
            simCount++;
    }*/
    //if (simCount >= (int)(numCheckPoints/2)) {
    if(changeRatio[0] <= (1-loopThresh/100)*3){ // the first frames are very similar
        if (loopQuality.at(loopQuality.size() - 2) > loopQuality.at(loopQuality.size() -1)) {
            cout << "erasing last one" << endl;
            loopQuality.erase(loopQuality.end() - 1);
            loopLengths.erase(loopLengths.end() - 1);
            loops.erase(loops.end() - 1);
            loopPlayIdx.erase(loopPlayIdx.end() - 1);
            displayLoops.erase(displayLoops.end() - 1);
        }
        else{
            cout << "erasing second to last" << endl;
            loopQuality.erase(loopQuality.end() - 2 );
            loopLengths.erase(loopLengths.end() - 2);
            loops.erase(loops.end() - 2);
            loopPlayIdx.erase(loopPlayIdx.end() - 2);
            displayLoops.erase(displayLoops.end() - 2);
        }
        return true;

    }
    cout << "keeping loop" << endl;
    return false;
}


void ofApp::update(){
    if (!pausePlayback) {
        if (frameStart >= vidPlayer.getTotalNumFrames()){
            frameStart = 0;
            initEnds();
        }
        
        
        vidPlayer.setFrame(frameStart);
        vidPlayer.update();
        //firstFrame = toCv(vidPlayer);
        
        //vidPlayer.setUseTexture(false);
        if(checkForLoop()){
            //cout << "Loop Found" << endl;
        }
        
        //flow.calcOpticalFlow(vidPlayer.getPixelsRef());
        //loopIdx ++;
        frameStart ++;
        populateLoopEnds();
        //vidPlayer.setUseTexture(true);
        for (int i = 0; i < loopPlayIdx.size(); i++) {
            loopPlayIdx.at(i)++;
            if(loopPlayIdx.at(i) >= loopLengths.at(i))
                loopPlayIdx.at(i) = 0;
        }
    }
}


void ofApp::draw(){
    ofBackground(127);
    vidPlayer.setFrame(frameStart - 1);
    vidPlayer.update();
    vidPlayer.draw(guiMatch->getGlobalCanvasWidth(), 0);
    
    
    for (int i = 0; i < loops.size(); i++) {
        //ofImage loopFrame = loops.at(i).at(loopPlayIdx.at(i));
        ofImage loopFrame = displayLoops.at(i).at(loopPlayIdx.at(i));
        loopFrame.draw(loopDrawRects.at(i));
    }
    /*if(loopIdx >= loop.size())
        loopIdx = 0;
    if (loop.size() > 0) {
        loop.at(loopIdx).draw(guiMatch->getGlobalCanvasWidth(),0);
    }*/
    
    
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
    pausePlayback = true;

}

void ofApp::mouseReleased(int x, int y, int button){
    pausePlayback = false;
    for (int i = 0; i < loopDrawRects.size(); i++) {
        if (loopDrawRects.at(i).inside(x, y)) {
            saveGif(i);
        }
    }
    //saveGif();
}

void ofApp::saveGif(int i){
    ofxGifEncoder *giffy = new ofxGifEncoder();
    (*giffy).setup(vidPlayer.getWidth(), vidPlayer.getHeight(), 1/fps, 256);
    ofImage save;
    vector<ofImage> loop = loops.at(i);
    for (int j = 0; j < loops.at(i).size(); j++) {
        save.setFromPixels(loop.at(j).getPixels(), vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
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


void ofApp::exit() {
    for (int i = 0; i < gifses.size(); i++) {
        (*gifses.at(i)).exit();
    }
    
    guiMatch->saveSettings("guiMatchSettings.xml");
    
	delete guiMatch;
}

void ofApp::keyPressed(int key){
    /*if(guiMovement->hasKeyboardFocus())
    {
        return;
    }*/
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
			break;
            
		case 'p':
			bdrawPadding = !bdrawPadding;
			guiMatch->setDrawWidgetPaddingOutline(bdrawPadding);
			break;
            
		case '[':
			guiMatch->setDrawWidgetPadding(false);
			break;
            
		case ']':
			guiMatch->setDrawWidgetPadding(true);
			break;
			
        case '1':
            guiMatch->toggleVisible();
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
	//cout << "got event from: " << name << endl;
    if(name == "BUTTON"){
		ofxUIButton *button = (ofxUIButton *) e.getButton();
	}
    else if (name == "Toggle Min Movement"){
        minMovementBool = e.getToggle()->getValue();
        //minMovementBool = !minMovementBool;
        minMove->setVisible(minMovementBool);
        initEnds();
        //guiMatch->autoSizeToFitWidgets();
    }
    else if (name == "Toggle Max Movement"){
        maxMovementBool = e.getToggle()->getValue();
        //maxMovementBool = !maxMovementBool;
        maxMove->setVisible(maxMovementBool);
        initEnds();
        //guiMatch->autoSizeToFitWidgets();
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
    else if(name == "Length Range"){
        minPeriod = (int)(fps*minPeriodSecs);
        maxPeriod = (int)(fps*maxPeriodSecs);
        if (!pausePlayback)
            initEnds();
    }
}


void ofApp::setGuiMatch(){
	
	guiMatch = new ofxUISuperCanvas("Loop Settings");
    
    guiMatch->addSpacer();
    guiMatch->addLabel("Press 'h' to Hide GUIs", OFX_UI_FONT_SMALL);
    
    guiMatch->addSpacer();
    guiMatch->addLabel("Valid Loop Lengths",OFX_UI_FONT_SMALL);
    ofxUIRangeSlider *loopLength = guiMatch->addRangeSlider("Length Range", 0.25, 8.0, &minPeriodSecs, &maxPeriodSecs);
    loopLength->setIncrement(0.05);
    
    guiMatch->addSpacer();
	guiMatch->addLabel("Match Slider (percent)",OFX_UI_FONT_SMALL);
	guiMatch->addSlider("Accuracy", 80.0, 100.0, &loopThresh)->setTriggerType(OFX_UI_TRIGGER_ALL);
    
	guiMatch->addSpacer();
    guiMatch->addLabel("Min Movement (% change)",OFX_UI_FONT_SMALL);
    guiMatch->addToggle( "Toggle Min Movement", &minMovementBool);
    minMove = guiMatch->addSlider("Min Movement", 0.0, 100.0, &minMovementThresh);
    minMove->setTriggerType(OFX_UI_TRIGGER_ALL);
    minMove->setVisible(minMovementBool);
    
    guiMatch->addSpacer();
    guiMatch->addLabel("Max Movement (% change)",OFX_UI_FONT_SMALL);
    //maxMovementBool = true;
    guiMatch->addToggle("Toggle Max Movement", &maxMovementBool);
	maxMove = guiMatch->addSlider("Max Movement", 0.0, 100.0, &maxMovementThresh);
    maxMove->setTriggerType(OFX_UI_TRIGGER_BEGIN|OFX_UI_TRIGGER_ALL);
    //maxMove->setVisible(true);
    //maxMovementBool = false;
    
    guiMatch->autoSizeToFitWidgets();
    maxMove->setVisible(false);
	ofAddListener(guiMatch->newGUIEvent,this,&ofApp::guiEvent);
}



