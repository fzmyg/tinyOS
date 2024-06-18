#ifndef __KEY_BOARD_H__
#define __KEY_BOARD_H__

#define KBD_BUF_PORT 0x60
#define esc 27
#define backspace '\b'
#define tab  '\t'
#define enter '\r'
#define delete 127

#define char_invisible 0
#define ctrl_l_char char_invisible
#define ctrl_r_char char_invisible
#define shift_l_char char_invisible
#define shift_r_char char_invisible
#define alt_l_char char_invisible
#define alt_r_char char_invisible
#define caps_lock_char char_invisible
#define up_char char_invisible
#define down_char char_invisible
#define left_char char_invisible
#define right_char char_invisible


#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define alt_r_break 0xe0b8
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a

#define up_make 0xe048
#define down_make 0xe050
#define left_make 0xe04b
#define right_make 0xe04d

#include"ioqueue.h"

extern struct ioqueue kbd_buf;

extern void int_kbd_handler(void);

extern void initKeyBoard(void);
#endif
