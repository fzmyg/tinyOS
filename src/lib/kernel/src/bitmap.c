#include"bitmap.h"
#include"debug.h"
#include"string.h"

/*
 *Description: set bitmap to 0
 *Return Value : void
 * */
void initBitmap(Bitmap*bitmap)
{
	ASSERT(bitmap!=NULL);
	memset(bitmap->pbitmap,0,bitmap->bitmap_byte_len);
}

/*
 * Description : judging bitmap[bit_idx] is used
 * Return Value: used: bit is 1 and return true ,unused: bit is 0 and return false;
 * */
bool bitIsUsed(Bitmap*bitmap,uint32_t bit_idx)
{
	ASSERT(bitmap!=NULL);
	uint32_t byte_index = bit_idx / 8;
	int32_t  bit_index  = bit_idx % 8;
	return  (((bitmap->pbitmap)[byte_index]) & (BIT_MASK << bit_index));
}

/*
 *Description:	check whether bitmap has remaining continues cnt bit
 *Return Value: on successful bit index is returned ,else -1 is returned
 * */
int32_t scanBitmap(Bitmap*bitmap,uint32_t cnt)
{
	ASSERT(bitmap!=NULL);
	uint32_t byte_index = 0;
	//long step to search empty bit
	while(bitmap->pbitmap[byte_index] == 0xff && byte_index < bitmap->bitmap_byte_len)
		byte_index ++ ;
	//ASSERT(byte_index < bitmap->bitmap_byte_len);
	if(byte_index == bitmap->bitmap_byte_len) /*not found*/
		return -1;
	uint32_t bit_index =  byte_index*8+0; 
	uint32_t bit_left  =  bitmap->bitmap_byte_len * 8 - byte_index * 8;
	uint32_t next_bit  =  bit_index; 
	uint32_t count = 0;

	while(bit_left-- >0)
	{
		if(bitIsUsed(bitmap,next_bit)==false){
			count++;
		}else{
			count=0;
		}
		if(count==cnt)
			break;

		next_bit++;
	}
	return next_bit-cnt+1;
}

/*
 *Description  : set val to bitmap by bit_index
 *Return Value : void
 * */
void setBitmap(Bitmap*bitmap,uint32_t bit_index,uint8_t val)
{
	ASSERT(val==0 || val==1);
	ASSERT(bitmap->bitmap_byte_len*8 > bit_index);
	uint32_t byte_index = bit_index / 8;
	uint32_t bit_odd = bit_index % 8 ;
	if(val==1){
		(bitmap->pbitmap)[byte_index] |=  ( BIT_MASK << bit_odd );
	}else{
		(bitmap->pbitmap)[byte_index] &= ~( BIT_MASK << bit_odd );
	}
}
