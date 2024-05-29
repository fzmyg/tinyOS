#include"stdint.h"
#include"string.h"
#include"stdio.h"
#include"syscall.h"



/*将整型val按base进制转换为相应字符串放入buf中*/
static uint32_t itoa(uint32_t val,char*buf,uint8_t base)
{   if(val==0){
        buf[0]='0';
        return 1;
    }
    int index = 0;
    while(val){
        uint32_t num = val%base;
        if(num >= 10)
            buf[index++]=num-10+'a';
        else
            buf[index++]=num + '0';
        val/=base;    
    }
    int len = index;
    while (index>len/2)
    {
        char tmp = buf[index];
        buf[index]=buf[len-1-index];
        buf[len-1-index]=tmp;
        index--;
    }
    return len;
}

/*将格式化字符串转换后放入str缓冲区*/
uint32_t vsprintf(char*const str,const char*format,va_list ap)
{
    ap+=sizeof(const char*);  //跳过format
    char* pbuf = str;
    int translate_tag = 0;
    while(*format){
        if(translate_tag==1){
            char buf[1024]={0};
	    int val = 0;
            char *s =NULL;
            char ch = 0;
	    uint32_t len = 0;
            switch(*format){
                case 'x': ;
                    val=va_arg(ap,int);
                    len = itoa(val,buf,16);
                    strncpy(pbuf,buf,len);
                    pbuf+=len;
                    break;
                case 'd': ;
                    val = va_arg(ap,int);
                    len = itoa(val,buf,10);
                    strncpy(pbuf,buf,len);
                    pbuf+=len;
                    break;
                case 's': ;
                    s = (char*)va_arg(ap,char*);
                    len = strlen(s);
                    strncpy(pbuf,s,len);
                    pbuf+=len;
                    break;
                case 'c': ;
                    ch = (char)va_arg(ap,int);
                    *pbuf=ch;
                    pbuf++;
                    break;
            }
            format++;
            translate_tag = 0;
            continue;
        }
        if(*format=='%'){
            translate_tag = 1;
            format++;
        }else{
            *pbuf = *format;
            pbuf++;
            format++;  
        }
    }
    return strlen(str);
}

/* 格式化打印 */
uint32_t printf(const char* format,...)
{
    va_list arg = NULL;
    va_start(arg,format);
    char buf[1024]={0}; 
    uint32_t len = vsprintf(buf,format,arg);
    len = write(buf);
    return len;
}

/* 将格式化字符串转换后放入缓冲区buf */
uint32_t sprintf(char*buf,const char*format,...)
{
    va_list args;
    va_start(args,format);
    uint32_t len=vsprintf(buf,format,args);
    va_end(args);
    return len;
}