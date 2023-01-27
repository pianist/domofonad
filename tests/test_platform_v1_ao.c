#include <platform/sdk.h>
#include <errno.h>
#include <string.h>

#include <signal.h>

int stop_flag = 0;

void action_on_sognal(int signum)
{
    printf("STOP\n");
    stop_flag = 1;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "./test_platform_v1_ao file_to_play.pcm\n");
        return -1;

    }

    signal(SIGINT, action_on_sognal);

    int ret = sdk_init();
    if (ret < 0)
    {
        fprintf(stderr, "sdk_init() failed at %s: 0x%X\n", __sdk_last_call, ret);
        return -1;
    }

    ret = sdk_audio_init(0, PT_LPCM, 48000);
    if (ret < 0)
    {
        fprintf(stderr, "init_sdk() failed at %s: 0x%X\n", __sdk_last_call, ret);
        return -1;
    }

    FILE* f = fopen(argv[1], "r");
    if (f)
    {
        printf("Now play %s!\n", argv[1]);
        sdk_audio_play(f, &stop_flag);
        fclose(f);
    }
    else
    {
        printf("ERROR %d\n", errno);
        printf("Error %d: %s\n", errno, strerror(errno));
    }

    sdk_audio_done();

    sdk_done();

    return 0;
}




