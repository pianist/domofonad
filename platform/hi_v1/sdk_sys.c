#include <platform/hi_v1/sdk.h>

#include <stdint.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "hi_comm_ao.h"
#include "mpi_ao.h"

#define SYS_ALIGN_WIDTH      64

const char* __sdk_last_call = 0;

static int platform_v1_mpi_sys_init()
{
    HI_S32 s32Ret = HI_FAILURE;

    VB_CONF_S stVbConf;
    MPP_SYS_CONF_S stSysConf;

    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    memset(&stSysConf, 0, sizeof(MPP_SYS_CONF_S));

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    __sdk_last_call = "HI_MPI_VB_SetConf()";
    s32Ret = HI_MPI_VB_SetConf(&stVbConf);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_VB_Init()";
    s32Ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_SYS_SetConf()";
    stSysConf.u32AlignWidth = SYS_ALIGN_WIDTH;

    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if (HI_SUCCESS != s32Ret) return s32Ret;

    __sdk_last_call = "HI_MPI_SYS_Init()";
    s32Ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32Ret) return s32Ret;
}

int sdk_init()
{
    HI_S32 s32Ret = HI_FAILURE;
    s32Ret = platform_v1_mpi_sys_init();
}

void sdk_done()
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
}

/* ugly sdk hack: start */
#define MAP_FAILED ((void *)-1)

void *mmap64(void *start, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *__addr, size_t __len);

void *mmap(void *start, size_t len, int prot, int flags, int fd, uint32_t off) {
    return mmap64(start, len, prot, flags, fd, off);
}
/* ugly sdk hack: end */

