#ifndef __DOPPLERPRO_H__
#define __DOPPLERPRO_H__

#include <stdio.h>
#include <pthread.h>

#include "calibrator.h"
#include "fft.h"

#define SAMPLE_RATE 44100
#define MIN_FREQ 19000
#define MAX_FREQ 21000
#define SMOOTHING_TIME_CONSTANT 0.5f
#define PRELIM_FREQ_INDEX  20000
#define PRELIM_FREQ   20000
#define RELEVANT_FREQ_WINDOW  33
#define SECOND_PEAK_RATIO  0.3

// static final float SMOOTHING_TIME_CONSTANT = 0.5f;

typedef void (* handlercore)(void *p);
typedef int (*NativeCallBack)(int);
typedef int (*NativeCallBackGetData)(int, short*, int);
//static int DEFAULT_SAMPLE_RATE = 44100;

typedef struct dopplerPro{
	int                         calibrate;
    Calibrator                  *cal;
    pthread_t                   mThread;
    pthread_mutex_t     		mLock;
    pthread_cond_t				mCondition;	
    handlercore                 handler;
    int						    mRunning;

    short                       *buffer;
    int                         bufferSize;

    
    NativeCallBack              mCBGetBufLen;
    NativeCallBackGetData       mCBGetBufData;
    float                       *fftRealArray;
    FFT                         *fft;

    float                       *oldFreqs;

    int                         freqIndex;
    float                       frequency;
    double                      maxVolRatio;

    int                         abort;
    int                         bandwidth[2];
    
} DopplerPro;

DopplerPro *createDopplerPro();
int startPro(DopplerPro *inst);
int stopPro(DopplerPro *inst);

void SetCallback(DopplerPro *inst, NativeCallBack func);
void SetCallbackGetData(DopplerPro *inst, NativeCallBackGetData func);
#endif