#ifndef __BUILD_IN_H__
#define __BUILD_IN_H__
#include"stdint.h"
#include"shell.h"
#define MAX_PARA_LEN  MAX_PARAMENTS_LEN
#define MAX_PARA_NO   MAX_PARAMENTS_NO

extern int32_t buildin_pwd(uint32_t argc,char* argv[MAX_PARA_NO]);

extern int32_t buildin_ls(uint32_t argc,char* argv[MAX_PARA_NO]);

extern int32_t buildin_ps(uint32_t argc,char* argv[MAX_PARA_NO]);

extern int32_t buildin_mkdir(uint32_t argc,char* argv[MAX_PARA_NO]);

extern int32_t buildin_rm(uint32_t argc,char* argv[MAX_PARA_NO]);

extern int32_t buildin_cd(uint32_t argc,char* argv[MAX_PARA_NO]);
#endif