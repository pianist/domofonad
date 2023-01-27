#ifndef __platform_sdk_h__
#define __platform_sdk_h__



#if (defined PLATFORM_HISI_v1)
#include <platform/hi_v1/sdk.h>
#elif (defined PLATFORM_HISI_v2)
# error "hisi v2 is not ready, sorry"
#else
# error "define available PLATFORM"
#endif



#endif // __platform_sdk_h__

