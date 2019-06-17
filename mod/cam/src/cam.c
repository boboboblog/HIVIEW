#include <unistd.h>
#include <stdlib.h>
#include "inc/gsf.h"

#include "cam.h"
#include "cfg.h"
#include "msg_func.h"

#include "mod/bsp/inc/bsp.h"
#include "sample_comm.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message "<<<<<<<<<<<<<<<< GSF_CPU_ARCH: " STR(GSF_CPU_ARCH)  " >>>>>>>>>>>>>>>>>"

GSF_LOG_GLOBAL_INIT("CAM", 8*1024);

static int req_recv(char *in, int isize, char *out, int *osize, int err)
{
    int ret = 0;
    gsf_msg_t *req = (gsf_msg_t*)in;
    gsf_msg_t *rsp = (gsf_msg_t*)out;

    *rsp  = *req;
    rsp->err  = 0;
    rsp->size = 0;

    ret = msg_func_proc(req, isize, rsp, osize);

    rsp->err = (ret == TRUE)?rsp->err:GSF_ERR_MSG;
    *osize = sizeof(gsf_msg_t)+rsp->size;

    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
      printf("pls input: %s cam_parm.json\n", argv[0]);
      return -1;
    }
    
    if(jscon_parm_load(argv[1], &cam_parm) < 0)
    {
      jscon_parm_save(argv[1], &cam_parm);
      jscon_parm_load(argv[1], &cam_parm);
    }
    info("parm.sp[0].level:%d\n", cam_parm.sp[0].level);
    
    GSF_LOG_CONN(1, 100);
    void* rep = nm_rep_listen(GSF_IPC_CAM, NM_REP_MAX_WORKERS, NM_REP_OSIZE_MAX, req_recv);

    #if (GSF_CPU_ARCH == 35161)
    SAMPLE_MPP_CONFIG_S cfg = {.sensor_type = SONY_IMX185_MIPI_1080P_30FPS};
    SAMPLE_COMM_MPP_Cfg(&cfg);
    SAMPLE_COMM_ISP_Init(WDR_MODE_NONE);
    #elif (GSF_CPU_ARCH == 3519)
    SAMPLE_MPP_CONFIG_S cfg = {.sensor_type = SONY_IMX290_MIPI_1080P_30FPS};
    SAMPLE_COMM_MPP_Cfg(&cfg);
    SAMPLE_COMM_ISP_Init(0, WDR_MODE_NONE, SAMPLE_FRAMERATE_30FPS, SAMPLE_SENSOR_SINGLE);
    #else
    SAMPLE_MPP_CONFIG_S cfg = {.sensor_type = SONY_IMX185_MIPI_1080P_30FPS};
    SAMPLE_COMM_MPP_Cfg(&cfg);
    SAMPLE_COMM_ISP_Init(WDR_MODE_NONE);
    #endif

    while(1)
    {
      //register To;
      GSF_MSG_DEF(gsf_mod_reg_t, reg, 8*1024);
      reg->mid = GSF_MOD_ID_CAM;
      strcpy(reg->uri, GSF_IPC_CAM);
      int ret = GSF_MSG_SENDTO(GSF_ID_MOD_CLI, 0, SET, GSF_CLI_REGISTER, sizeof(gsf_mod_reg_t), GSF_IPC_BSP, 2000);
      
      printf("GSF_CLI_REGISTER To:%s, ret:%d, err:%d, size:%d\n", GSF_IPC_BSP, ret, __pmsg->err, __rsize);
      if(ret == 0)
      {
        int i = 0;
        gsf_mod_reg_t *mod = (gsf_mod_reg_t*)__pmsg->buf;
        for(i = 0; i < GSF_MOD_ID_END; i++)
        {
          if(strlen(mod->uri))
            printf("[ mid:%d, uri:%s ]\n", mod->mid, mod->uri);
          mod++;
        }
        break;
      }

      sleep(1);
    }
    
    {
      GSF_MSG_DEF(gsf_msg_t, msg, 4*1024);
      int ret = GSF_MSG_SENDTO(GSF_ID_BSP_DEF, 0, GET, 0, 0, GSF_IPC_BSP, 2000);
      gsf_bsp_def_t *cfg = (gsf_bsp_def_t*)__pmsg->buf;
      printf("GET GSF_ID_BSP_DEF To:%s, ret:%d, err:%d, model:%s\n", GSF_IPC_BSP, ret, __pmsg->err, cfg->board.model);
    }
    
    while(1)
    {
      /*
      struct timespec _ts;
      clock_gettime(CLOCK_MONOTONIC, &_ts);
      printf("test log speed %d ms.\n", _ts.tv_sec*1000 + _ts.tv_nsec/1000000);
      usleep(10*1000);
      */
      sleep(1); 
    }
      
    GSF_LOG_DISCONN();
    return 0;
}