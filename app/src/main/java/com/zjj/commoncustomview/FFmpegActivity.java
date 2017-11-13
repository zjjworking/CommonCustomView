package com.zjj.commoncustomview;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.View;

/**
 * Created by zjj on 17/11/6.
 */

public class FFmpegActivity extends AppCompatActivity{
    SurfaceView surfaceView;
    ZJJPlayer player;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ffmpeg);
        surfaceView = (SurfaceView)findViewById(R.id.surface);
        player = new ZJJPlayer();
        player.setSurfaceView(surfaceView);
    }

    public void palyer(View view) {
        //本地所有视频都可以播放
        //流媒体
        player.playJava("rtmp://live.hkstv.hk.lxdns.com/live/hks");
    }

    public void stop(View view) {
        player.release();
    }
}
