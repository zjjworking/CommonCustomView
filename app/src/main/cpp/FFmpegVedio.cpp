//
// Created by 张俊杰 on 17/11/5.
//
#include <unistd.h>
#include "FFmpegVedio.h"
FFmpegVedio::FFmpegVedio() {
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
}
static void (*video_call)(AVFrame *frame);

void FFmpegVedio::setAvCodecContext(AVCodecContext *codec) {this->codec = codec;}


void FFmpegVedio::stop() {
    LOGE("VIDEO stop");
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    //因为可能卡在 deQueue
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_join(p_id, 0);
    LOGE("VIDEO join pass");
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    LOGE("VIDEO close");
}
void *play_vedio(void *arg){

    LOGE("开启视频线程");
    FFmpegVedio *vedio = (FFmpegVedio *) arg;
    //像素数据（解码数据）
    AVFrame *frame = av_frame_alloc();

    //转换rgba
    SwsContext *sws_ctx = sws_getContext(
            vedio->codec->width, vedio->codec->height,vedio->codec->pix_fmt,
            vedio->codec->width,vedio->codec->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);

    AVFrame *rgb_frame = av_frame_alloc();
    int out_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA,vedio->codec->width,
                                            vedio->codec->height, 1);
    LOGE("上下文宽%d  高%d",vedio->codec->width, vedio->codec->height);
    uint8_t *out_buffer = (uint8_t *) malloc(sizeof(uint8_t) * out_size);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, out_buffer,
                         AV_PIX_FMT_RGBA,
                         vedio->codec->width,vedio->codec->height, 1);
    int len ,got_frame, framecount = 0;
    LOGE("宽  %d ,高  %d ",vedio->codec->width,vedio->codec->height);
    //编码数据
    AVPacket *packet = (AVPacket *)av_mallocz(sizeof(AVPacket));
    //6.一阵一阵读取压缩的视频数据AVPacket
    double  last_play  //上一帧的播放时间
    ,play             //当前帧的播放时间
    , last_delay    // 上一次播放视频的两帧视频间隔时间
    ,delay         //两帧视频间隔时间
    ,audio_clock //音频轨道 实际播放时间
    ,diff   //音频帧与视频帧相差时间
    ,sync_threshold
    ,start_time  //从第一帧开始的绝对时间
    ,pts
    ,actual_delay//真正需要延迟时间
    ;//两帧间隔合理间隔时间
    start_time = av_gettime() / 1000000.0;
    while (vedio->isPlay) {
        LOGE("视频 解码  一帧 %d",vedio->queue.size());
//        消费者取到一帧数据  没有 阻塞
        vedio->get(packet);
        len = avcodec_decode_video2(vedio->codec,frame, &got_frame, packet);
        if (!got_frame) {
            continue;
        }

//        转码成rgb
        int code=sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0,
                           vedio->codec->height,
                           rgb_frame->data, rgb_frame->linesize);
        LOGE("输出%d ,%d",code,len);
        LOGE("宽%d  高%d  行字节 %d  状态%d ====%d",frame->width,frame->height,rgb_frame->linesize[0],got_frame,vedio->codec->height);
//        得到了rgb_frame  下节课讲绘制   frame  rgb     pcm  frame


        if ((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE) {
            pts = 0;
        }
        play = pts * av_q2d(vedio->time_base);
//        纠正时间
        play = vedio->synchronize(frame, play);
        //这一次播放时间 －－上一次播放时间
        delay = play - last_play;
        if (delay <= 0 || delay > 1) {
            delay = last_delay;
        }
        audio_clock = vedio->audio->clock;
        last_delay = delay;
        last_play = play;
//音频与视频的时间差
        diff = vedio->clock - audio_clock;
//        在合理范围外  才会延迟   之内加快
        sync_threshold = (delay > 0.01 ? 0.01 : delay);
        //当音频数据大于10s则便是没有数据
        if (fabs(diff) < 10) {
            if (diff <= -sync_threshold) {
                //视频慢了  视频需要加快
                delay = 0;
            } else if (diff >=sync_threshold) {
                //视频快了  视频需要减慢
                delay = 2 * delay;
            }
        }
        start_time += delay;
        actual_delay=start_time - av_gettime()/1000000.0;
        if (actual_delay < 0.01) {
            actual_delay = 0.01;
        }
        av_usleep(actual_delay*1000000.0+6000);
        video_call(rgb_frame);
//        av_packet_unref(packet);
//        av_frame_unref(rgb_frame);
//        av_frame_unref(frame);
    }
    LOGE("free packet");
    av_free(packet);
    LOGE("free packet ok");
    LOGE("free packet");
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    sws_freeContext(sws_ctx);
    size_t size = vedio->queue.size();
    for (int i = 0; i < size; ++i) {
        AVPacket *pkt = vedio->queue.front();
        av_free(pkt);
        vedio->queue.pop();
    }
    LOGE("VIDEO EXIT");
    pthread_exit(0);
}
void FFmpegVedio::play() {
    isPlay = 1;
    pthread_create(&p_id,0,play_vedio,this);
}
int FFmpegVedio::put(AVPacket *avPacket) {
    AVPacket *packet1 = (AVPacket *)av_mallocz(sizeof(AVPacket));
    if(av_copy_packet(packet1,avPacket)){
        LOGE("克隆失败");
        return 0;
    }
    LOGE("压入一帧视频数据");
    pthread_mutex_lock(&mutex);
    queue.push(packet1);
    //通知消费者  消费帧 不在阻塞
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}
int FFmpegVedio::get(AVPacket *avPacket) {
    pthread_mutex_lock(&mutex);
    while (isPlay){
        LOGE("视频取一帧");
        if(!queue.empty()){
            if(av_copy_packet(avPacket,queue.front()) < 0){
                break;
            }
            //不明白
            AVPacket *pkt = queue.front();
            queue.pop();
            //  av_packet_unref(pkt);
            //  av_free_packet(pkt);
            av_free(pkt);
            break;
        } else{
            pthread_cond_wait(&cond,&mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

FFmpegVedio::~FFmpegVedio() {
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

void FFmpegVedio::setPlayCall(void (*call)(AVFrame *)) {
    video_call = call;
}

double FFmpegVedio::synchronize(AVFrame *frame, double play) {
    //clock是当前播放的时间位置
    if(play != 0){
        clock = play;
    } else{
        //pst 为0 则先把pts设为上一针时间
        play = clock;
    }
    //可能有pts为0 则主动增加clock
    //frame->repeat_pict = 当解码时，这张图片需要要延迟多少
    //需要求出扩展延时：
    //extra_delay = repeat_pict / (2*fps) 显示这样图片需要延迟这么久来显示
    double  repeat_pict = frame->repeat_pict;
    //使用AvcodecContext的而不是stream的
    double frame_delay = av_q2d(codec->time_base);
    // 如果time_base是1：25 ，把1s分成25份，则fps为25
    // fps = 1(1/25)
    double  fps = 1/frame_delay;
    //pts 加上 这个延迟 是显示时间
    double extra_delay = repeat_pict / (2 * fps);
    double  delay = extra_delay + frame_delay;
    LOGE("extra_delay:%f",extra_delay);
    clock += delay;
    //使用
    return play;
}

void FFmpegVedio::setAudio(FFmpegAudio *audio) {
    this->audio = audio;
}


