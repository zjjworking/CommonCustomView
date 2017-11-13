//
// Created by 张俊杰 on 17/11/2.
//

#ifndef COMMONCUSTOMVIEW_FFMPEGMUSIC_H
#define COMMONCUSTOMVIEW_FFMPEGMUSIC_H

#endif //COMMONCUSTOMVIEW_FFMPEGMUSIC_H

#include <jni.h>
#include <string>
#include <android/log.h>

extern "C"{
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"
#include <android/native_window_jni.h>
#include <unistd.h>

}
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"ZJJ",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ZJJ",FORMAT,##__VA_ARGS__);

int createFFmpeg(int *rate,int *channel);

int getPcm(void **pcm,size_t *pcm_size);

void realseFFmpeg();