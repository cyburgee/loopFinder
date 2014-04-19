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
    maxPeriod = (int)fps*5;
    cout<< "min period: " << minPeriod << endl;
    cout << "max period: " << maxPeriod << endl;
    
    //sampleRate = 44100;
    //channels = 2;
    //fileName = "testMovie";
    //fileExt = ".mp4";
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    bRecording = false;
    //ofEnableAlphaBlending();
    
    scale = 10;
    imResize = cv::Size(vidPlayer.getWidth()/scale,vidPlayer.getHeight()/scale);
    
    minMovementThresh = 1;//MAXFLOAT;
    
    maxMovementThresh = 20;
    
    loopThresh = 0.05;
    
    minChangeRatio = MAXFLOAT;
    
    loopFound = false;
    loopIdx = 0;
    
    frameStart = 1000;
    initEnds();
    
    ofSetWindowShape(vidPlayer.getWidth()*2, vidPlayer.getHeight()*2);
    
    gifNum = 0;
    ofAddListener(ofxGifEncoder::OFX_GIF_SAVE_FINISHED, this, &ofApp::onGifSaved);
    //ofSetFrameRate(30); //CHANGE THIS
}


void ofApp::initEnds(){
    
    if (potentialLoopEnds.size() > 0){
        for (int i = 0; i < potentialLoopEnds.size(); i++) {
            free(potentialLoopEnds.at(i));
        }
        potentialLoopEnds.clear();
    }
    vidPlayer.setFrame(frameStart + minPeriod);
    for (int i = frameStart + minPeriod; i < frameStart + maxPeriod && i <= vidPlayer.getTotalNumFrames(); i ++) {
        cv::Mat *currFrame = new cv::Mat();

        getMatFromFrameNum(currFrame, i);
        
        potentialLoopEnds.push_back(currFrame);
        
    }
    /*while(vidPlayer.getCurrentFrame() <= vidPlayer.getTotalNumFrames() && vidPlayer.getCurrentFrame() < frameStart + maxPeriod) {
        cv::Mat currGray;
        cv::cvtColor(toCv(vidPlayer), currGray, CV_RGB2GRAY);
        cv::Mat currFrame;
        cv::resize(currGray, currFrame, imResize);
        cv::Mat *currFrame = new cv::Mat();
        getMatFromFrameNum(currFrame, vidPlayer.getCurrentFrame());
        
        potentialLoopEnds.push_back(currFrame);
        cout << "adding Frame number: " << vidPlayer.getCurrentFrame() << endl;
        vidPlayer.nextFrame();
    }
    vidPlayer.setFrame(frameStart);*/
}

void ofApp::populateLoopEnds(){
    /*if (potentialLoopEnds.size() > 0){
        free(*potentialLoopEnds.begin());
        potentialLoopEnds.erase(potentialLoopEnds.begin());
        int endIdx = frameStart + maxPeriod;
        //cout << "endIdx: " << endIdx << endl;
        if (endIdx < vidPlayer.getTotalNumFrames()){
            vidPlayer.setFrame(endIdx);
            
            //cv::Mat endGray;
            //cv::cvtColor(toCv(vidPlayer), endGray, CV_RGB2GRAY);
            //cv::Mat endFrame;
            //cv::resize(endGray, endFrame, imResize);
            cv::Mat *endFrame = new cv::Mat();
            getMatFromFrameNum(endFrame, endIdx);
            
            potentialLoopEnds.push_back(endFrame);
        }
    }
    else{
        initEnds();
    }
    vidPlayer.setFrame(frameStart);*/
    if (potentialLoopEnds.size() > 0){
        free(*potentialLoopEnds.begin());
        potentialLoopEnds.erase(potentialLoopEnds.begin());
        int endIdx = frameStart + maxPeriod;
            //cout << "endIdx: " << endIdx << endl;
        if (endIdx < vidPlayer.getTotalNumFrames()){
            cv::Mat *endFrame = new cv::Mat();
            getMatFromFrameNum(endFrame, endIdx);
            
            vidPlayer.setFrame(endIdx);
            
            potentialLoopEnds.push_back(endFrame);
        }
    }
    else{
        initEnds();
    }
}


bool ofApp::checkForLoop(){
    /*cv::Mat startGray;
    cv::cvtColor(firstFrame, startGray, CV_RGB2GRAY);
    cv::Mat start;
    cv::resize(startGray, start, imResize);*/
    cv::Mat *start = new cv::Mat();
    getMatFromFrameNum(start, frameStart);
    //cv::Mat start = *startGray;
    
    float startSum = cv::sum(*start)[0] + 1; //add one to get rid of multiplication by 0 if screen is black
    //cout << "Start Sum: " << startSum << endl;
    /*
    if (startSum == 0) {
        return false;
    }*/
    
    for (int i = 0; i < potentialLoopEnds.size(); i++) {
        //cout << "against frame num: " << frameStart + minPeriod + i << endl;
        cv::Mat *currEnd = potentialLoopEnds.at(i);
        cv::Mat diff;
        cv::absdiff(*start, *currEnd, diff);
        float endDiff = cv::sum(diff)[0] + 1;
        //cout << "endDiff: " << endDiff << endl;;
        float changeRatio = endDiff/startSum;
        //cout << "changeRatio: " << changeRatio << endl;
        
        if (changeRatio < minChangeRatio) {
            minChangeRatio = changeRatio;
            cout << "min Ratio: " << minChangeRatio << endl;
        }
        
        if (changeRatio < loopThresh) {
            potentialEndIdx = frameStart + minPeriod + i;
            if (hasMovement()){// && !loopFound){
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
                
                free(start);
                return true;
            }
            else{
                frameStart = potentialEndIdx - 1;
                vidPlayer.setFrame(frameStart);
                //cout << "no movement, setting frame to: " << vidPlayer.getCurrentFrame() << endl;
                initEnds();
                //potentialLoopEnds.clear();
                free(start);
                return false;
            }
        }
    }

    //cout << "no Loop" << endl;
    free(start);
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
    
    //cout << "current frame start: " << frameStart << endl;
    //cout << "Frame Number: " << vidPlayer.getCurrentFrame() << endl;
    if (frameStart >= vidPlayer.getTotalNumFrames())
        frameStart = 0;
    
    vidPlayer.setFrame(frameStart);
    firstFrame = toCv(vidPlayer);
    
    if(checkForLoop()){
        //cout << "Loop Found" << endl;
    }
    
    //flow.calcOpticalFlow(vidPlayer.getPixelsRef());
    loopIdx ++;
    populateLoopEnds();
    frameStart += 1;//(int)fps/15;
}


void ofApp::draw(){
    
    //cout << "Frame Number: " << vidPlayer.getCurrentFrame() << endl;
    vidPlayer.setFrame(frameStart);
    //scout << "Frame Number: " << vidPlayer.getCurrentFrame() << endl;
    im.setFromPixels(vidPlayer.getPixels(), vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
    im.draw(vidPlayer.getWidth(), vidPlayer.getHeight());
    
    if(loopIdx >= loop.size())
        loopIdx = 0;
    if (loop.size() > 0) {
        loop.at(loopIdx).draw(0,0);
    }
    
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
}

void ofApp::keyReleased(int key){
    if(key=='r'){
        bRecording = !bRecording;
        if(bRecording && !vidRecorder.isInitialized()) {
            //vidRecorder.setup(fileName+ofGetTimestampString()+fileExt, ofGetWidth(), ofGetHeight(), 25, sampleRate, channels);
            vidRecorder.setup(fileName+ofGetTimestampString()+fileExt, ofGetWidth(),ofGetHeight(), 25); // no audio
            //            vidRecorder.setup(fileName+ofGetTimestampString()+fileExt, 0,0,0, sampleRate, channels); // no video
        }
    }
    if(key=='c'){
        bRecording = false;
        vidRecorder.close();
    }
    
}



