#include "dopplerpro.h"

#include <android/log.h>

static short SHORT_MAX_VALUE = (short) 0x7FFF;

DopplerPro *gCtx = NULL;

int getHigherP2(int val);
void optimizeFrequency(DopplerPro *ctx, int minFreq, int maxFreq);
void smoothOutFreqs(DopplerPro *ctx);
void readMic(DopplerPro *ctx);

static void *startThread(void* ptr){
	//__android_log_print(ANDROID_LOG_INFO, "zj", "startThread11  %d", ptr);
	DopplerPro* pro = (DopplerPro *) ptr;
	pro->mRunning = 1;
    pro->handler((void *)pro);
	pro->mRunning = 0;
}

void handleCore(void* ptr){
    DopplerPro *ctx = (DopplerPro *) ptr;
    
    int bufferReadResult = 0;
    //获取每次读取buf的长度，根据需要调用
    if (ctx->mCBGetBufLen != NULL) {
         bufferReadResult = ctx->mCBGetBufLen(0);
    } else {
         bufferReadResult = 2048;
    }
    
    ctx->buffer = (short *)malloc(sizeof(short)*bufferReadResult);
    ctx->bufferSize = bufferReadResult;

    bufferReadResult = getHigherP2(bufferReadResult);
 
    //get higher p2 because buffer needs to be "filled out" for FFT
    bufferReadResult = getHigherP2(bufferReadResult);
    ctx->fftRealArray = (float *)malloc(sizeof(float)*bufferReadResult);
    memset(ctx->fftRealArray, 0, sizeof(float)*bufferReadResult);

    ctx->fft = createFFT(bufferReadResult, SAMPLE_RATE);

    if (ctx->fft == NULL) {
        return;
    }

    optimizeFrequency(ctx, MIN_FREQ, MAX_FREQ);
    readMic(ctx);

    __android_log_print(ANDROID_LOG_INFO, "zj", "handleCore0  %d, %d, %d", ptr, bufferReadResult, ctx->fft);
    while (!ctx->abort){
         readMic(ctx);
    }

    __android_log_print(ANDROID_LOG_INFO, "zj", "handleCore  %d, %d, %d", ptr, bufferReadResult, ctx->fft);
}

DopplerPro *createDopplerPro() {
	 DopplerPro *ctx = (DopplerPro *)malloc(sizeof(DopplerPro)); 
     if (!ctx)
         return NULL;

     memset(ctx, 0, sizeof(DopplerPro));

     ctx->cal = createCalibrator();
     ctx->mRunning = 0;
     ctx->handler = handleCore;
     ctx->mCBGetBufLen = NULL;
     ctx->mCBGetBufData = NULL;
     ctx->freqIndex = PRELIM_FREQ_INDEX;
     ctx->frequency = PRELIM_FREQ;
     ctx->abort     = 0;
     ctx->maxVolRatio = 0.05;
     ctx->calibrate = 1;
     //__android_log_print(ANDROID_LOG_INFO, "zj", "createDopplerPro  %d", ctx->cal);
     return ctx;
}

int startPro(DopplerPro *inst) {
     DopplerPro *ctx = (DopplerPro *)inst; 
     if (!ctx)
         return NULL;
	 pthread_mutex_init(&(ctx->mLock), NULL);
	 pthread_cond_init(&(ctx->mCondition), NULL);

	 pthread_create(&(ctx->mThread), NULL, startThread, ctx);
	 //__android_log_print(ANDROID_LOG_INFO, "zj", "startPro  %d", ctx);
     return 0;
}

int stopPro(DopplerPro *inst) {
    if (inst != NULL) {
        inst->abort = 1;
    }
	return 0;
}

void SetCallback(DopplerPro *inst, NativeCallBack func){
	DopplerPro *ctx = inst;
	if (!ctx)
		return;
    ctx->mCBGetBufLen = func;
}

void SetCallbackGetData(DopplerPro *inst, NativeCallBackGetData func) {
    DopplerPro *ctx = inst;
    if (!ctx)
        return;

    ctx->mCBGetBufData = func;
}

int getHigherP2(int val) {
    val--;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 8;
    val |= val >> 16;
    val++;
    return(val);
}

void readAndFFT(DopplerPro *ctx) {
    if (!ctx) {
        return;
    }

    //copy into old freqs array
    if (specSize(ctx->fft) != 0 && ctx->oldFreqs == NULL) {
        ctx->oldFreqs = (float *)malloc((specSize(ctx->fft))*sizeof(float));  //new float[fft.specSize()];
    }

    for (int i = 0; i < specSize(ctx); ++i) {
        ctx->oldFreqs[i] = getBand(ctx->fft, i);
    }

    int bufferReadResult = 0; 

    if (ctx->mCBGetBufData != NULL) {
         bufferReadResult = ctx->mCBGetBufData(0, ctx->buffer, ctx->bufferSize);
    } else {
         return;
    }

    //int bufferReadResult = microphone.read(buffer, 0, bufferSize);

    for (int i = 0; i < bufferReadResult; i++) {
        ctx->fftRealArray[i] = (float) ctx->buffer[i] / SHORT_MAX_VALUE; //32768.0
    }

    // //apply windowing
    for (int i = 0; i < bufferReadResult/2; ++i) {
        // Calculate & apply window symmetrically around center point
        // Hanning (raised cosine) window
        float winval = (float)(0.5+0.5*cos(PI*(float)i/(float)(bufferReadResult/2)));
        if (i > bufferReadResult/2)  winval = 0;
        ctx->fftRealArray[bufferReadResult/2 + i] *= winval;
        ctx->fftRealArray[bufferReadResult/2 - i] *= winval;
    }

    // // zero out first point (not touched by odd-length window)
    // //fftRealArray[0] = 0;

    forward(ctx->fft, ctx->fftRealArray);

    // //apply smoothing
    smoothOutFreqs(ctx);
}

void setFrequency(DopplerPro *ctx, float frequency) {
    ctx->frequency = frequency;
    ctx->freqIndex = freqToIndex(ctx->fft, frequency);
}

void optimizeFrequency(DopplerPro *ctx, int minFreq, int maxFreq) {
    if (!ctx || !ctx->fft)
        return;

    readAndFFT(ctx);
    int minInd = freqToIndex(ctx->fft, minFreq);
    int maxInd = freqToIndex(ctx->fft, maxFreq);

    int primaryInd = ctx->freqIndex;
    for (int i = minInd; i <= maxInd; ++i) {
        if (getBand(ctx->fft, i) > getBand(ctx->fft, primaryInd)) {
            primaryInd = i;
        }
    }
    setFrequency(ctx, indexToFreq(ctx->fft, primaryInd));
    // Log.d("NEW PRIMARY IND", fft.indexToFreq(primaryInd) + "");

    __android_log_print(ANDROID_LOG_INFO, "zj", "optimizeFrequency  end");
}

void smoothOutFreqs(DopplerPro *ctx) {
    for (int i = 0; i < specSize(ctx->fft); ++i) {
        float smoothedOutMag = SMOOTHING_TIME_CONSTANT * getBand(ctx->fft, i) + (1 - SMOOTHING_TIME_CONSTANT) * ctx->oldFreqs[i];
        setBand(ctx->fft, i, smoothedOutMag);
    }
}

int *getBandwidth(DopplerPro *ctx) {
    readAndFFT(ctx);

    //rename this
    int primaryTone = ctx->freqIndex;

    double normalizedVolume = 0;

    double primaryVolume = getBand(ctx->fft, primaryTone);

    int leftBandwidth = 0;

    do {
        leftBandwidth++;
        double volume = getBand(ctx->fft, primaryTone - leftBandwidth);
        normalizedVolume = volume/primaryVolume;
    } while (normalizedVolume > ctx->maxVolRatio && leftBandwidth < RELEVANT_FREQ_WINDOW);


    // //secondary bandwidths are for looking past the first minima to search for "split off" peaks, as per the paper
    int secondScanFlag = 0;
    int secondaryLeftBandwidth = leftBandwidth;

    // //second scan
    do {
        secondaryLeftBandwidth++;
        double volume = getBand(ctx->fft, primaryTone - secondaryLeftBandwidth);
        normalizedVolume = volume/primaryVolume;

        if (normalizedVolume > SECOND_PEAK_RATIO) {
            secondScanFlag = 1;
        }

        if (secondScanFlag == 1 && normalizedVolume < ctx->maxVolRatio ) {
            break;
        }
    } while (secondaryLeftBandwidth < RELEVANT_FREQ_WINDOW);

    if (secondScanFlag == 1) {
        leftBandwidth = secondaryLeftBandwidth;
    }

    int rightBandwidth = 0;

    do {
        rightBandwidth++;
        double volume = getBand(ctx->fft, primaryTone + rightBandwidth);
        normalizedVolume = volume/primaryVolume;
    } while (normalizedVolume > ctx->maxVolRatio && rightBandwidth < RELEVANT_FREQ_WINDOW);

    secondScanFlag = 0;
    int secondaryRightBandwidth = rightBandwidth;
    do {
        secondaryRightBandwidth++;
        double volume = getBand(ctx->fft, primaryTone + secondaryRightBandwidth);
        normalizedVolume = volume/primaryVolume;

        if (normalizedVolume > SECOND_PEAK_RATIO) {
            // Log.d("Volume", volume + "");
            // Log.d("Primary Vol", primaryVolume + "");
            secondScanFlag = 1;
        }

        if (secondScanFlag == 1 && normalizedVolume < ctx->maxVolRatio) {
            break;
        }
    } while (secondaryRightBandwidth < RELEVANT_FREQ_WINDOW);

    if (secondScanFlag == 1) {
        rightBandwidth = secondaryRightBandwidth;
    }

    ctx->bandwidth[0] = leftBandwidth;
    ctx->bandwidth[1] = rightBandwidth;
    return ctx->bandwidth;
    // return new int[]{leftBandwidth, rightBandwidth};
}



void readMic(DopplerPro *ctx) {
    int *bandwidths = getBandwidth(ctx);
    int leftBandwidth = bandwidths[0];
    int rightBandwidth = bandwidths[1];

    // if (isReadCallbackOn) {
    //     callReadCallback(leftBandwidth, rightBandwidth);
    // }

    // if (isGestureListenerAttached) {
    //     callGestureCallback(leftBandwidth, rightBandwidth);
    // }

    if (ctx->calibrate) {
        ctx->maxVolRatio = calibratepro(ctx->cal, ctx->maxVolRatio, leftBandwidth, rightBandwidth);
    }

    // if (repeat) {
    //     mHandler.post(new Runnable() {
    //         @Override
    //         public void run() {
    //             readMic();
    //         }
    //     });
    // }
}


