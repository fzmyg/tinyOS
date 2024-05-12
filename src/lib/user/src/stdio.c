#include"stdint.h"
#include"string.h"
#include"stdio.h"
#include"syscall.h"
#define va_start(ap,v) ap = (va_list*)&v;
#define va_arg(ap,type) (*((type*)((ap+=sizeof(type))-sizeof(type))))
#define va_end(ap) ap = NULL

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

uint32_t vsprintf(char*str,const char*format,va_list ap)
{
    ap+=sizeof(char*);  //跳过format
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
                    s = va_arg(ap,char*);
                    len = strlen(s);
                    strncpy(pbuf,s,len);
                    pbuf+=len;
                    break;
                case 'c': ;
                    ch = va_arg(ap,char);
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

uint32_t printf(const char* format,...)
{
    va_list arg = NULL;
    va_start(arg,format);
    char buf[1024]={0}; 
    uint32_t len = vsprintf(buf,format,arg);
    len = write(buf);
    return len;
}
