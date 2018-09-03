#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>
extern "C"{
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
//像素处理
#include "libswscale/swscale.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <unistd.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
}
#include "FFmpegMusic.h"
#include "FFmpegAudio.h"
#include "FFmpegVedio.h"
#include "ImageProcess.h"

#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"zjj",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"zjj",FORMAT,##__VA_ARGS__);

//////////////////////////////////////////车牌识别///////////////////////////////////////////////////
extern "C" {

#include "common.h"

CarPlateRecgize *carPlateRecgize = 0;

void bitmap2_Mat(JNIEnv *env,jobject bitmap,Mat &dst){
    AndroidBitmapInfo info;
    void *pixels = 0;
    //获得bitmap信息
    CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
    //必须是 rgba8888 rgb565
    CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);
    //lock 获得数据
    CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
    CV_Assert(pixels);
    dst.create(info.height, info.width, CV_8UC3);
    LOGE("bitmap2Mat: RGBA_8888 bitmap -> Mat");
    Mat tmp(info.height, info.width, CV_8UC4, pixels);
    cvtColor(tmp, dst, COLOR_RGBA2BGR);
    // cvtColor(dst, dst, COLOR_RGBA2RGB);
    tmp.release();
    AndroidBitmap_unlockPixels(env, bitmap);
}
void mat2Bitmap(JNIEnv *env,Mat &src,jobject bitmap){
    AndroidBitmapInfo info;
    void *pixels = 0;
    LOGE("nMatToBitmap");
    CV_Assert(AndroidBitmap_getInfo(env,bitmap,&info) >= 0);
    CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);
    CV_Assert(src.dims == 2 && info.height == (uint32_t) src.rows &&
              info.width == (uint32_t) src.cols);
    CV_Assert(src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4);
    CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
    CV_Assert(pixels);
    Mat tmp(info.height, info.width, CV_8UC4, pixels);
    if (src.type() == CV_8UC1) {
        LOGI("nMatToBitmap: CV_8UC1 -> RGBA_8888");
        cvtColor(src, tmp, COLOR_GRAY2RGBA);
    } else if (src.type() == CV_8UC3) {
        LOGI("nMatToBitmap: CV_8UC3 -> RGBA_8888");
        cvtColor(src, tmp, COLOR_BGR2RGBA);
    } else if (src.type() == CV_8UC4) {
        LOGI("nMatToBitmap: CV_8UC4 -> RGBA_8888");
        src.copyTo(tmp);
    }
    AndroidBitmap_unlockPixels(env, bitmap);
    return;
}

JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_Carplate_init(JNIEnv *env, jobject instance, jstring svm_,
                                            jstring ann_, jstring ann_zh_) {
    const char *svm = env->GetStringUTFChars(svm_, 0);
    const char *ann = env->GetStringUTFChars(ann_, 0);
    const char *ann_zh = env->GetStringUTFChars(ann_zh_, 0);

    carPlateRecgize = new CarPlateRecgize(svm,ann,ann_zh);

    env->ReleaseStringUTFChars(svm_, svm);
    env->ReleaseStringUTFChars(ann_, ann);
    env->ReleaseStringUTFChars(ann_zh_, ann_zh);
}

JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_Carplate_release(JNIEnv *env, jobject instance) {

    if(carPlateRecgize){
        delete carPlateRecgize;
    }

}

JNIEXPORT jstring JNICALL
Java_com_zjj_commoncustomview_Carplate_recognition(JNIEnv *env, jobject instance, jobject bitmap,
                                                   jobject out) {

    Mat src;
    bitmap2_Mat(env,bitmap,src);
    Mat plate;
    //找到车牌
    string str= carPlateRecgize->(src,plate);
    mat2Bitmap(env,plate,out);
    src.release();
    return env->NewStringUTF(str.c_str());
}
}


//////////////////////////////////////////身份证号码识别/////////////////////////////////////////////




extern "C" {


JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_ImageProcess_findIdNumber(JNIEnv *env, jclass type, jobject src,
                                                        jobject out, jobject config) {

}

JNIEXPORT jobject JNICALL
Java_com_zjj_commoncustomview_ImageProcess_getIdNumber(JNIEnv *env, jclass type, jobject src,
                                                       jobject tpl, jobject config) {

    ImageProcess *audio = new ImageProcess;
    return audio->getIdNumber(env,type,src,tpl,config);

}

}


//////////////////////////////////////人脸识别openCV/////////////////////////////////////////////


extern "C" {
using namespace std;
using namespace cv;

void bitmap2Mat(JNIEnv *env,jobject bitmap,Mat &dst);

CascadeClassifier *faceClassifier;

ANativeWindow *nativeWindow;


/**
 * 加载二进制流
 */
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_FaceDetetionView_loadModel(JNIEnv *env, jobject instance,
                                                         jstring detectModel_) {
    const char *detectModel = env->GetStringUTFChars(detectModel_, 0);
    //获取一个Classifier model
    faceClassifier = new CascadeClassifier(detectModel);

    env->ReleaseStringUTFChars(detectModel_, detectModel);
}

JNIEXPORT jboolean JNICALL
Java_com_zjj_commoncustomview_FaceDetetionView_process(JNIEnv *env, jobject instance,
                                                       jobject bitmap) {
    int ret = 1;
    Mat src;
    bitmap2Mat(env,bitmap,src);
    imwrite("/sdcar/d.png",src);
    if(faceClassifier){
        vector<Rect> faces;
        Mat grayMat;
        //图片灰度化
        cvtColor(src,grayMat,CV_BGR2GRAY);
        imwrite("/sdcard/huidu.png",grayMat);
        //直方图均衡化 增强对比效果
        equalizeHist(grayMat,grayMat);
        imwrite("/sdcar/e.png",grayMat);
        //识别，并将识别的头部区域写入faces向量中
        faceClassifier->detectMultiScale(grayMat,faces);
        grayMat.release();
        for (int i = 0; i < faces.size(); ++i) {
            Rect face = faces[i];
            //Scalar(0,255,255) 参数含义，矩阵的颜色
            rectangle(src,face.tl(),face.br(),Scalar(0,255,255));
        }
    }
    if(!nativeWindow){
        LOGE("native window null");
        ret = 0;
        goto end;
    }
    ANativeWindow_Buffer window_buffer;
    if(ANativeWindow_lock(nativeWindow,&window_buffer,0)){
        LOGE("native window lock fail");
        ret = 0;
        goto end;
    }
    imwrite("/sdcard/c.png",src);
    //直接画 不传到java，使用原生的nativeWindow直接画
    cvtColor(src,src,CV_BGR2RGBA);
    imwrite("sdcard/b.png",src);
    resize(src,src,Size(window_buffer.width,window_buffer.height));
    memcpy(window_buffer.bits,src.data,window_buffer.height * window_buffer.width *4);
    ANativeWindow_unlockAndPost(nativeWindow);

    end:
    src.release();

    return ret;

}
//TODO 目的和作用?
// java的surface 创建一个window 用于显示native 的图像，显示在surface 指定区域
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_FaceDetetionView_setSurface(JNIEnv *env, jobject instance,
                                                          jobject surface, jint w, jint h) {
    if(surface && w && h){
        if(nativeWindow){
            LOGI("release old native window");
            ANativeWindow_release(nativeWindow);
            nativeWindow = 0;
        }
        LOGI("new native window");
        nativeWindow = ANativeWindow_fromSurface(env,surface);
        if(nativeWindow){
            //设置nativeWindow的分辨率和显示图像的格式
            ANativeWindow_setBuffersGeometry(nativeWindow,w,h,WINDOW_FORMAT_RGBA_8888);
        }
    } else{
        if(nativeWindow){
            LOGI("release old native window");
            ANativeWindow_release(nativeWindow);
            nativeWindow = 0;
        }
    }

}

JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_FaceDetetionView_destroy(JNIEnv *env, jobject instance) {

    if(faceClassifier) {
        delete faceClassifier;
        faceClassifier = 0;
    }
    if(nativeWindow) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = 0;
    }
}
void bitmap2Mat(JNIEnv *env,jobject bitmap,Mat &dst){

#if 0
    AndroidBitmapInfo info;
    void *pixels = 0;
    //获取bitmap信息
    CV_Assert(AndroidBitmap_getInfo(env,bitmap,&info) >= 0);
    //必须是 rgba8888 rgb565
    CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);
    //lock 获取数据
    CV_Assert(AndroidBitmap_lockPixels(env,bitmap,&pixels) >= 0);
    CV_Assert(pixels);
    dst.create(info.height,info.width,CV_8UC3);
    LOGE("bitmap2Mat: RGBA_8888 bitmap -> mat");
    Mat tmp;
    tmp = Mat(info.height,info.width,CV_8UC3,pixels);
    cvtColor(tmp,dst,COLOR_RGBA2BGR);
    tmp.release();
    AndroidBitmap_unlockPixels(env,bitmap);
#else
    AndroidBitmapInfo info;
    void*  pixels = 0;
    try {
        LOGE("nBitmapToMat");
        CV_Assert(AndroidBitmap_getInfo(env,bitmap,&info) >= 0);

        CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                  info.format == ANDROID_BITMAP_FORMAT_RGB_565);

        CV_Assert(AndroidBitmap_lockPixels(env,bitmap,&pixels) >= 0);
        CV_Assert(pixels);
        dst.create(info.height,info.width,CV_8UC4);
        if(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888){
            LOGE("nBitmapTOMat: RGBA_8888 -> CV_8UC4");
            Mat tmp(info.height,info.width,CV_8UC4,pixels);
            tmp.copyTo(dst);
            cvtColor(dst,dst,COLOR_RGBA2BGR);
            tmp.release();
        } else{
            LOGE("mBitmapToMat: RGB_565 -> CV_8UC4");
            Mat tmp(info.height,info.width,CV_8UC2,pixels);
            cvtColor(tmp,dst,COLOR_BGR5652BGR);
            tmp.release();
        }
        AndroidBitmap_unlockPixels(env,bitmap);
        return;
    }catch (const cv::Exception& e){
        AndroidBitmap_unlockPixels(env,bitmap);
        LOGE("nBitmapToMat catched cv::Exception: %s", e.what());
        jclass je = env->FindClass("org/opencv/core/CvException");
        if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    }catch (...){
        AndroidBitmap_unlockPixels(env,bitmap);
        LOGE("nBitmapToMat catched unknown exception (...)");
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }

#endif
}
}

//////////////////////////////////////////// 音视频 //////////////////////////////////////////////////
extern "C" {
ANativeWindow *window = 0;
//创建Audio 结构体
static SLObjectItf engineObject = NULL;
//音频引擎
static SLEngineItf engineEngine = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverbItf = NULL;

//混音器
static SLObjectItf outputMixObject = NULL;

JavaVM *vm = 0;


static const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
//队列缓冲区
SLAndroidSimpleBufferQueueItf bqPlayerQueue;
//播放器
static SLObjectItf bgPlayerObject = NULL;
//
static SLPlayItf bqPlayerPlay = NULL;
//音量对象
static SLVolumeItf bqPlayerVolume;
static void *buffer;
static size_t bufferSize = 0;

const char *path;

FFmpegVedio *vedio;
FFmpegAudio *audio;

pthread_t p_tid;

int isPlay = 0;

void call_video_play(AVFrame *frame){
    if (!window) {
        return;
    }
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        return;
    }
    //视频数据填充，绘制显示
    LOGE("绘制 宽%d,高%d",frame->width,frame->height);
    LOGE("绘制 宽%d,高%d  行字节 %d ",window_buffer.width,window_buffer.height, frame->linesize[0]);
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}
//解码函数
void *process(void *args){
    LOGE("开启解码线程");
    //1.注册组件
    av_register_all();
    avformat_network_init();
    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if(avformat_open_input(&pFormatCtx,path,NULL,NULL) != 0){
        LOGE("%s","打开输入视频文件失败");
    }
    //3.获取视频信息
    if(avformat_find_stream_info(pFormatCtx,NULL) < 0){
        LOGE("%s","获取视频信息失败");
    }

    //视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
    int i = 0;
    for(; i < pFormatCtx->nb_streams;i++){
        //4.获取视频解码器
        AVCodecContext *pCodeCtx = pFormatCtx->streams[i]->codec;
        AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);

        AVCodecContext *codec = avcodec_alloc_context3(pCodec);
        avcodec_copy_context(codec, pCodeCtx);
        if(avcodec_open2(codec,pCodec,NULL) < 0){
            LOGE("%s","解码器无法打开");
            continue;
        }
        //根据类型判断，是否是视频流
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            /*找到视频流*/
            vedio->setAvCodecContext(codec);
            vedio->index = i;
            vedio->time_base = pFormatCtx->streams[i]->time_base;
            if (window)
                ANativeWindow_setBuffersGeometry(window, vedio->codec->width,
                                                 vedio->codec->height,
                                                 WINDOW_FORMAT_RGBA_8888);

        } else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio->setAvCodecContext(codec);
            audio->index = i;
            audio->time_base=pFormatCtx->streams[i]->time_base;
        }

    }

//    开启 音频 视频  播放的死循环
    vedio->setAudio(audio);
    vedio->play();
    audio->play();
    isPlay = 1;
//    解码packet

    //编码数据
    AVPacket *packet = (AVPacket *)av_mallocz(sizeof(AVPacket));
//    解码完整个视频 子线程
    int ret;
    while (isPlay ) {
//        如果这个packet  流索引 等于 视频流索引 添加到视频队列
        ret = av_read_frame(pFormatCtx, packet);
        if (ret == 0) {
            if (vedio && vedio->isPlay && packet->stream_index == vedio->index) {
                vedio->put(packet);
            } else  if (audio&& audio->isPlay && packet->stream_index == audio->index) {
                audio->put(packet);
            }
            av_packet_unref(packet);
        } else if(ret == AVERROR_EOF) {
//            读完了
            //读取完毕 但是不一定播放完毕
            while (isPlay) {
                if (vedio->queue.empty() && audio->queue.empty()) {
                    break;
                }
//                LOGI("等待播放完成");
                av_usleep(10000);
            }
        }

    }
//    视频解码完     可能视频播放完了   也可能视频没播放完成







    isPlay = 0;
    if (vedio && vedio->isPlay) {
        vedio->stop();
    }
    if (audio && audio->isPlay) {
        audio->stop();
    }
    av_free_packet(packet);
    avformat_free_context(pFormatCtx);
    pthread_exit(0);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_ZJJPlayer_dispaly(JNIEnv *env, jobject instance, jobject surface) {

    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    //初始化用于播放的window
    window = ANativeWindow_fromSurface(env, surface);
    //给window设置宽高等信息
    if (vedio && vedio->codec) {
        ANativeWindow_setBuffersGeometry(window, vedio->codec->width, vedio->codec->height,
                                         WINDOW_FORMAT_RGBA_8888);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_ZJJPlayer_play(JNIEnv *env, jobject instance, jstring path_) {
    path = env->GetStringUTFChars(path_, 0);
//        实例化对象
    vedio = new FFmpegVedio;
    audio = new FFmpegAudio;
    vedio->setPlayCall(call_video_play);
    pthread_create(&p_tid,NULL,process,NULL);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_ZJJPlayer_release(JNIEnv *env, jobject instance) {


    if (isPlay) {
        isPlay = 0;
        pthread_join(p_tid, 0);
    }
    if (vedio) {
        if (vedio->isPlay) {
            vedio->stop();
        }
        delete (vedio);
        vedio = 0;
    }
    if (audio) {
        if (audio->isPlay) {
            audio->stop();
        }
        delete (audio);
        audio = 0;

    }

}


//只要喇叭一读完 就会回调此函数 添加pcm数据到缓冲区
void bqPlayerCallBack(SLAndroidSimpleBufferQueueItf bq, void *context) {
    bufferSize = 0;
    LOGE("准备")
    getPcm(&buffer, &bufferSize);
    LOGE("开始")
    if (NULL != buffer && 0 != bufferSize) {
        //播放的关键地方
        SLresult lresult = (*bqPlayerQueue)->Enqueue(bqPlayerQueue, buffer, bufferSize);
        LOGE("正在播放%d", lresult);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_ZJJPlayer_player(JNIEnv *env, jobject instance) {
    //初始化一个引擎
    SLresult sLresult;
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    //获取到引擎接口
    sLresult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    LOGE("引擎地址%p     sLresult  %d ", engineEngine, sLresult);
    //创建混音器
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);

    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    //设置环境混音
    sLresult = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                                &outputMixEnvironmentalReverbItf);
    LOGE(" sLresult 1 %d", sLresult);
    //混音器设置完毕
    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentalReverbItf)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverbItf, &settings);
    }
    //初始化FFmpeg;
    int rate;
    int channers;
    createFFmpeg(&rate, &channers);
    LOGE("初始化FFmpeg完毕");

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
                                                  &audioSnk, 3, ids, req);
    LOGE("   sLresult  %d   bgPlayerObject  %p  slDataSource  %p", sLresult, bgPlayerObject,
         &slDataSource);

    sLresult = (*bgPlayerObject)->Realize(bgPlayerObject, SL_BOOLEAN_FALSE);
    LOGE("sLresult  2 %d", sLresult);
    //创建一个播放器接口
    sLresult = (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    LOGE(" sLresult 3 %d", sLresult);
    //注册缓冲区
    sLresult = (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerQueue);
    LOGE(" sLresult 4 %d", sLresult);
    //设置回调接口 getPcm pthread 函数
    sLresult = (*bqPlayerQueue)->RegisterCallback(bqPlayerQueue, bqPlayerCallBack, NULL);
    LOGE(" sLresult 5 %d", sLresult);

    (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    //播放第一帧
    bqPlayerCallBack(bqPlayerQueue, NULL);

}
void shutDown() {

}


extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_ZJJPlayer_stop(JNIEnv *env, jobject instance) {
    if (bgPlayerObject != NULL) {
        (*bgPlayerObject)->Destroy(bgPlayerObject);
        bgPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerQueue = NULL;
        bqPlayerVolume = NULL;
    }
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverbItf = NULL;
    }
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineEngine = NULL;
        engineObject = NULL;
    }
    realseFFmpeg();
}

//音屏解压播放
extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_ZJJPlayer_sound(JNIEnv *env, jobject instance, jstring input_,
                                              jstring output_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    const char *output = env->GetStringUTFChars(output_, 0);
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //第四个参数是 可以传一个 字典   是一个入参出参对象
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGE("%s", "打开输入音频文件失败");
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取音频信息失败");
        return;
    }

    int audio_stream_idx = -1;
    int i = 0;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGE("  找到音频id %d", pFormatCtx->streams[i]->codec->codec_type);
            audio_stream_idx = i;
            break;
        }
    }
// mp3的解码器

//    获取音频编解码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    LOGE("获取视频编码器上下文 %p  ", pCodecCtx);
    //加密的用不了
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    LOGE("获取音屏编码")
    //版本升级了
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
    }
    AVPacket *packet = (AVPacket *) av_mallocz(sizeof(AVPacket));
//    av_init_packet(packet);
//    像素数据
    AVFrame *frame;
    frame = av_frame_alloc();
//    mp3  里面所包含的编码格式   转换成  pcm
    SwrContext *swrContext = swr_alloc();
    int length = 0;
    int got_frame;

    uint8_t *out_buffer = (uint8_t *) av_mallocz(44100 * 2);

    /**
 * struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
      int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
      int log_offset, void *log_ctx);
      7.1
 */

    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
//    输出采样位数
    enum AVSampleFormat out_formart = AV_SAMPLE_FMT_S16;

//输出的采样率必须与输入相同
    int out_sample_rate = pCodecCtx->sample_rate;


    swr_alloc_set_opts(swrContext, out_ch_layout, out_formart, out_sample_rate,
                       pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,
                       NULL);

    swr_init(swrContext);
    //    获取通道数  2
    int out_channer_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    //反射操作

    //反射得到Class类型
    jclass zjj_player = env->GetObjectClass(instance);
    //反射得到createAudio方法
    jmethodID createAudio = env->GetMethodID(zjj_player, "createAudio", "(II)V");
    //放射调用createAudio
    env->CallVoidMethod(instance, createAudio, 44100, out_channer_nb);
    jmethodID audio_write = env->GetMethodID(zjj_player, "playTrack", "([BI)V");



    //    输出文件
//    FILE *pcm_file = fopen(output, "wb");
    int frameCount = 0;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
//            解码  mp3   编码格式frame----pcm   frame
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if (got_frame) {
                LOGE("解码");
                /**
                 * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                                const uint8_t **in , int in_count);
                 */
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
                            frame->nb_samples);
//                通道数 size大小
                int size = av_samples_get_buffer_size(NULL, out_channer_nb, frame->nb_samples,
                                                      AV_SAMPLE_FMT_S16, 1);

                //调用java方法 传入数据
                jbyteArray audio_sample_array = env->NewByteArray(size);
                //将out_buffer里面的数据转换为jbyteArray
                env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
                env->CallVoidMethod(instance, audio_write, audio_sample_array, size);
                env->DeleteLocalRef(audio_sample_array);
//                fwrite(out_buffer,1,out_buffer,pcm_file);
            }
        }
    }
    //fclose(pcm_file);
    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);


    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);
}

//视频播放 mp4 －－》 RGB
extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_VideoView_render(JNIEnv *env, jobject instance, jstring input_,
                                               jobject surface) {
    const char *input = env->GetStringUTFChars(input_, NULL);

    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //第四个参数是 可以传一个 字典   是一个入参出参对象
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
        return;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视屏信息失败")
        return;
    }
    int vidio_stream_idx = -1;
    int i = 0;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGE("  找到视频id %d", pFormatCtx->streams[i]->codec->codec_type);
            vidio_stream_idx = i;
            break;
        }
    }
    //获取视屏编码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[vidio_stream_idx]->codec;
    LOGE("获取视频编码器上下文 %p  ", pCodecCtx);
    //加密的不用了
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    LOGE("获取视频编码 %p", pCodex);
//版本升级了
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {


    }
    AVPacket *packet = (AVPacket *) av_mallocz(sizeof(AVPacket));
    //像素数据
    AVFrame *frame;
    frame = av_frame_alloc();
    //RGB
    AVFrame *rgb_frame = av_frame_alloc();
    //    给缓冲区分配内存
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_mallocz(
            avpicture_get_size(AV_PIX_FMT_RGBA, pCodecCtx->width,
                               pCodecCtx->height));
    LOGE("宽  %d,  高  %d  ", pCodecCtx->width, pCodecCtx->height);
//设置yuvFrame的缓冲区，像素格式
    int re = avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA, pCodecCtx->width,
                            pCodecCtx->height);
    LOGE("申请内存%d   ", re);
    //    输出需要改变
    int length = 0;
    int got_frame;
    //输出文件
    int frameCount = 0;
    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL
    );
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
//    视频缓冲区
    ANativeWindow_Buffer outBuffer;
    //ANativeWindow
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == vidio_stream_idx) {
            length = avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);
            LOGE(" 获得长度   %d ", length);

            //非零   正在解码
            if (got_frame) {
                //            绘制之前   配置一些信息  比如宽高   格式
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodecCtx->width, pCodecCtx->height,
                                                 WINDOW_FORMAT_RGBA_8888);
                //绘制
                ANativeWindow_lock(nativeWindow, &outBuffer, NULL);
                // 之前h264 -----yuv  这次 RGBA
                LOGI("解码%d帧", frameCount++);
                sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                          pCodecCtx->height, rgb_frame->data,
                          rgb_frame->linesize);
                //rgb_frame是有画面数据
                uint8_t *dst = (uint8_t *) outBuffer.bits;
                //拿到一行有多少个字节 RGBA
                int destStride = outBuffer.stride * 4;
                //像素数据的首地址
                uint8_t *src = rgb_frame->data[0];
                //实际内存一行数据
                int srcStride = rgb_frame->linesize[0];

                for (int i = 0; i < pCodecCtx->height; ++i) {
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                usleep(1000 * 16);
            }
        }
        av_free_packet(packet);
    }
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pFormatCtx);
    env->ReleaseStringUTFChars(input_, input);
}
//视屏转码 mp4-－－》yuv
extern "C"
JNIEXPORT void JNICALL
Java_com_zjj_commoncustomview_MainActivity_open(
        JNIEnv *env,
        jobject obj, jstring inputStr_, jstring outStr_) {
    const char *inputStr = env->GetStringUTFChars(inputStr_, 0);
    const char *outStr = env->GetStringUTFChars(outStr_, 0);
    //注册各大组件
    av_register_all();
    AVFormatContext *pContext = avformat_alloc_context();

    if (avformat_open_input(&pContext, inputStr, NULL, NULL) < 0) {
        LOGE("打开失败");
        return;
    }
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }
    int vedio_stream_idx = -1;
    //找到视频流
    for (int i = 0; i < pContext->nb_streams; ++i) {
        LOGE("循环 %d", i);
        //codec 每一个流 对应的解码上下文 codec_type 流的类型
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vedio_stream_idx = i;
        }
    }
    //获取到解码器上下文
    AVCodecContext *pCodecCtx = pContext->streams[vedio_stream_idx]->codec;
    //解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    //ffempg版本升级
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }
    //分配内存 malloc AVPacket 1  2
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //初始化结构体
    av_init_packet(packet);
    //还不够
    AVFrame *frame = av_frame_alloc();
    //声明一个yuvframe
    AVFrame *yuvFrame = av_frame_alloc();
    //给yuvframe 的缓冲区 初始化
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
                               pCodecCtx->height));
    //初始化
    int re = avpicture_fill((AVPicture *) yuvFrame, out_buffer, AV_PIX_FMT_YUV420P,
                            pCodecCtx->width, pCodecCtx->height);

    LOGE("宽 %d 高 %d", pCodecCtx->width, pCodecCtx->height);
    // mp4的上下文pCodecCtx－>pix_fmt
    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                            SWS_BILINEAR, NULL, NULL, NULL);

    int frameCount = 0;
    FILE *fp_yuv = fopen(outStr, "wb");

    //packet入参 出参对象 转换上下文
    int got_frame;
    while (av_read_frame(pContext, packet) >= 0) {
        //        解封装

        //     根据frame 进行原生绘制    bitmap  window
        avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);
        //frame 的数据拿到 视频像素数据  yuv 三个rgb  r  g  b  数据量大  三个通道
        LOGE("解码 %d", frameCount++);
        if (got_frame > 0) {
            sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0,
                      frame->height,
                      yuvFrame->data, yuvFrame->linesize);
            int y_size = pCodecCtx->width * pCodecCtx->height;
            // y亮度信息写完了
            //y 4 y:u:v  = 4:1:1
            fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
            //u 1
            fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);
            //v 1
            fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);
        }
        av_free_packet(packet);
    }
    fclose(fp_yuv);
    av_frame_free(&frame);
    av_frame_free(&yuvFrame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pContext);
    env->ReleaseStringUTFChars(inputStr_, inputStr);
    env->ReleaseStringUTFChars(outStr_, outStr);


}
}

