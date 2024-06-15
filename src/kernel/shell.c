#include"syscall.h"
#include"stdio.h"
#include"string.h"
#define MAX_PATH_LEN 512
#define CMD_LEN 128

char cwd_buf[MAX_PATH_LEN];
char cmd_buf[CMD_LEN];
uint32_t stat_pos;
static uint32_t print_prompt()
{
    memset(cwd_buf,0,MAX_PATH_LEN);
    getcwd(cwd_buf,MAX_PATH_LEN);
    return printf("[zbccc@localhost:%s]$",cwd_buf);
}

static void readLine(char*buf,uint32_t count)
{
    int i = 0;
    for(;i<count;i++){
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
            default:
                putchar(buf[i]);
                break;
        }
    }
}

void shell(void)
{
    clear();
    while(1){
        stat_pos=print_prompt();
        memset(cmd_buf,0,CMD_LEN);
        readLine(cmd_buf,CMD_LEN);//读取命令
        if(cmd_buf[0]==0) continue;
        //命令解析
    }
}