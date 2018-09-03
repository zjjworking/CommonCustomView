//
// Created by 张俊杰 on 17/11/10.
//

#ifndef COMMONCUSTOMVIEW_IMAGEPROCESS_H
#define COMMONCUSTOMVIEW_IMAGEPROCESS_H
#include <jni.h>

#include <android/bitmap.h>


#include <opencv/cv.hpp>

extern  "C" {
#include "Log.h"
using namespace cv;
using namespace std;

class ImageProcess{
public:
    ImageProcess();
    ~ImageProcess();
 jobject getIdNumber(JNIEnv *env, jclass type, jobject src,
                     jobject tpl, jobject config);
};
}


#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))



#endif //COMMONCUSTOMVIEW_IMAGEPROCESS_H
