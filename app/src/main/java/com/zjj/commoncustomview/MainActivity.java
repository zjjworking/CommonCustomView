package com.zjj.commoncustomview;

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Environment;
import android.os.IBinder;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    VideoView videoView;
    ZJJPlayer zjjPlayer;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        videoView= (VideoView) findViewById(R.id.surface);
        zjjPlayer = new ZJJPlayer();
    }


    public native void open(String inputStr,String outStr);

    public void load(View view) {
        String input = new File(Environment.getExternalStorageDirectory(),"input.mp4").getAbsolutePath();
        String  output= new File(Environment.getExternalStorageDirectory(), "output.yuv").getAbsolutePath();
        open(input, output);
    }

    public void mPlay(View view) {

        //原生视频播放
        String input = new File(Environment.getExternalStorageDirectory(),"input.mp4").getAbsolutePath();
        videoView.player(input);

        //使用AudioTrack做音频播放
//        String cput = new File(Environment.getExternalStorageDirectory(),"input.mp3").getAbsolutePath();
//        String output = new File(Environment.getExternalStorageDirectory(),"output.pcm").getAbsolutePath();
//        ZJJPlayer davidPlayer = new ZJJPlayer();
//        davidPlayer.sound(cput,output);

        //使用FFmpeg做音频播放
        zjjPlayer.player();

    }

    public void stop(View view) {
        zjjPlayer.stop();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.FFmpeg) {
            startActivity(new Intent(MainActivity.this, FFmpegActivity.class));
            return true;
        } else if(id == R.id.faceDetetion){
            startActivity(new Intent(MainActivity.this, FaceDetetionViewActivity.class));
            return true;
        }
        else if(id == R.id.imageProcess){
            startActivity(new Intent(MainActivity.this, ImageProcessActivity.class));
            return true;
        }
        else if(id == R.id.Carplate){
            startActivity(new Intent(MainActivity.this, CarplateActivity.class));
            return true;
        }
// else if(id == R.id.DemoAct){
//
//            return true;
//        }else if(id == R.id.itemTouchHelper){
//
//            return true;
//        }
        return super.onOptionsItemSelected(item);
    }

    private ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {

        }
    }

}
