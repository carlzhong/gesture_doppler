#include <android/log.h>
#include <math.h>
#include "fft.h"

// static double PI = 3.141592653589793;

void noAverages(FFT *f);
void allocateArrays(FFT *f);
float calcAvg(FFT *f, float lowFreq, float hiFreq); 
int freqToIndex(FFT *f, float freq);
float indexToFreq(FFT *f,  int i);
float getBandWidth(FFT *f);
int specSize(FFT *f);
float getBand(FFT *f, int i);

//fft
void buildReverseTable(FFT *f);
void bitReverseSamples(FFT *f, float *samples, int startAt);
void buildTrigTables(FFT *f);
void bitReverseComplex(FFT *f);
void scaleBand(FFT *f, int i, float s);
void setBand(FFT *f, int i, float a);
void forward(FFT *f, float *buffer, int len);
void forward1(FFT *f, float *buffer, int len, int startAt);
float sinself(FFT *f, int i);
float cosself(FFT *f, int i);


FFT *createFFT(int ts, float sr){
     __android_log_print(ANDROID_LOG_INFO, "zj", "createFFT1");
     FFT *f = (FFT *)malloc(sizeof(FFT)); //new(Calibrator);
     if (!f)
         return NULL;
     memset(f, 0, sizeof(FFT));

     f->timeSize = ts;
     f->sampleRate = (int)sr;
     f->bandWidth = (2.0f / f->timeSize) * ((float)(f->sampleRate) / 2.0f);

     noAverages(f);
     allocateArrays(f);
     if ((ts & (ts - 1)) != 0) {
         free(f);
         return NULL;
     }
     //__android_log_print(ANDROID_LOG_INFO, "zj", "createFFT4");
     buildReverseTable(f);
     buildTrigTables(f);
     return f;
}

void noAverages(FFT *f) {
     f->averages = NULL;
     f->whichAverage = NOAVG;
}

void allocateArrays(FFT *f) {
    f->spectrum = (float *)malloc((f->timeSize / 2 + 1)*sizeof(float)); //new float[f->timeSize / 2 + 1];
    f->spectrum_len = f->timeSize / 2 + 1;
    f->real = (float *)malloc(f->timeSize*sizeof(float));//new float[f->timeSize];
    f->real_len = f->timeSize;
    f->imag = (float *)malloc(f->timeSize*sizeof(float));//new float[timeSize];
    f->imag_len = f->timeSize;
}

void linAverages(FFT *f, int numAvg) {
    if (numAvg > f->spectrum_len/ 2) {
        return;
    } else {
        f->averages = (float *)malloc(numAvg*sizeof(float)); //new float[numAvg];
        f->averages_len = numAvg;
    }
    f->whichAverage = LINAVG;
}

void logAverages(FFT *f, int minBandwidth, int bandsPerOctave) {
    float nyq = (float) (f->sampleRate) / 2.0f;
    f->octaves = 1;
    while ((nyq /= 2) > minBandwidth) {
        f->octaves++;
    }
    f->avgPerOctave = bandsPerOctave;
    f->averages = (float *)malloc(f->octaves * bandsPerOctave * sizeof(float));//new float[octaves * bandsPerOctave];
    f->averages_len = f->octaves * bandsPerOctave;
    f->whichAverage = LOGAVG;
}

void setComplex(FFT *f, float *r, int rlen, float *i, int ilen) {
    if (f->real_len != rlen && f->imag_len != ilen) {
    } else {
        memcpy(f->real, r, f->real_len*sizeof(float));
        memcpy(f->imag, i, f->imag_len*sizeof(float));
    }
}



void fillSpectrum(FFT *f) {
    for (int i = 0; i < f->spectrum_len; i++) {
        f->spectrum[i] = (float) sqrt(f->real[i] * f->real[i] + f->imag[i] * f->imag[i]);
    }

    if (f->whichAverage == LINAVG) {
        int avgWidth = (int) f->spectrum_len / f->averages_len;
        for (int i = 0; i < f->averages_len; i++) {
            float avg = 0;
            int j;
            for (j = 0; j < avgWidth; j++) {
                int offset = j + i * avgWidth;
                if (offset < f->spectrum_len) {
                    avg += f->spectrum[offset];
                } else {
                    break;
                }
            }
            avg /= j + 1;
            f->averages[i] = avg;
        }
    } else if (f->whichAverage == LOGAVG) {
        for (int i = 0; i < f->octaves; i++) {
            float lowFreq, hiFreq, freqStep;
            if (i == 0) {
                lowFreq = 0;
            } else {
                lowFreq = (f->sampleRate / 2) / (float) pow(2, f->octaves - i);
            }
            hiFreq = (f->sampleRate / 2) / (float) pow(2, f->octaves - i - 1);
            freqStep = (hiFreq - lowFreq) / f->avgPerOctave;
            float ff = lowFreq;
            for (int j = 0; j < f->avgPerOctave; j++) {
                int offset = j + i * f->avgPerOctave;
                f->averages[offset] = calcAvg(f, ff, ff + freqStep);
                ff += freqStep;
            }
        }
    }

    //for (int i = 0; i < spectrum.length; i++) {
    //     spectrum[i] = (float) Math.sqrt(real[i] * real[i] + imag[i] * imag[i]);
    // }

    // if (whichAverage == LINAVG)
    // {
    //     int avgWidth = (int) spectrum.length / averages.length;
    //     for (int i = 0; i < averages.length; i++)
    //     {
    //         float avg = 0;
    //         int j;
    //         for (j = 0; j < avgWidth; j++)
    //         {
    //             int offset = j + i * avgWidth;
    //             if (offset < spectrum.length)
    //             {
    //                 avg += spectrum[offset];
    //             }
    //             else
    //             {
    //                 break;
    //             }
    //         }
    //         avg /= j + 1;
    //         averages[i] = avg;
    //     }
    // }
    // else if (whichAverage == LOGAVG)
    // {
    //     for (int i = 0; i < octaves; i++)
    //     {
    //         float lowFreq, hiFreq, freqStep;
    //         if (i == 0)
    //         {
    //             lowFreq = 0;
    //         }
    //         else
    //         {
    //             lowFreq = (sampleRate / 2) / (float) Math.pow(2, octaves - i);
    //         }
    //         hiFreq = (sampleRate / 2) / (float) Math.pow(2, octaves - i - 1);
    //         freqStep = (hiFreq - lowFreq) / avgPerOctave;
    //         float f = lowFreq;
    //         for (int j = 0; j < avgPerOctave; j++)
    //         {
    //             int offset = j + i * avgPerOctave;
    //             averages[offset] = calcAvg(f, f + freqStep);
    //             f += freqStep;
    //         }
    //     }
    // }
}


float calcAvg(FFT *f, float lowFreq, float hiFreq) {
    int lowBound = freqToIndex(f, lowFreq);
    int hiBound = freqToIndex(f, hiFreq);
    float avg = 0;
    for (int i = lowBound; i <= hiBound; i++) {
        avg += f->spectrum[i];
    }
    avg /= (hiBound - lowBound + 1);
    return avg;
}


int freqToIndex(FFT *f, float freq) {
    // special case: freq is lower than the bandwidth of spectrum[0]
    if (freq < getBandWidth(f) / 2) return 0;
    // special case: freq is within the bandwidth of spectrum[spectrum.length - 1]
    if (freq > f->sampleRate / 2 - getBandWidth(f) / 2) return f->spectrum_len - 1;
    // all other cases
    float fraction = freq / (float) f->sampleRate;
    int i = (int)round(f->timeSize * fraction);
    return i;
}

float indexToFreq(FFT *f,  int i) {
    float bw = getBandWidth(f);
    // special case: the width of the first bin is half that of the others.
    //               so the center frequency is a quarter of the way.
    if ( i == 0 ) return bw * 0.25f;
    // special case: the width of the last bin is half that of the others.
    if ( i == f->spectrum_len- 1 ) {
        float lastBinBeginFreq = (f->sampleRate / 2) - (bw / 2);
        float binHalfWidth = bw * 0.25f;
        return lastBinBeginFreq + binHalfWidth;
    }
    // the center frequency of the ith band is simply i*bw
    // because the first band is half the width of all others.
    // treating it as if it wasn't offsets us to the middle
    // of the band.
    return i*bw;
}


float getBandWidth(FFT *f) {
    return f->bandWidth;
}

int specSize(FFT *f) {
    return f->spectrum_len;
}

float getBand(FFT *f, int i) {
    if (i < 0) i = 0;
    if (i > f->spectrum_len - 1) i = f->spectrum_len - 1;
    return f->spectrum[i];
}


//fft
void scaleBand(FFT *f, int i, float s) {
    if (s < 0) {
        // Minim.error("Can't scale a frequency band by a negative value.");
        return;
    }

    f->real[i] *= s;
    f->imag[i] *= s;
    f->spectrum[i] *= s;

    if (i != 0 && i != f->timeSize / 2) {
        f->real[f->timeSize - i] = f->real[i];
        f->imag[f->timeSize - i] = -(f->imag[i]);
    }
}

void setBand(FFT *f, int i, float a) {
    if (a < 0) {
        // Minim.error("Can't set a frequency band to a negative value.");
        return;
    }
    if (f->real[i] == 0 && f->imag[i] == 0) {
        f->real[i] = a;
        f->spectrum[i] = a;
    } else {
        f->real[i] /= f->spectrum[i];
        f->imag[i] /= f->spectrum[i];
        f->spectrum[i] = a;
        f->real[i] *= f->spectrum[i];
        f->imag[i] *= f->spectrum[i];
    }
    if (i != 0 && i != f->timeSize / 2) {
        f->real[f->timeSize - i] = f->real[i];
        f->imag[f->timeSize - i] = -(f->imag[i]);
    }
}

void buildReverseTable(FFT *f) {
    int N = f->timeSize;
    f->reverse = (int *)malloc((N)*sizeof(int));  //new int[N];

    // // set up the bit reversing table
    f->reverse[0] = 0;
    for (int limit = 1, bit = N / 2; limit < N; limit <<= 1, bit >>= 1)
        for (int i = 0; i < limit; i++)
            f->reverse[i + limit] = f->reverse[i] + bit;
}

void bitReverseSamples(FFT *f, float *samples, int startAt) {
    for (int i = 0; i < f->timeSize; ++i) {
        f->real[i] = samples[ startAt + f->reverse[i] ];
        f->imag[i] = 0.0f;
    }
}

void bitReverseComplex(FFT *f) {
    float *revReal = (float *)malloc((f->real_len)*sizeof(float));  //new float[real.length];
    float *revImag = (float *)malloc((f->imag_len)*sizeof(float)); //new float[imag.length];
    for (int i = 0; i < f->real_len; i++) {
        revReal[i] = f->real[f->reverse[i]];
        revImag[i] = f->imag[f->reverse[i]];
    }
 
    free(f->real);
    free(f->imag);

    f->real = revReal;
    f->imag = revImag;
}

void buildTrigTables(FFT *f) {
    int N = f->timeSize;
    //__android_log_print(ANDROID_LOG_INFO, "zj", "buildTrigTables1  %d", N);
    f->sinlookup = (float *)malloc((N)*sizeof(float)); //new float[N];
    f->coslookup = (float *)malloc((N)*sizeof(float)); //new float[N];
    
    for (int i = 0; i < N; i++) {
        f->sinlookup[i] = (float) sin(-(float) PI / i);
        f->coslookup[i] = (float) cos(-(float) PI / i);
    }
}

void fft(FFT *f) {
    for (int halfSize = 1; halfSize < f->real_len; halfSize *= 2) {
        // float k = -(float)Math.PI/halfSize;
        // phase shift step
        // float phaseShiftStepR = (float)Math.cos(k);
        // float phaseShiftStepI = (float)Math.sin(k);
        // using lookup table
        float phaseShiftStepR = (float)cosself(f, halfSize);
        float phaseShiftStepI = (float)sinself(f, halfSize);
        // current phase shift
        float currentPhaseShiftR = 1.0f;
        float currentPhaseShiftI = 0.0f;
        for (int fftStep = 0; fftStep < halfSize; fftStep++) {
            for (int i = fftStep; i < f->real_len; i += 2 * halfSize) {
                int off = i + halfSize;
                float tr = (currentPhaseShiftR * f->real[off]) - (currentPhaseShiftI * f->imag[off]);
                float ti = (currentPhaseShiftR * f->imag[off]) + (currentPhaseShiftI * f->real[off]);
                f->real[off] = f->real[i] - tr;
                f->imag[off] = f->imag[i] - ti;
                f->real[i] += tr;
                f->imag[i] += ti;
            }
            float tmpR = currentPhaseShiftR;
            currentPhaseShiftR = (tmpR * phaseShiftStepR) - (currentPhaseShiftI * phaseShiftStepI);
            currentPhaseShiftI = (tmpR * phaseShiftStepI) + (currentPhaseShiftI * phaseShiftStepR);
        }
    }
}

void forward(FFT *f, float *buffer, int len) {
    if (len != f->timeSize) {
        //    Minim.error("FFT.forward: The length of the passed sample buffer must be equal to timeSize().");
        return;
    }
    //  doWindow(buffer);
    // copy samples to real/imag in bit-reversed order
    bitReverseSamples(f, buffer, 0);
    // perform the fft
    fft(f);
    // fill the spectrum buffer with amplitudes
    fillSpectrum(f);
}

void forward1(FFT *f, float *buffer, int len, int startAt) {
    if ( len - startAt < f->timeSize ) {
        return;
    }

    //   windowFunction.apply( buffer, startAt, timeSize );
    bitReverseSamples(f, buffer, startAt);
    fft(f);
    fillSpectrum(f);
}

void inverse(FFT *f, float *buffer, int len) {
    if (len > f->real_len) {
        //   Minim.error("FFT.inverse: the passed array's length must equal FFT.timeSize().");
        return;
    }
    // conjugate
    for (int i = 0; i < f->timeSize; i++) {
        f->imag[i] *= -1;
    }
    bitReverseComplex(f);
    fft(f);
    // copy the result in real into buffer, scaling as we do
    for (int i = 0; i < len; i++) {
        buffer[i] = f->real[i] / f->real_len;
    }
}

float sinself(FFT *f, int i) {
    return f->sinlookup[i];
}

float cosself(FFT *f, int i) {
    return f->coslookup[i];
}

