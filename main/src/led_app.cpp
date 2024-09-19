#include "led_app.h"

// extern Clock *sysClock;



void LED::setPadState(landing_pad_led_states_et state) {
    padState = state;
}

void LED::handler() {
    /*if(padState == eLED_PAD_ON) {
        if(TimeSpent(startTime, this->duration)) {
            padState = eLED_PAD_OFF;
        }
    }*/
}

landing_pad_led_states_et LED::getPadState() {
    return padState;
}

void LED::setLandingPadColor(int rVal, int gVal, int bVal) 
{
    padState = ESP_LP_LED_DEFINED_BY_RGB;
    padColor[0] = rVal;
    padColor[1] = gVal;
    padColor[2] = bVal;
}

int LED::getLandingPadColor(int val)
{
    return padColor[val];
}

void LED::setLeftEarColor(int rVal, int gVal, int bVal) {
    leftEarColor[0] = rVal;
    leftEarColor[1] = gVal;
    leftEarColor[2] = bVal;
}

void LED::setRightEarColor(int rVal, int gVal, int bVal) {
    rightEarColor[0] = rVal;
    rightEarColor[1] = gVal;
    rightEarColor[2] = bVal;
}

int LED::getLeftEarColor(int val) {
    return leftEarColor[val];
}


int LED::getRightEarColor(int val) {
    return rightEarColor[val];
}



bool LED::bleWriteLampData(int state, uint32_t duration, int rVal, int gVal, int bVal) {
   /* if(state) {
        if(!checkRGBValue(rVal) || !checkRGBValue(gVal) || !checkRGBValue(bVal)) {
            return false;
        }
        padState = eLED_PAD_ON;
        this->duration = duration * 1000;
        startTime = GetStartTime();        
        padColor[0] = rVal;
        padColor[1] = gVal;
        padColor[2] = bVal;
        return true;
    }
    else {
        if(padState == eLED_PAD_ON) {
            padState = eLED_PAD_OFF;
            return true;
        }
        return false;
    }
    return false;*/
    return true; //comment this later
}

bool LED::checkRGBValue(int value)
{
    if(value >= 0 && value <= 255) {
        return true;
    }
    return false;
}

