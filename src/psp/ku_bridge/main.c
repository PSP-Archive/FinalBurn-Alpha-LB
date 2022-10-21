/******************************************************************************

  ku_bridge.prx

******************************************************************************/

#include <pspsdk.h>
#include <pspimpose_driver.h>
#include <pspctrl.h>


PSP_MODULE_INFO("ku_bridge", 0x1006, 1, 1);
PSP_MAIN_THREAD_ATTR(0);


/******************************************************************************
  prototypes
******************************************************************************/

int sceCtrl_driver_C4AAD55F(SceCtrlData *pad_data, int count);
int sceCtrl_driver_454455AC(SceCtrlData *pad_data, int count);

#define sceCtrlPeekBufferPositive371  sceCtrl_driver_C4AAD55F
#define sceCtrlReadBufferPositive371  sceCtrl_driver_454455AC

/******************************************************************************
  local variables
******************************************************************************/

static int (*__sceCtrlPeekBufferPositive)(SceCtrlData *pad_data, int count);
static int (*__sceCtrlReadBufferPositive)(SceCtrlData *pad_data, int count);

/******************************************************************************
  functions
******************************************************************************/

int kuImposeGetParam(SceImposeParam param)
{
  int k1 = pspSdkSetK1(0);
  int ret = sceImposeGetParam(param);
  pspSdkSetK1(k1);
  return ret;
}

int kuImposeSetParam(SceImposeParam param, int value)
{
  int k1 = pspSdkSetK1(0);
  int ret = sceImposeSetParam(param, value);
  pspSdkSetK1(k1);
  return ret;
}


int kuCtrlPeekBufferPositive(SceCtrlData *pad_data, int count)
{
  if(__sceCtrlPeekBufferPositive)
  {
    int k1 = pspSdkSetK1(0);
    int ret = (*__sceCtrlPeekBufferPositive)(pad_data, count);
    pspSdkSetK1(k1);
    return ret;
  }

  return -1;
}

int kuCtrlReadBufferPositive(SceCtrlData *pad_data, int count)
{
  if(__sceCtrlReadBufferPositive)
  {
    int k1 = pspSdkSetK1(0);
    int ret = (*__sceCtrlReadBufferPositive)(pad_data, count);
    pspSdkSetK1(k1);
    return ret;
  }

  return -1;
}


void init_ku_bridge(int devkit_version)
{
  if(devkit_version < 0x03070110)
  {
    __sceCtrlPeekBufferPositive = sceCtrlPeekBufferPositive;
    __sceCtrlReadBufferPositive = sceCtrlReadBufferPositive;
  }
  else
  {
    __sceCtrlPeekBufferPositive = sceCtrlPeekBufferPositive371;
    __sceCtrlReadBufferPositive = sceCtrlReadBufferPositive371;
  }
}


int module_start(SceSize args, void *argp)
{
  __sceCtrlPeekBufferPositive = NULL;
  __sceCtrlReadBufferPositive = NULL;

  return 0;
}

int module_stop(void)
{
  return 0;
}


