/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2013 by Michael Kohn
 *
 */

#ifndef _TABLE_STM8_H
#define _TABLE_STM8_H

#include "assembler.h"

#define ST7_NO 0
#define ST7_YES 1

struct _stm8_single
{
  char *instr;
  unsigned char opcode;
  char cycles;
  char st7_support;
};

struct _stm8_x_y
{
  char *instr;
  unsigned char opcode;
  char cycles;
  char st7_support;
};

struct _stm8_r_r
{
  char *instr;
  unsigned char opcode;
  char cycles_min;
  char cycles_max;
  char st7_support;
};

struct _stm8_jumps
{  char *instr;
   unsigned char opcode;
   unsigned char prefix;
};

extern struct _stm8_single stm8_single[];
extern struct _stm8_x_y stm8_x_y[];
extern char *stm8_type1[];
extern char *stm8_type2[];
extern char *stm8_bit_oper[];
extern struct _stm8_r_r stm8_r_r[];
extern struct _stm8_jumps stm8_jumps[];

#endif


