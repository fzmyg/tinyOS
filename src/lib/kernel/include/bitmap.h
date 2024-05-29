#ifndef __BIT_MAP_H__
#define __BIT_MAP_H__

#include"stdint.h"
#define BIT_MASK 1
typedef struct Bitmap{
	uint32_t bitmap_byte_len;
	uint8_t * pbitmap;
}Bitmap;
extern void initBitmap(Bitmap*bitmap);
extern bool bitIsUsed(Bitmap*bitmap,uint32_t bit_index);
/* 
 * Description: find continuous cnt bit 
 * Return     : for successful:return the first bit_index of bitmap,failed return -1
 * */
extern int32_t  scanBitmap(Bitmap*bitmap,uint32_t cnt);
extern void setBitmap(Bitmap*bitmap,uint32_t bit_index,uint8_t value);
#endif
