//
// Created by 张俊杰 on 17/11/4.
//

#ifndef COMMONCUSTOMVIEW_LOG_H
#define COMMONCUSTOMVIEW_LOG_H

#include <android/log.h>

#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"zjj",FORMAT,##__VA_ARGS__);
#endif //COMMONCUSTOMVIEW_LOG_H
