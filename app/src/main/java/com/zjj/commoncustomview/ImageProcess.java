package com.zjj.commoncustomview;

import android.graphics.Bitmap;

/**
 * Created by zjj on 17/11/9.
 */

public class ImageProcess  {
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
    public static native void findIdNumber(Bitmap src, Bitmap out, Bitmap tpl);
    public static native Bitmap getIdNumber(Bitmap src, Bitmap tpl, Bitmap.Config config);
}
