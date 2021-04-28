#ifndef __CALIBRATOR_H__
#define __CALIBRATOR_H__
#include <stdio.h>
#include <pthread.h>

#define ITERATION_CYCLES  20
#define UP_THRESHOLD  5
#define DOWN_THRESHOLD 0
#define UP_AMOUNT 1.1
#define DOWN_AMOUNT 0.9
#define MAX_RATIO 0.95
#define MIN_RATIO 0.0001

#define MAX(a,b) a>b?a:b
#define MIN(a,b) a>b?b:a

typedef struct  calibrator{
    double  previousDiff;
    int     previousDirection;
    double  directionChanges;
    int     iteration;	
}Calibrator;

Calibrator *createCalibrator();
//double calibratepro(double maxVolumeRatio, int leftBandwidth, int rightBandwidth);

#endif