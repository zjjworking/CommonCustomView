package com.zjj.commoncustomview;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by zjj on 17/11/1.
 */

public class ZJJPlayer implements SurfaceHolder.Callback{
    static{
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("opencv_java3");
        System.loadLibrary("swscale-3");
        System.loadLibrary("native-lib");
    }

    private SurfaceView surfaceView;

    public void playJava(String path){
        if(surfaceView == null){
            return;
        }
        play(path);
    }
    public void setSurfaceView(SurfaceView surfaceView){
        this.surfaceView = surfaceView;
        dispaly(surfaceView.getHolder().getSurface());
        surfaceView.getHolder().addCallback(this);
    }

    public native void sound(String input, String output);

    public native void player();

    public native void stop();

    public native void play(String path);

    public native void dispaly(Surface surface);


    public native void release();

    private AudioTrack audioTrack;

    /**
     *
     * @param sampleRateInHz 采样率
     * @param nb_channals 通道数
     */
    public void createAudio(int sampleRateInHz,int nb_channals){
        int channaleConfig;
        if(nb_channals == 1){
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        }else if(nb_channals == 2){
            channaleConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }else {
            channaleConfig = AudioFormat.CHANNEL_OUT_MONO;
        }
        int buffersize = AudioTrack.getMinBufferSize(sampleRateInHz,channaleConfig,AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,sampleRateInHz,channaleConfig,AudioFormat.ENCODING_PCM_16BIT,
                buffersize,AudioTrack.MODE_STREAM);
        audioTrack.play();
    }

    /**
     * C传入音频数据
     * @param buffer
     * @param length
     */
    public synchronized void playTrack(byte[] buffer,int length){
        if(audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING){
            audioTrack.write(buffer,0,length);
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        dispaly(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
