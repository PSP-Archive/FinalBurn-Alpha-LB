  .set noreorder

  #include "pspimport.s"

  IMPORT_START "sceCtrl_driver", 0x00010000

  IMPORT_FUNC  "sceCtrl_driver", 0x454455AC, sceCtrl_driver_454455AC
  IMPORT_FUNC  "sceCtrl_driver", 0x1F803938, sceCtrl_driver_1F803938

  IMPORT_FUNC  "sceCtrl_driver", 0xC4AAD55F, sceCtrl_driver_C4AAD55F
  IMPORT_FUNC  "sceCtrl_driver", 0x3A622550, sceCtrl_driver_3A622550

