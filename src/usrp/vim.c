#include"stdio.h"
#include"syscall.h"
#include"array.h"
#include"string.h"
#include"stdint.h"

#define LINE_NO 25

struct page_buf{
    char* buf;
    uint32_t start_line,end_line;
    uint32_t edit_line,edit_index;
    uint32_t cursor_pos;
    uint32_t page_len;
};

struct file_buf{
    char* file;
    uint32_t file_len;
};

static uint32_t getUpLineIndex(const char*file_content,uint32_t edit_index)
{
    int32_t i = (int32_t)edit_index - 1;
    uint32_t down_off,up_len;
    for(;i>=0;i--){
        if(file_content[i]=='\r'||file_content[i]=='\n'){
            break;
        }
    }
    if(i==-1)
        return edit_index;
    down_off =  edit_index - i;
    for(i--;i>=0;i--){
        if(file_content[i]=='\r'||file_content[i]=='\n')
            break;
    }
    up_len = edit_index - down_off - i;
    if(up_len>down_off){
        return i+down_off;
    }else{
        return edit_index - down_off;
    }
    return 0;
}


static uint32_t getDownLineIndex(const char*file_content,uint32_t file_content_len,uint32_t edit_index)
{
    int32_t i = edit_index - 1;
    uint32_t up_off = 0,up_len = 0,down_len = 0;
    for(;i>=0;i--){
        if(file_content[i]=='\r'||file_content[i]=='\n')
            break;
    }
    up_off = edit_index - i;
    for(i=edit_index;i<(int32_t)file_content_len;i++){
        if(file_content[i]=='\r'||file_content[i]=='\n')
            break;
    }
    if(i==(int32_t)file_content_len){ //若为最后一行
        return edit_index;
    }
    up_len = i - edit_index + up_off;
    for(i++;i<(int32_t)file_content_len;i++){
        if(file_content[i]=='\r'||file_content[i]=='\n')
            break;
    }
    down_len = i - (up_len + edit_index - up_off);
    if(down_len>up_off){
        return up_len - up_off + edit_index + up_off;
    }else{
        return i;
    }  
    return 0;
}


static uint32_t getEditLine(const char* file_content,uint32_t edit_index)
{
    int32_t i = (int32_t)edit_index - 1;
    uint32_t enter_cnt = 0;
    for(;i>=0;i--){
        if(file_content[i]=='\n'||file_content[i]=='\r')
            enter_cnt ++;
    }
    return enter_cnt;
}


static char* getPageBuf(const char*file_content,uint32_t file_content_len,uint32_t page_start_line,uint32_t*page_len)
{
    uint32_t i = 0,enter_cnt = 0;
    *page_len = 0;
    for(;i<file_content_len && enter_cnt != page_start_line ;i++){
        if(file_content[i]=='\n'||file_content[i]=='\r')
            enter_cnt ++;
    }    
    const char* page_start = file_content+i;
    enter_cnt = 0;
    for(;i<file_content_len;i++){
        if(file_content[i]=='\n'||file_content[i]=='\r')
            enter_cnt ++;
        if(enter_cnt == 25)
            break;
    }
    *page_len = i - (page_start - file_content);
    return (char*)page_start;
}

static uint32_t getCursorPos(const char*file_content,uint32_t file_content_len,uint32_t index)
{
    bool first_enter_tag = true;
    uint32_t line_no=0,line_off=0;
    int32_t i = (int32_t)index - 1;
    while(i>=0){
        if(file_content[i]=='\r'||file_content[i]=='\n'){
            if(first_enter_tag){
                first_enter_tag = false;
            }
            line_no ++;
        }else{
            if(first_enter_tag){
                if(file_content[i]=='\t')
                    line_off+=4;
                else    
                    line_off++;
            }
        }
        i--;
    }
    return line_no*80+line_off;
}



int main(int argc,char**argv)
{
    int fd = -1;
    struct file_stat file_stat;
    struct string file_content_buf;

    char ch;
    bool exit_tag = false;
    bool file_exist_tag = false;

    uint32_t edit_index = 0,edit_line = 0;
    uint32_t page_start_line = 0, page_end_line = LINE_NO - 1;
    uint32_t file_content_len=0;char*file_content=NULL;
    uint32_t cursor_pos = 0;
    
    char* page_buf = NULL;
    uint32_t page_len;
 

    if(argc!=2){
        printf("format error:vim + file name\n");
        return 0;
    }
    if(stat(argv[1],&file_stat)==0){
        file_exist_tag = true;
    }
    if(file_exist_tag == false){ //文件不存在
        fd = open(argv[1],O_CREATE|O_RDWR);
        if(initString(&file_content_buf,1024)==-1){
            printf("system busy , please try again later\n");
            close(fd);
            return 0;
        }
    }else{
        if(file_stat.st_ft == FT_DIRECTORY){ //文件为目录
            printf("can not edit a directory\n");
            return 0;
        }else{ //文件存在
            fd = open(argv[1],O_RDWR);
            if(initString(&file_content_buf,file_stat.st_size)==-1){
                printf("system busy , please try again later\n");
                close(fd);
                return 0;
            }
            file_content = getstring(&file_content_buf,&file_content_len);
            int32_t read_size = read(fd,file_content,file_stat.st_size) ;
            pushstr(&file_content_buf,file_content,file_stat.st_size);
            file_content = getstring(&file_content_buf,&file_content_len);
            if(read_size != (int32_t)file_stat.st_size){
                printf("read file content error\n");
                close(fd);
                releaseString(&file_content_buf);
                return -1;
            }
        }
    }
     

    while(exit_tag == false){
        file_content = getstring(&file_content_buf,&file_content_len);
        if(file_content == NULL) exit(-1);
        edit_line = getEditLine(file_content,edit_index);
        if(edit_line<page_start_line){
            page_start_line --;page_end_line--;
        }else if(edit_line>page_end_line){
            page_end_line++;page_start_line++;
        }
        page_buf = getPageBuf(file_content,file_content_len,page_start_line,&page_len);
        clear();
        write(stdout,page_buf,page_len);
        cursor_pos = getCursorPos(page_buf,page_len,edit_index-(page_buf - file_content));
        setcursor((int)cursor_pos);

        if(read(stdin,&ch,1)!=1){
            printf("system busym,please try again later\n");
            break;
        }

        switch(ch){
            case (char)27:
                while(ch != 'y' && ch!='n' &&  ch!='Y' && ch!='N')
                    read(stdin,&ch,1);
                if(ch == 'y' || ch == 'Y')
                    exit_tag = true;
                else    
                    exit_tag = false;
                break;

            case '\b':
                if(edit_index>0){
                    edit_index --;
                    removechar(&file_content_buf,edit_index);
                    file_content_len--;
                }
                break;
            case (char)0x81:
                edit_index = getUpLineIndex(file_content,edit_index);
                break;
            case (char)0x82:
                edit_index = getDownLineIndex(file_content,file_content_len,edit_index);
                break;
            case (char)0x83:
                if(edit_index>0) edit_index--;
                break;
            case (char)0x84:
                if(edit_index < file_content_len) edit_index++;
                break;
            default:
                insertchar(&file_content_buf,ch,edit_index);
                file_content = getstring(&file_content_buf,&file_content_len);
                edit_index++;
                break;
        }
  
    }
    file_content = getstring(&file_content_buf,&file_content_len);
    lseek(fd,0,SEEK_SET);
    write(fd,file_content,file_content_len);
    clear();
    close(fd);
    releaseString(&file_content_buf);
    return 0;
}