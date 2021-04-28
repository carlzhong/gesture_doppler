package com.jasperlu.doppler;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import com.jasperlu.doppler.FFT.FFT;

import java.nio.ByteBuffer;
import java.nio.ShortBuffer;

public class DopplerCVer implements INativeListener{
    private int bufferSize = 2048;
    private int SAMPLE_RATE = 44100;
    public  final float PRELIM_FREQ = 20000;
    private short[] buffer;

    private FrequencyPlayer frequencyPlayer;
    private AudioRecord microphone;
    private long inst = 0;

    private static final double MAX_VOL_RATIO_DEFAULT = 0.05;
    //private static final double SECOND_PEAK_RATIO = 0.3;
    public static double maxVolRatio = MAX_VOL_RATIO_DEFAULT;

    //callbacks
    private boolean isGestureListenerAttached = false;
    private OnGestureListener gestureListener;
    private boolean isReadCallbackOn = false;
    private OnReadCallback readCallback;

    private static INativeListener listener= null;
    static {
        System.loadLibrary("doppler");
    }

    @Override
    public int onGetBufLen() {
        int ret = microphone.read(buffer, 0, bufferSize);
        Log.i("zj", "onGetBufLen "+ret);
        return ret;
    }

    @Override
    public ShortBuffer onGetBufData(int size) {
        int ret = microphone.read(buffer, 0, size);
        if (ret > 0 ) {
            ShortBuffer shortBuffer = ShortBuffer.allocate(ret);
            shortBuffer.put(buffer);
            //shortBuffer.capacity();
            return shortBuffer;
        }
        return null;
    }

//    @Override
//    public int onGetBufData(short[] buf, int size) {
//        int ret = microphone.read(buf, 0, size);
//        Log.i("zj", "onGetBufData "+ret);
//        return ret;
//    }


    public interface OnReadCallback {
        //bandwidths are the number to the left/to the right from the pilot tone the shift was
        public void onBandwidthRead(int leftBandwidth, int rightBandwidth);
        //for testing/graphing as well
        public void onBinsRead(double[] bins);
    }

    public interface OnGestureListener {
        //swipe towards
        public void onPush();
        //swipe away
        public void onPull();
        //self-explanatory
        public void onTap();
        public void onDoubleTap();

        public void onNothing();
    }


    public DopplerCVer(){
        bufferSize = AudioRecord.getMinBufferSize(SAMPLE_RATE, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
        buffer = new short[bufferSize];
        //麦克风输出
        frequencyPlayer = new FrequencyPlayer(PRELIM_FREQ);
        //麦克风输入
        microphone = new AudioRecord(MediaRecorder.AudioSource.VOICE_RECOGNITION, SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, bufferSize);

        inst = nativeCreateInstance();
        listener = this;
    }

    public boolean start(){
        if (inst == 0){
            return false;
        }

        frequencyPlayer.play();
        try{
            microphone.startRecording();
            //int bufferReadResult = microphone.read(buffer, 0, bufferSize);
            //bufferReadResult = getHigherP2(bufferReadResult);
            //get higher p2 because buffer needs to be "filled out" for FFT
            //fftRealArray = new float[getHigherP2(bufferReadResult)];
            //fft = new FFT(getHigherP2(bufferReadResult), SAMPLE_RATE);
            Log.i("zj", "doppler cver start "+inst);
            nativeStart(inst);
        } catch (Exception e){
            e.printStackTrace();
            return false;
        }
        return true;
    }

    public boolean pause() {
        try {
            microphone.stop();
            frequencyPlayer.pause();
            nativeStop(inst);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public void setOnReadCallback(OnReadCallback callback) {
        readCallback = callback;
        isReadCallbackOn = true;
    }

    public void setOnGestureListener(OnGestureListener listener) {
        gestureListener = listener;
        isGestureListenerAttached = true;
    }

    public static int GetBufLenFromNative() {
        Log.i("zj", "GetBufLenFromNative "+listener);
        if (listener != null) {
            return listener.onGetBufLen();
        }
        return  0;
    }

    public static ShortBuffer getBufDataFromNative(int bufferSize){
        if (listener != null) {
            return listener.onGetBufData(bufferSize);
        }
        return  null;
    }



    private static native long nativeCreateInstance();
    private static native int nativeStart(long inst);
    private static native int nativeStop(long inst);
}
