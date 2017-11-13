//
// Created by 张俊杰 on 17/11/4.
//

#ifndef COMMONCUSTOMVIEW_FFMPEGAUDIO_H
#define COMMONCUSTOMVIEW_FFMPEGAUDIO_H

#include <queue>
#include <SLES/OpenSLES_Android.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <pthread.h>
#include <libswresample/swresample.h>
#include "Log.h"
class FFmpegAudio{
    //成员变量
public:
    FFmpegAudio();

    ~FFmpegAudio();
    int put( AVPacket *packet);

    int get(AVPacket *packet);
    void play();

    void stop();

    void setAvCodecContext(AVCodecContext *codec);


    int createPlayer();
public:
    //是否正在播放
    int isPlay;
    //流索引
    int index;
    // 音频解码队列
    std::queue<AVPacket*> queue;
    // 音频播放线程
    pthread_t  p_playid;
    //同步锁
    pthread_mutex_t mutex;
    //锁条件变量
    pthread_cond_t  cond;
    //新增
    SwrContext *swrContext;
    uint8_t  *out_buffer;

    int out_channer_nb;
    //相对于第一针时间
    double clock;


    AVCodecContext *codec;

    AVRational time_base;


    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverbItf;
    SLObjectItf outputMixObject;
    SLObjectItf bgPlayerObject;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerQueue;

};
};
#endif //COMMONCUSTOMVIEW_FFMPEGAUDIO_H