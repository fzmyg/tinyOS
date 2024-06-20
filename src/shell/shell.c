#include"syscall.h"
#include"stdio.h"
#include"string.h"
#include"shell.h"
#include"buildin.h"

#define MAX_PATH_LEN 512
#define CMD_LEN 128

char cwd_buf[MAX_PATH_LEN];
char cmd_buf[CMD_LEN];
char*cmd_argv[MAX_PARAMENTS_NO];
uint32_t stat_pos;
static uint32_t print_prompt(void)
{
    memset(cwd_buf,0,MAX_PATH_LEN);
    getcwd(cwd_buf,MAX_PATH_LEN);
    return printf("[zbccc@localhost:%s]$",cwd_buf);
}


static void readLine(char*buf,uint32_t count)
{
    int i = 0;
    for(;i<(int)count;i++){
        read(stdin,buf+i,1);
        switch(buf[i]){
            case '\r':
            case '\n':
                buf[i]=0;
                putchar('\n');
                return;
            case '\b':
                if(i>0){
                    putchar('\b');
                    i-=2;
                }else if(i==0){
                    i-=1;
                }
                break;
            case '\t':
                i--;
                break;
            case (char)0x81: //上键
                while(i-->0){
                    putchar('\b');
                }
                memset(buf,0,count);
                break;
            case (char)0x82: //下键
                while(i-->0){
                    putchar('\b');
                }
                memset(buf,0,count);
                break;
            case (char)0x83://左键
                if(i>0){
                    putchar('\b');
                    i-=2;
                }else if(i==0){
                    i-=1;
                }
                break;
            case (char)0x84://右键
                putchar(' ');
                break;
            case 'l'-'a':
                clear();
                print_prompt();
                buf[i]=0;
                printf("%s",buf);
                i--;
                break;
            case 'c'-'a':
                printf("^C");
                putchar('\n');
                i=-1;
                memset(buf,0,count);
                print_prompt();
                break;
            default:
                putchar(buf[i]);
                break;
        }
    }
}

static int32_t  cmd_parse(char*cmd,char* argv[MAX_PARAMENTS_NO])
{
    int32_t cmd_len = (int32_t)strlen(cmd);
    int32_t argv_cnt = 0;
    int i = 0,j = 0;
    for(;i < cmd_len;i++){
        if(argv_cnt>MAX_PARAMENTS_NO) return -1;
        while(i<cmd_len && cmd[i]==' ') i++;
        j = i + 1;
        while(j<cmd_len && cmd[j]!=' ' && cmd[j]!='\0') j++;
        if(j-i>=MAX_PARAMENTS_LEN) return -1;
        argv[argv_cnt] = cmd+i;
        if(j<cmd_len) cmd[j]='\0';
        argv_cnt++;
        i = j;
    }
    return argv_cnt;
}


static int32_t execbuildin(uint32_t argv_cnt,char* argv[MAX_PARAMENTS_NO])
{
    if(!strcmp(argv[0],"ls\0")){
        buildin_ls(argv_cnt,argv);
        return 0;
    }if(!strcmp(argv[0],"ps\0")){
        buildin_ps(argv_cnt,argv);
        return 0;
    }if(!strcmp(argv[0],"mkdir\0")){
        buildin_mkdir(argv_cnt,argv);
        return 0;
    }if(!strcmp(argv[0],"rm\0")){
        buildin_rm(argv_cnt,argv);
        return 0;
    }if(!strcmp(argv[0],"cd\0")){
        buildin_cd(argv_cnt,argv);
        return 0;
    }if(!strcmp(argv[0],"pwd\0")){
        buildin_pwd(argv_cnt,argv);
        return 0;
    }if(!strcmp(argv[0],"clear\0")){
        clear();
        return 0;
    }
    return -1;
}

void shell(void)
{
    clear();
    getcwd(cwd_buf,MAX_PATH_LEN);
    while(1){
        stat_pos=print_prompt();
        memset(cmd_buf,0,CMD_LEN); //清空输入缓冲区
        memset(cmd_argv,0,sizeof(uint32_t)*MAX_PARAMENTS_NO);//清空命令解析缓冲区
        readLine(cmd_buf,CMD_LEN);//读取命令
        if(cmd_buf[0]==0) continue;
        //命令解析
        int argv_cnt = (int)cmd_parse(cmd_buf,cmd_argv);
        if(argv_cnt==-1){
            printf("\nparament length invild\n");
            continue;
        }
        if(execbuildin(argv_cnt,cmd_argv)==0){ //shell内置命令
            continue;
        }else{
            printf("command not find\n");
        }
    }
}