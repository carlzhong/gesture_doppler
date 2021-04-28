package com.jasperlu.test;

import com.jasperlu.doppler.Doppler;
import com.jasperlu.doppler.DopplerCVer;

public class TheDopplerCVer {
    private static DopplerCVer doppler;

    public static DopplerCVer getDoppler() {
        if (doppler == null) {
            doppler = new DopplerCVer();
        }
        return doppler;
    }
}
