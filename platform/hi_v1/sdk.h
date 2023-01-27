#ifdef __PLATFORM_SDK__
# error "Only one platform sdk allowed"
#endif

#ifndef __platform_hi_v1_sdk_h__
#define __platform_hi_v1_sdk_h__

#include <stdio.h>
#include <string.h>
#include "hi_common.h"



extern const char* __sdk_last_call;

int sdk_init();
void sdk_done();

int sdk_audio_init(int bMic, PAYLOAD_TYPE_E payload_type, int audio_rate);
void sdk_audio_play(FILE* f, int* stop_flag);
void sdk_audio_done();








#endif // __platform_hi_v1_sdk_h__

