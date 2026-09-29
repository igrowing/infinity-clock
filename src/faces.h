// The clock faces: the different ways of showing the time. To add one, write `static void myFace(DateTime now)`
// in faces.cpp and append it to FACES there.
#pragma once
#include <RTClib.h>

uint8_t clockFaceCount();

// Draw the face number `mode`. False if there is no such face.
bool drawClockFace(uint8_t mode, DateTime now);

// Start measuring the length of the current second from now.
void restartSecondTimer();
