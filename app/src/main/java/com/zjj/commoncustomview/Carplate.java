package com.zjj.commoncustomview;

import android.graphics.Bitmap;

/**
 * Created by zjj on 17/11/15.
 */

public class Carplate {
    static{
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("native-lib");

    }
    public native void init(String svm, String ann, String ann_zh);

    public native void release();

    public native String recognition(Bitmap bitmap, Bitmap out);
}
