//
// Created by Administrator on 2016/11/25.
//

#ifndef LOGIC_JNILOG_H
#define LOGIC_JNILOG_H

#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_INFO, "LOGICLIB", __VA_ARGS__)


#endif //LOGIC_JNILOG_H
