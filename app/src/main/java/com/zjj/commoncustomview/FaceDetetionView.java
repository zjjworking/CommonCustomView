package com.zjj.commoncustomview;

import android.graphics.Bitmap;
import android.view.Surface;

/**
 * Created by zjj on 17/11/8.
 */

public class FaceDetetionView {
    static{
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("opencv_java3");
        System.loadLibrary("native-lib");

    }
    public native void loadModel(String detectModel);
    public native boolean process(Bitmap bitmap);
    public native void setSurface(Surface surface, int w, int h);
    public native void destroy();
}
