#include"syscall.h"
#include"shell.h"
#include"buildin.h"
#include"stdio.h"
#include"string.h"
extern char cmd_argv[MAX_PARAMENTS_NO][MAX_PARAMENTS_LEN];
#define UNUSED __attribute__((unused))

int32_t buildin_pwd(uint32_t argc UNUSED ,char* argv[MAX_PARA_NO] UNUSED)
{
    printf("%s\n",cwd_buf);
    return 0;
}


int32_t buildin_ls(uint32_t argc,char* argv[MAX_PARA_NO])
{
    bool  l_tag = false,a_tag = false;
    uint32_t para_cnt = 0;
    int32_t path_index = -1;
    uint32_t i = 1;
    for(;i<argc;i++){
        if(argv[i][0]=='-'){
            para_cnt++;
            if(argv[i][1]=='l')
                l_tag = true;
            if(argv[i][1]=='a')
                a_tag = true;
        }else{
            path_index = i;
        }
    } 
    if(argc-1-para_cnt>1){
        printf("Invalid format:too many path parameters\n");
        return -1;
    }
    struct dir* dir = NULL;
    char*path=NULL;
    if(path_index == -1){
        dir = opendir(cwd_buf);
        path = cwd_buf;
    }else{
        dir=opendir(argv[path_index]);
        path=argv[path_index];
    }if(dir==NULL){
        printf("System busy:please try argin later\n");
        return -1;
    }

    if(l_tag){        
        struct dir_entry* dir_entry = readdir(dir);
        while(dir_entry!=NULL){
            if(a_tag||dir_entry->name[0]!='.'){
                struct file_stat fs;
                if(stat(dir_entry->name,&fs)==-1){
                    printf("System error:please try argin later\n");
                    free(dir_entry);
                    closedir(dir);
                    return -1;
                }
                char type_char = dir_entry->f_type == FT_DIRECTORY ? 'd' : '-' ;
                printf("%c %d %s\n",type_char,fs.st_size,dir_entry->name);
            }
            free(dir_entry);
            dir_entry=readdir(dir);
        }
    }else{
        struct dir_entry* dir_entry = readdir(dir);
        int out_cnt = 0;
        while(dir_entry!=NULL){
            if(a_tag||dir_entry->name[0]!='.'){
                printf("%s ",dir_entry->name);
                out_cnt ++;
            }
            free(dir_entry);
            dir_entry=readdir(dir);
        }
        if(out_cnt) putchar(10);
    }
    closedir(dir);
    return 0;
}

int32_t buildin_ps(uint32_t argc,char* argv[MAX_PARA_NO] UNUSED)
{
    if(argc!=1){
        printf("Invalid format:ps() should not have any arguments\n");
        return -1;
    }
    return ps();
}

int32_t buildin_mkdir(uint32_t argc,char* argv[MAX_PARA_NO])
{
    if(argc!=2&&argv[1][0]=='-') {
        printf("Invalid format:mkdir() should not have any arguments except a pathstring\n");
        return -1;
    }
    if(mkdir(argv[1])==-1){
        return -1;
    }
    return 0;
}

int32_t buildin_rm(uint32_t argc,char* argv[MAX_PARA_NO])
{
    if(argc!=2&&argv[1][0]=='-') {
        printf("Invalid format:rmdir() should not have any arguments except a pathstring\n");
        return -1;
    }
    if(strcmp(argv[1],".")==0 || strcmp(argv[1],"..")==0){
        printf("can not remove such directory\n");
        return -1;
    }
    struct file_stat file_stat;
    memset(&file_stat,0,sizeof(struct file_stat));
    stat(argv[1],&file_stat);
    if(file_stat.st_ft==FT_DIRECTORY){
        if(rmdir(argv[1])==-1){
            printf("system error:remove directory faild\n");
            return -1;
        }
    }else{
        if(unlink(argv[1])==-1){
            printf("system error:remove file faild\n");
            return -1;
        }
    }
    return 0;
}

int32_t buildin_cd(uint32_t argc,char* argv[MAX_PARA_NO])
{
    if(argc!=2&&argv[1][0]=='-') {
        printf("Invalid format:rmdir() should not have any arguments except a pathstring\n");
        return -1;
    }
    if(chdir(argv[1])==-1){
        printf("chdir error\n");
        return -1;
    }else{
        getcwd(cwd_buf,MAX_PATH_LEN);
    }
    return 0;
}
