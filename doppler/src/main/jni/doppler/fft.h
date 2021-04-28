#ifndef __FFT_H__
#define __FFT_H__
#include <stdio.h>
#include <pthread.h>

#define LINAVG  1
#define LOGAVG  2
#define NOAVG   3
#define PI      3.141592653589793

// static double PI = 3.141592653589793;

typedef struct  fft{
	//FourierTransform
    int timeSize;
    int sampleRate;
    float bandWidth;

    float *real;
    float *imag;
    float *spectrum;
    float *averages;
    int whichAverage;
    int octaves;
    int avgPerOctave;
    
    int spectrum_len;
    int averages_len;
    int real_len;
    int imag_len;

    //fft
    int   *reverse;
    float *sinlookup;
    float *coslookup;
}FFT;


FFT *createFFT(int ts, float sr);

#endif