package com.jasperlu.doppler;

import java.nio.ShortBuffer;

public interface INativeListener {
    public int onGetBufLen();
    public ShortBuffer onGetBufData(int size);
}
