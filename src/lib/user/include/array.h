#ifndef __ARRAY_H__
#define __ARRAY_H__
#include"stdint.h"
struct array{
    char* start,*end,*edge;
    uint32_t val_size;
};

extern int initArray(struct array*arr,uint32_t val_size,uint32_t init_arr_len);

extern int releaseArray(struct array* arr);

extern int push(struct array*array,void*val);

extern int insert(struct array*array,void*val,uint32_t index);

extern int remove(struct array*array,uint32_t index);

extern int pushs(struct array*array,void*val,uint32_t len);

extern int pop(struct array*array,void*ret);

extern char* travalArray(struct array*arr,bool(*func)(void*,uint32_t));


struct string{
    struct array array;
};

extern int initString(struct string*s,uint32_t init_strlen);

extern int releaseString(struct string* arr);

extern int pushchar(struct string * s ,char ch);

extern int pushstr(struct string * str ,char*s,uint32_t len);

extern int insertchar(struct string*s , char ch ,uint32_t index);

extern int removechar(struct string*s,uint32_t index);

extern int popchar(struct string * s,char *ch);

extern char* getstring(struct string*s,uint32_t* len);


struct int_array{
    struct array array;
};

extern int initIntArray(struct int_array*s,uint32_t init_arr_len);

extern int releaseIntArray(struct int_array* arr);

extern int pushint(struct int_array * s ,int ch);

extern int pushints(struct int_array* arr,int*ch,uint32_t cnt);

extern int insertint(struct int_array*s , int ch ,uint32_t index);

extern int removeint(struct int_array*s,uint32_t index);

extern int popint(struct int_array * s,int* ch);

extern int* getIntArray(struct int_array*s,uint32_t*len);


#endif