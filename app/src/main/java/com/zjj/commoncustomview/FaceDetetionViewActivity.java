package com.zjj.commoncustomview;

import android.app.ProgressDialog;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Created by zjj on 17/11/9.
 */

public class FaceDetetionViewActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private final static String TAG = "ZJJ";

    SurfaceView surfaceView;
    private ProgressDialog pd;

    FaceDetetionView faceDetetionView;

    private Bitmap bm;


    private void showLoading() {
        if (pd == null) {
            pd = new ProgressDialog(this);
            pd.setIndeterminate(true);
        }
        pd.show();
    }

    private void dismissLoading() {
        if (pd != null) {
            pd.dismiss();
        }
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_face_detetion);

        surfaceView = (SurfaceView)findViewById(R.id.surfaceView);
        faceDetetionView = new FaceDetetionView();
        surfaceView.getHolder().addCallback(this);
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... voids) {
                File dir = new File(Environment.getExternalStorageDirectory(), "face");
                copyAssetsFile("haarcascade_frontalface_alt.xml", dir);
                File file = new File(dir, "haarcascade_frontalface_alt.xml");
                faceDetetionView.loadModel(file.getAbsolutePath());
                return null;
            }

            @Override
            protected void onPreExecute() {
                showLoading();
            }

            @Override
            protected void onPostExecute(Void aVoid) {
                dismissLoading();
            }
        }.execute();
    }

    private void copyAssetsFile(String s, File dir) {
        if(!dir.exists()){
            dir.mkdirs();
        }
        File file = new File(dir,s);
        if(!file.exists()){
            try {
                InputStream is = getAssets().open(s);
                FileOutputStream fos = new FileOutputStream(file);
                byte[] buffer = new byte[1024];
                int len;
                while ((len = is.read(buffer)) != -1){
                    fos.write(buffer,0,len);
                }
                fos.flush();
                fos.close();
                is.close();
            }catch (IOException e){
                e.printStackTrace();
            }
        }
    }
    public void from_album(View view){
        Intent intent;
        if(Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT){
            intent = new Intent();
            intent.setAction(Intent.ACTION_GET_CONTENT);
        }else {
            intent = new Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
        }
        intent.setType("image/*");
        //使用选取器并自定义标题
        startActivityForResult(Intent.createChooser(intent,"选择待识别图片"),100);
    }
    public static Bitmap toBitmap(String pathName){
        if(TextUtils.isEmpty(pathName)){
            return null;
        }
        BitmapFactory.Options o = new BitmapFactory.Options();
        o.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(pathName,o);
        int width_tmp = o.outWidth, height_tmp = o.outHeight;
        int scale = 1;
        while(true){
            if(width_tmp <= 640 && height_tmp <=480){
                break;
            }
            width_tmp /= 2;
            height_tmp /= 2;
            scale *= 2;
        }
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inSampleSize = scale;
        opts.outHeight = height_tmp;
        opts.outWidth = width_tmp;
        return BitmapFactory.decodeFile(pathName,opts);
    }
    private void getResult(Uri uri) {
        safeRecycled();
        String imagePath = null;
        if (null != uri) {
            //在我们的魅族测试手机上发现有一个相册管家 从这里选取图片会得到类似
            //file:///storage/emulated/0/tencent/MicroMsg/WeiXin/mmexport1474966179606.jpg的uri
            if ("file".equals(uri.getScheme())) {
                Log.i(TAG, "path uri 获得图片");
                imagePath = uri.getPath();
            } else if ("content".equals(uri.getScheme())) {
                Log.i(TAG, "content uri 获得图片");
                String[] filePathColumns = {MediaStore.Images.Media.DATA};
                Cursor c = getContentResolver().query(uri, filePathColumns, null, null, null);
                if (null != c) {
                    if (c.moveToFirst()) {
                        int columnIndex = c.getColumnIndex(filePathColumns[0]);
                        imagePath = c.getString(columnIndex);
                    }
                    c.close();
                }
            }
        }
        if (!TextUtils.isEmpty(imagePath)) {
            bm = toBitmap(imagePath);
            safeProcess();
        }
    }


    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 100 && null != data) {
            getResult(data.getData());
        }
    }

    public void safeProcess() {
        if (null != bm && !bm.isRecycled()) {
            faceDetetionView.process(bm);
        }
    }

    public void safeRecycled() {
        if (null != bm && !bm.isRecycled()) {
            bm.recycle();
        }
        bm = null;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        faceDetetionView.setSurface(holder.getSurface(), 640, 480);
        safeProcess();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        faceDetetionView.destroy();
        safeRecycled();
    }
}
