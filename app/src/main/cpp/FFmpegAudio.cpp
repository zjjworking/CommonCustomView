//
// Created by 张俊杰 on 17/11/5.
//

#include "FFmpegAudio.h"
FFmpegAudio::FFmpegAudio() {
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
}
int createFFmpeg(FFmpegAudio *audio){
//    mp3  里面所包含的编码格式   转换成  pcm
    audio->swrContext = swr_alloc();
    int length=0;
    int got_frame;
    // 44100 ＊2
    audio->out_buffer = (uint8_t *) av_mallocz(44100 * 2);
    uint64_t  out_ch_layout= AV_CH_LAYOUT_STEREO;
    //    输出采样位数  16位
    enum AVSampleFormat out_formart=AV_SAMPLE_FMT_S16;
    //输出的采样率必须与输入相同
    int out_sample_rate = audio->codec->sample_rate;

    swr_alloc_set_opts(audio->swrContext, out_ch_layout, out_formart, out_sample_rate,
                       audio->codec->channel_layout, audio->codec->sample_fmt,
                       audio->codec->sample_rate, 0,
                       NULL);

    swr_init(audio->swrContext);
    //    获取通道数  2
    audio->out_channer_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    LOGE("------>通道数%d  ", audio->out_channer_nb);
    return 0;
}
int getPcm(FFmpegAudio *audio){
    int frameCount = 0;
    int got_frame = 0;
    int size = 0;
    AVPacket *packet = (AVPacket *)av_mallocz(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    LOGE("解码前");

    while(audio->isPlay){
        LOGE("解码判断");
        size = 0;
        audio->get(packet);
        LOGE("3");
        if(packet->pts != AV_NOPTS_VALUE){
            audio->clock = av_q2d(audio->time_base) * packet->pts;
        }
        LOGE("2");
        LOGE("   codecContext  %p   frame  %p  packet  %p", audio->codec, frame,
             packet);
        LOGE("decoded_frame %d",frame);
        LOGE("pCodecCtx %d",audio->codec);
        LOGE("packet size %d",packet->size);
        //之前这里一值报错的原因是因为packet实例化异常导致的 get和put的时候实例化
        avcodec_decode_audio4(audio->codec,frame,&got_frame,packet);

        LOGE("解码开始");
        if(got_frame){
            LOGE("解码");
            /**
             * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                            const uint8_t **in , int in_count);
             */
            swr_convert(audio->swrContext, &audio->out_buffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
//                缓冲区的大小
            size = av_samples_get_buffer_size(NULL,audio->out_channer_nb,frame->nb_samples,
                                                  AV_SAMPLE_FMT_S16,1);
            break;

        }

    }
    av_free(packet);
    av_frame_free(&frame);
    return size;
}

/**
 * 第一次主动调用在调用线程
 * 之后在新线程中回调
 * @param bq
 * @param context
 */
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context){
    FFmpegAudio *audio = (FFmpegAudio *)context;
    LOGE("bqPlayerCallback");
    int datalen = getPcm(audio);
    if(datalen > 0){
        double time = datalen/((double)44100 * 2 * 2);
        LOGE("数据长队%d 分母%d 值%f 通道数%d",datalen,44100 * 2 * 2,time,audio->out_channer_nb);
        audio->clock = audio->clock + time;
        LOGE("当前一帧声音时间%f   播放时间%f",time,audio->clock);
        (*bq)->Enqueue(bq,audio->out_buffer,datalen);
        LOGE("播放 %d",audio->queue.size());
    } else{
        LOGE("解码错误");
    }
}
void FFmpegAudio::setAvCodecContext(AVCodecContext *codec)  {
    this->codec = codec;
    createFFmpeg(this);
}

void *play_audio(void *arg){
    FFmpegAudio *audio = (FFmpegAudio *) arg;
    audio->createPlayer();
    pthread_exit(0);
}

void FFmpegAudio::play() {
    isPlay = 1;
    pthread_create(&p_playid,NULL,play_audio,this);
}
/**
 * 消费者线程
 * @param packet
 * @return
 */
int FFmpegAudio::get(AVPacket *packet) {
    pthread_mutex_lock(&mutex);
    while (isPlay){
        if(!queue.empty()){
            //从队列中取出一个packet clone一个 给入参对象
            //将av_ref_packet改为av_copy_packet就不会报错了
            if(av_copy_packet(packet,queue.front())){
                break;
            }
            //取成功了 弹出队列 销毁packet
            AVPacket *pkt = queue.front();
            queue.pop();
            LOGE("取出一针音频数据")
            av_free(pkt);
            break;
        } else{
            pthread_cond_wait(&cond,&mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}
/**
 * 生产者
 * @param packet
 * @return
 */
int FFmpegAudio::put(AVPacket *packet){
    AVPacket *packet1 = (AVPacket *)av_mallocz(sizeof(AVPacket));
    //将av_ref_packet改为av_copy_packet就不会报错了 音视频都要修改
    if(av_copy_packet(packet1,packet)){
        return 0;
    }
    LOGE("压入一针音频数据");
    pthread_mutex_lock(&mutex);
    queue.push(packet1);
    //通知消费者 消费针  不在阻塞
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}
FFmpegAudio::~FFmpegAudio() {
    if (out_buffer) {
        free(out_buffer);
    }
    for (int i = 0; i < queue.size(); ++i) {
        AVPacket *pkt = queue.front();
        queue.pop();
        LOGE("销毁音频帧%d",queue.size());
        av_free(pkt);
    }
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}
void FFmpegAudio::stop() {
    LOGE("声音暂停");
    //因为可能卡在 deQueue
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(p_playid, 0);
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    if (bgPlayerObject) {
        (*bgPlayerObject)->Destroy(bgPlayerObject);
        bgPlayerObject = 0;

        bqPlayerQueue = 0;
        bqPlayerVolume = 0;
    }

    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }

    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineEngine = 0;
    }
    if (swrContext)
        swr_free(&swrContext);
    if (this->codec) {
        if (avcodec_is_open(this->codec))
            avcodec_close(this->codec);
        avcodec_free_context(&this->codec);
        this->codec = 0;
    }
    LOGE("AUDIO clear");
}

int FFmpegAudio::createPlayer() {
    SLresult sLresult;
    // 创建引擎engineObject
    sLresult = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != sLresult) {
        return 0;
    }
    // 实现引擎engineObject
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    // 获取引擎接口engineEngine
    sLresult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    LOGE("引擎地址%p     sLresult  %d ", engineEngine, sLresult);
    // 创建混音器outputMixObject
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
    // 实现混音器outputMixObject
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    //设置环境混音
    sLresult = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                &outputMixEnvironmentalReverbItf);
    LOGE(" sLresult 1 %d", sLresult);
    //混音器设置完毕
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverbItf, &settings);
    }
//    //初始化FFmpeg;
//    int rate;
//    int channers;
//    createFFmpeg(&rate, &channers);
//    LOGE("初始化FFmpeg完毕");

    SLDataLocator_AndroidBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    /**
    typedef struct SLDataFormat_PCM_ {
    SLuint32 		formatType;  pcm
    SLuint32 		numChannels;  通道数
    SLuint32 		samplesPerSec;  采样率
    SLuint32 		bitsPerSample;  采样位数
    SLuint32 		containerSize;  包含位数
    SLuint32 		channelMask;     立体声
    SLuint32		endianness;    end标志位
    } SLDataFormat_PCM;
    */
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource slDataSource = {&android_queue, &pcm};

    //设置关联混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    LOGE("引擎  %p", engineEngine);

    //混音器关联起来
    //创建一个播放器bgPlayerObject
    sLresult = (*engineEngine)->CreateAudioPlayer(engineEngine, &bgPlayerObject, &slDataSource,
                                                  &audioSnk, 2, ids, req);
    LOGE("   sLresult  %d   bgPlayerObject  %p  slDataSource  %p", sLresult, bgPlayerObject,
         slDataSource);
    //初始化播放器
    sLresult = (*bgPlayerObject)->Realize(bgPlayerObject, SL_BOOLEAN_FALSE);
    LOGE("sLresult  2 %d", sLresult);
    //创建一个播放器接口
    sLresult = (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    LOGE(" sLresult 3 %d", sLresult);
    //注册缓冲区
    sLresult = (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerQueue);
    LOGE(" sLresult 4 %d", sLresult);
    //设置缓冲回调接口 getPcm pthread 函数
    sLresult = (*bqPlayerQueue)->RegisterCallback(bqPlayerQueue, bqPlayerCallback, this);
    LOGE(" sLresult 5 %d", sLresult);
    //    获取音量接口
    (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    //    获取播放状态接口
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    //播放第一帧
    bqPlayerCallback(bqPlayerQueue, this);
    return 1;
}
//void *play_audio(void *args){
//    LOGE("开启音频线程");
//    FFmpegAudio* audio = (FFmpegAudio *) args;
//    AVFrame* frame = av_frame_alloc();
//    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
//    //mp3 里面所包含的编程格式  转换成 pcm
//    SwrContext *swrContext = swr_alloc();
//    int length = 0;
//    // 输出采样位数
//    // 输出的采样率必须与输入相同
//    swr_alloc_set_opts(0,AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,audio->codecContext->sample_rate,
//                    audio->codecContext->channel_layout,audio->codecContext->sample_fmt,
//                    audio->codecContext->sample_rate,0,0);
//    swr_init(swrContext);
//    uint8_t  *out_buffer = (uint8_t *)av_mallocz(44100 * 2);
//    int channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
//    int got_Frame = 0;
//    while (audio->isPlay){
//        audio->get(packet);
//        avcodec_decode_audio4(audio->codecContext,frame,&got_Frame,packet);
//        if(got_Frame){
//            LOGE("解码音频 &％d",got_Frame);
//            swr_convert(swrContext,&out_buffer,44100 * 2,(const uint8_t **)frame->data,
//                        frame->nb_samples);
//            //通道数
//            int out_buffer_size = av_samples_get_buffer_size(NULL,channels,frame->nb_samples,AV_SAMPLE_FMT_S16,1);
//            usleep(1);
//            LOGE("解码音频大小 %d",out_buffer_size);
//        }
//    }
//}
