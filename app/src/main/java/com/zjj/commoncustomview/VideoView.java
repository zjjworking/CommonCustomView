package com.zjj.commoncustomview;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by zjj on 17/11/1.
 */

public class VideoView extends SurfaceView{
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
    public VideoView(Context context) {
        super(context);
    }

    public VideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }
    private void init() {
        SurfaceHolder holder = getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }
    public void player(final String input) {
        new Thread(new Runnable() {
            @Override
            public void run() {
//        绘制功能 不需要交给SurfaveView        VideoView.this.getHolder().getSurface()
                render(input, VideoView.this.getHolder().getSurface());
            }
        }).start();
    }

    public native void render(String input, Surface surface);


}
