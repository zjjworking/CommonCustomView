//
// Created by 张俊杰 on 17/11/5.
//

#ifndef COMMONCUSTOMVIEW_FFMPEGVEDIO_H
#define COMMONCUSTOMVIEW_FFMPEGVEDIO_H

#include <queue>
#include "FFmpegAudio.h"

extern  "C"{
#include <unistd.h>
#include <pthread.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include "Log.h"

class FFmpegVedio{
public:
    FFmpegVedio();

    ~FFmpegVedio();

    int put(AVPacket *avPacket);

    int get(AVPacket *avPacket);

    void play();

    void setAvCodecContext(AVCodecContext *codec);


    void stop();
    /**
     * 设置回调接口
     * @param call
    */
    void setPlayCall(void (*call)(AVFrame* frame));

    double synchronize(AVFrame *frame, double play);

    void setAudio(FFmpegAudio *audio);

public:
    double  clock;
    //    解码器上下文
    AVCodecContext *codec;
    //    处理线程
    pthread_t p_id;
    //    是否正在播放
    int isPlay;
    //    流索引
    int index;
    //    同步锁
    pthread_mutex_t mutex;
    //    条件变量
    pthread_cond_t cond;
    // 视频解码队列
    std::queue<AVPacket*> queue;

    AVRational time_base;

    FFmpegAudio* audio;
};

};

#endif //COMMONCUSTOMVIEW_FFMPEGVEDIO_H
