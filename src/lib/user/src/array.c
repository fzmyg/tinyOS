#include"array.h"
#include"syscall.h"
#include"string.h"
#define ARRAY_INIT_SIZE 32
int initArray(struct array*arr,uint32_t val_size,uint32_t init_arr_len)
{
    char*buf = malloc(init_arr_len*val_size);
    if(buf==NULL) return -1;
    arr->start = arr->end = buf;
    arr->edge = arr->start + init_arr_len*val_size;
    arr->val_size = val_size;
    return 0;
}

int releaseArray(struct array* arr){
    free(arr->start);
    return 0;
}

static int appendCapacity(struct array*arr,uint32_t size)
{
    char*buf = malloc(size);
    if(buf==NULL) return -1;
    memcpy(buf,arr->start,arr->end-arr->start);
    arr->start = buf;
    arr->end = arr->start + size/2;
    arr->edge = arr->start + size;
    free(arr->start);
    return 0;
}


int push(struct array*array,void*val)
{
    if(array->end==array->edge){
        if(appendCapacity(array, (array->end - array->start) * 2)==-1)
            return -1;
    }
    memcpy(array->end,val,array->val_size);
    array->end+=array->val_size;
    return 0;
}

int insert(struct array*array,void*val,uint32_t index)
{
    if(array->end==array->edge){
        if(appendCapacity(array, (array->end - array->start) * 2)==-1)
            return -1;
    }
    uint32_t total_len = (array->end-array->start)/array->val_size; //总数据数
    uint32_t move_len  = total_len - index;                         //需移动数据数
    memmove(array->start+(index+1)*array->val_size,array->start+index*array->val_size,move_len*array->val_size);
    memcpy(array->start+index*array->val_size,val,array->val_size);
    array->end+=array->val_size;
    return 0;
}

int remove(struct array*array,uint32_t index)
{
    uint32_t total_len = (array->end-array->start)/array->val_size; //总数据数
    uint32_t move_len  = total_len - index ;                         //需移动数据数

    memcpy(array->start+array->val_size*index,array->start+array->val_size*(index+1),move_len*array->val_size);
    array->end -= array->val_size;
    return 0;
}

int pushs(struct array*array,void*val,uint32_t len)
{
    if(array->end + (len - 1)*array->val_size >= array->edge){
        if(appendCapacity(array, (array->end + (len - 1)*array->val_size - (char*)NULL) * 2)==-1)
            return -1;
    }
    memcpy(array->end,val,len*array->val_size);
    array->end+= (len* array->val_size);
    return 0;
}

int pop(struct array*array,void*ret)
{
    if(array->end==array->start) return -1;
    array->end -= array->val_size;
    memcpy(ret,array->end,array->val_size);
    return 0;
}

char* travalArray(struct array*arr,bool(*func)(void*,uint32_t))
{
    char* p = arr->start;
    while(p<=arr->end){
        if(func(p,arr->val_size))
            return p;
        p+=arr->val_size;
    }
    return NULL;
}

int initString(struct string*s,uint32_t int_strlen)
{
    return initArray(&s->array,1,int_strlen);
}

int releaseString(struct string* arr)
{
    return releaseArray(&arr->array);
}

int pushchar(struct string * s ,char ch)
{
    return push(&s->array,&ch);
}

int insertchar(struct string*s , char ch ,uint32_t index)
{
    return insert(&s->array,&ch,index);
}

int removechar(struct string*s,uint32_t index)
{
    return remove(&s->array,index);
}


int popchar(struct string * s,char* ch)
{
    return pop(&s->array,ch);
}

int pushstr(struct string * str ,char*s,uint32_t len)
{
    return pushs(&str->array,s,len);
}

char* getstring(struct string*s,uint32_t*len)
{
    *len = s->array.end-s->array.start;
    return s->array.start;
}

int initIntArray(struct int_array*s,uint32_t init_arr_len)
{
    return initArray(&s->array,4,init_arr_len);
}

int releaseIntArray(struct int_array* arr)
{
    return releaseArray(&arr->array);
}

int pushint(struct int_array * s ,int ch)
{
    return push(&s->array,&ch);
}

int insertint(struct int_array*s , int ch ,uint32_t index)
{
    return insert(&s->array,&ch,index);
}

int removeint(struct int_array*s,uint32_t index)
{
    return remove(&s->array,index);
}

int pushints(struct int_array * s ,int*ch,uint32_t len)
{
    return pushs(&s->array,ch,len);
}

int popint(struct int_array * s,int* ch)
{
    return pop(&s->array,ch);
}

int* getIntArray(struct int_array*s,uint32_t*len)
{
    *len = (s->array.end-s->array.start)/4;
    return (int*)s->array.start;
}