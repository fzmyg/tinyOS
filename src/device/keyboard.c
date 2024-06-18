#include"keyboard.h"
#include"stdint.h"
#include"interrupt.h"
#include"io.h"
#include"print.h"
#include"ioqueue.h"
static bool ctrl_status=0,shift_status=0,alt_status=0,caps_lock_status=0,ext_scancode=0;
struct ioqueue kbd_buf;
static char keymap[][2]={{0,0},/*0x01*/{esc,esc},/*0x02*/{'1','!'},/*0x03*/{'2','@'},/*0x04*/{'3','#'},/*0x05*/{'4','$'},/*0x06*/{'5','%'},/*0x07*/{'6','^'},/*0x08*/{'7','&'},/*0x09*/{'8','*'},/*0x0a*/{'9','('},/*0x0b*/{'0',')'},/*0x0c*/{'-','_'},/*0x0d*/{'=','+'},/*0x0e*/{backspace,backspace},/*0x0f*/{tab,tab},/*0x10*/{'q','Q'},/*0x11*/{'w','W'},/*0x12*/{'e','E'},/*0x13*/{'r','R'},/*0x14*/{'t','T'},/*0x15*/{'y','Y'},/*0x16*/{'u','U'},/*0x17*/{'i','I'},/*0x18*/{'o','O'},/*0x19*/{'p','P'},/*0x1a*/{'[','{'},/*0x1b*/{']','}'},/*0x1c*/{enter,enter},/*0x1d*/{ctrl_l_char,ctrl_l_char},/*0x1e*/{'a','A'},/*0x1f*/{'s','S'},/*0x20*/{'d','D'},/*0x21*/{'f','F'},/*0x22*/{'g','G'},/*0x23*/{'h','H'},/*0x24*/{'j','J'},/*0x25*/{'k','K'},/*0x26*/{'l','L'},/*0x27*/{';',':'},/*0x28*/{'\'','"'},/*0x29*/{'`','~'},/*0x2a*/{shift_l_char,shift_l_char},/*0x2b*/{'\\','|'},/*0x2c*/{'z','Z'},/*0x2d*/{'x','X'},/*0x2e*/{'c','C'},/*0x2f*/{'v','V'},/*0x30*/{'b','B'}, /*0x31*/{'n','N'},/*0x32*/{'m','M'},/*0x33*/{',','<'},/*0x34*/{'.','>'},/*0x35*/{'/','?'},/*0x36*/{shift_r_char,shift_r_char},/*0x37*/{'*','*'},/*0x38*/{alt_l_char,alt_l_char},/*0x39*/{' ',' '},/*0x3a*/{caps_lock_char,caps_lock_char}};


static bool isExtCode(const uint16_t code)
{
	return code == 0x00e0;
}


void int_kbd_handler(void)
{
	uint16_t code = (uint16_t)inb(KBD_BUF_PORT);
	if(isExtCode(code)){
		ext_scancode = 1;
		return;
	}
	if(ext_scancode){
		code = (code | 0xe000);
		ext_scancode = 0;
	}
	if((code&0x80)!=0){ //break code
		uint16_t origin_scancode = code & 0xff7f;
		if((origin_scancode == shift_l_make) || (origin_scancode == shift_r_make)) 
			shift_status = 0;
		else if((origin_scancode == ctrl_l_make) || (origin_scancode == ctrl_r_make))
			ctrl_status = 0;
		else if((origin_scancode == alt_l_make) || (origin_scancode == alt_r_make))
			alt_status = 0;
		/* do not process caps lock*/
		return ; 
	}
	else if((code>0&&code<0x3b)||(code==ctrl_r_make)||(code==alt_r_make)){	//make code
		bool shift_tag = 0;
		if(code<0x0e||code==0x29||code==0x1a||code==0x1b||code==0x2b||code==0x27||code==0x28||code==0x33||code==0x34||code==0x35){  /*'0'~'9' '-' '=' '`' '[' ']' '\\' ';' '\'' ',' '.' '/'*/ 
			if(shift_status) shift_tag = 1;
		}else{ /* 'a'~'z' */
			if(shift_status && caps_lock_status) shift_tag = 0;
			else if(shift_status || caps_lock_status) shift_tag = 1;
			else shift_tag = 0;
		}
		
		uint8_t index = code & 0xff; //remove high byte
		char cur_char = keymap[index][shift_tag];
		if((cur_char=='l' || cur_char=='c' )&&ctrl_status==true){ //ctrl + l 快捷键
			if(!isIOQueueFull(&kbd_buf)){
				ioq_put_char(&kbd_buf,cur_char-'a');
			}
			return;
		}
		if(cur_char!=0){	
			if(!isIOQueueFull(&kbd_buf)){
				ioq_put_char(&kbd_buf,cur_char);
			}
			return;
		}
		if(code == shift_l_make || code == shift_r_make) shift_status = 1;
		else if(code == ctrl_l_make || code == ctrl_r_make) ctrl_status = 1;
		else if(code == alt_l_make || code == alt_r_make) alt_status = 1;
		else if(code == caps_lock_make) caps_lock_status = !caps_lock_status;
	}
	else{
		if(code==up_make) ioq_put_char(&kbd_buf,0x81);
		else if(code == down_make) ioq_put_char(&kbd_buf,0x82);
		else if(code == left_make) ioq_put_char(&kbd_buf,0x83);
		else if(code == right_make) ioq_put_char(&kbd_buf,0x84);
	}
	
}	

void initKeyBoard(void)
{
	put_str("init keyboard start\n");
	initIOQueue(&kbd_buf);
	put_str("init keyboard end \n");
}
