#include <android/log.h>

#include "calibrator.h"

Calibrator *createCalibrator(){
	 Calibrator *cal = (Calibrator *)malloc(sizeof(Calibrator)); //new(Calibrator);
     if (!cal)
         return NULL;
     memset(cal, 0, sizeof(Calibrator));

	 cal->previousDiff = 0;
	 cal->previousDirection = 0;
	 cal->directionChanges = 0;
	 cal->iteration = 0;
     return cal;
}

int signum(int x){
	if (x > 0) {
		return 1;
	} else if (x < 0) {
        return -1;
	} else {
		return 0;
	}
}

double calibratepro(Calibrator *cal, double maxVolumeRatio, int leftBandwidth, int rightBandwidth){
     int difference = leftBandwidth - rightBandwidth;
     int direction = (int) signum(difference);

     if (cal == NULL) {
     	return 0.0f;
     }

     if (cal->previousDirection != direction) {
         cal->directionChanges++;
         //Log.d("CALIBRATE", "increase directionchanges");
         cal->previousDirection = direction;
     }

     cal->iteration = ++(cal->iteration) % ITERATION_CYCLES;
     if (cal->iteration == 0) {
         //Log.d("Direction changes", directionChanges + "");
         if (cal->directionChanges >= UP_THRESHOLD) {
             maxVolumeRatio = maxVolumeRatio * UP_AMOUNT;
         }
         if (cal->directionChanges == DOWN_THRESHOLD) {
             maxVolumeRatio = maxVolumeRatio * DOWN_AMOUNT;
         }

         maxVolumeRatio = MIN(MAX_RATIO, maxVolumeRatio);
         maxVolumeRatio = MAX(MIN_RATIO, maxVolumeRatio);
         cal->directionChanges = 0;
     }

     return maxVolumeRatio;
}