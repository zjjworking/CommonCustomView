package com.zjj.commoncustomview;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;

import java.util.Objects;

/**
 * Created by zjj on 17/12/7.
 */

public class TestServer extends Service {

    @Override
    public void onCreate() {
        super.onCreate();
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return new Binder(){
            @Override
            protected boolean onTransact(int code, Parcel data, Parcel reply, int flags) throws RemoteException {
                if(code ==  FIRST_CALL_TRANSACTION){
                    int i = data.readInt();
                    reply.writeNoException();
                    reply.writeInt(2 * i);
                    return true;
                }
                    return super.onTransact(code,data,reply,flags);
            }
        };
    }

    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }

    @Override
    public void onStart(Intent intent, int startId) {
        super.onStart(intent, startId);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
