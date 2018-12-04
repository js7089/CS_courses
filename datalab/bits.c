/* 
 * CS:APP Data Lab 
 * 
 * 20150211 Jinsik Kim
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses ISO/IEC 10646 (2nd ed., published 2011-03-15) /
   Unicode 6.0.  */
/* We do not support C11 <threads.h>.  */
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 	*/
int bitAnd(int x, int y) {
  // De Morgan's Law
  // (not ((not x) or (not y))) is equivalant to (x and y)
  return ~((~x)|(~y));
}
/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
  // shift x by 8*n steps right, and extract the least significant 8bits
  return (x>>(n<<3))&0xFF;
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
  int large = (0x1 << 31); // 1000 0000 .... 0000
  // MSB and following n-bits replaced to 0 in case n>0 and msb = 1
  int cond = (!((!n & ((x&large)>>31))&0x1))<<31;
  // otherwise simply x>>n
  return   (~((cond>>n)<<1)) & (x>>n);
  
  
  	
}
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
  int unit = 0x11; // 0001 0001
  int unit_d = unit | unit<<8 | unit<<16 | unit<<24; // 0001 0001 0001 0001 0001 0001 0001 0001
  // unit_d = 0x11111111
  int x1 = x;
  int x2 = x>>1;
  int x3 = x>>2;
  int x4 = x>>3;
  /* x1~x4 : shift the original x by 0~3 bits.
   * by ANDing with 0x11111111, we know which bit is 1 or 0
   * partitions 32-bit int to 8 sections
   */
  int sum = (x1&unit_d) + (x2&unit_d) + (x3&unit_d) + (x4&unit_d);
  /* x & 0x11111111, each byte will be 1 if there's 1, 0 otherwise.
   * for example, if x = 0x1001, the result is 0x00001001.
   * summing these will produce number, where each byte represents
   * occurence of 1's. 
   * ex) sum = 0x42321041 means 4+2+3+2+1+0+4+1 = 17 occurences of 1's
   */

  // the return value is simply adding each bytes, elaborated above.
  return ((sum>>28)&(0xF)) + ((sum>>24)&(0xF)) + ((sum>>20)&(0xF)) + ((sum>>16)&(0xF)) + ((sum>>12)&(0xF)) + ((sum>>8)&(0xF)) + ((sum>>4)&0xF) + (sum&0xF);
}
/* 
 * bang - Compute !x without using !
 *   Examples: bang(3) = 0, bang(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int bang(int x) {
  /* a : 32bit = 16bit + 16bit
   * if 1 exists in either section, then the OR result will also contain 1.
   * halving through b,c,d,e ...
   * divide-and-conquer
   */
  int a1 = (x>>16) | x; 
  int b = (a1>>8) | a1;
  int c = (b>>4) | (b);
  int d = (c>>2) | c;
  int e = ~(((d>>1) | d )) & 0x1; // is final result is 1?
 
  return e;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  // is 0x80000000
  return 0x1<<31;
}
/* 
 * fitsBits - return 1 if x can be represented as an 
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
  /* assume that x can be represented as n-bit 2's complement.
   * then removing first (32-n) bits doesn't matter.
   */
  int truncateV = 33+(~n); // 32-n = 32 + (~n) + 1 = 33 + (~n)

  // lshift (32-n)bits -> rshift (32-n)bits 
  // are they the same? same=possible=1 / otherwise 0

  return  !((x<<truncateV>>truncateV)^x);

}
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
  int msb = x>>31;
  int bias = (0x1<<n)+(~0); // 2^n-1

  /* case positive : simply x>>n
   * if negative, bias(=2^n-1) must be added before shifting
   */

  return (x+(bias&msb))>>n;
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  // 2's complement negation ~x+1
  return ~x+1;
}
/* 
 * isPositive - return 1 if x > 0, return 0 otherwise 
 *   Example: isPositive(-1) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 3
 */
int isPositive(int x) {
  // MSB = 0 and x!=0
  int msb = (x>>31) & (0x1);
  int iszero = !x;
  return (!iszero) & (!msb) ;
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  /* 
   * msbx, msby : MSB of x,y 
   * large : 0x7fffffff
   * absx,absy : x,y except MSB
   * xeqy, xpym, xmyp : x=y? x+y-? x-y+?
   */

  int msbx = (x>>31)&0x1;
  int msby = (y>>31)&0x1;
  int large = ~(0x1<<31);
  int absx = large&x;
  int absy = large&y;
  int xeqy = !(x^y); // 1 if x=y, 0 otherwise

  int xpym = msby & (!msbx) ;
  int xmyp = msbx & (!msby) ;

  // must check y - x => 0  then 1
  // check MSB of y-x. 
  // negation of MSB of (y-x) 
  int yminusx = absy + (~absx + 1); // |y| - |x| when same sign
  int msb = (yminusx>>31)&(0x1); // |y| - |x| <0 then 1, otherwise 0
  
  return (xeqy | xmyp | !( (msb) | ((xpym|xmyp)) )  );
}
/*
 * ilog2 - return floor(log base 2 of x), where x > 0
 *   Example: ilog2(16) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 90
 *   Rating: 4
 */
int ilog2(int x) {
/*  log2(x)
	as binary bit : log(x) ranges 0~31 for int x
	0~31, therefore 5 bits
	
	b0 : is larger then 2^4?
	b1 : is larger then (2^4*b0) + 2^3?
	b2 : is larger then (2^4*b0 + 2^3*b1) + 2^2?
	b3 : is larger then (2^4*b0 + 2^3*b1 + 2^2*b2) + 2^1?
	b4 : is larger then (2^4*b0 + 2^3*b1 + 2^2*b2 + 2^1*b1) + 2^0?

*/
  int x1 = x;
  int b0 = !((x1>>16) & ((0xFF<<8) + 0xFF));	// upper 16-bit is all zero? 1 if allzero, 0 otherwise
  int b1 = !((x1>> (((!b0)<<4)+8)) & (0xFF));
  int b2 = !((x1>> (((!b0)<<4) + ((!b1)<<3) + 4) ) & 0xF);
  int b3 = !((x1>> (((!b0)<<4) + ((!b1)<<3) + ((!b2)<<2) + 2))  & (0x3));
  int b4 = !((x1>> (((!b0)<<4) + ((!b1)<<3) + ((!b2)<<2) + ((!b3)<<1) +1)) & 0x1);

  return ((!b0)<<4) + ((!b1)<<3) + ((!b2)<<2) + ((!b3)<<1) + !b4 ;
}
/* 
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
 unsigned expn = (0xFF)&(uf>>23);

 if( (!(0xFF^expn)) &  !(!(uf<<9)) ) return uf; // if expn = 0xFF and frac !=0
 return (1<<31)^uf;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
  int Xb=x;
  int msb=x>>31; 
  int exp = 0;
  int expN = 157;
  int large = 0x1<<30;
  int frac,frac2;
 // int mask = ~(0xFF<<23 | (0x1<<31) ); // 0 0000 0000 1111 ... 1111
  int mask2 = ~(0xFE<<24); // 0000 0001 1111 ... 1111
  int guard, round, sticky;
  int tmp;
  // extract uppermost 24 bits
 
  if(!x){
   return 0;
  }
  if(x==(0x1<<31)){
    return 0xCF<<24;
  }
  if(msb) Xb = -x;
  while( !((large>>exp)&Xb) ) { 
	exp = exp +1;
        if(exp==31)   break;
  }
//exp is number of consecutive 0's except for sign bit. 
//  printf("TESTCASE 0x%x -- expN = 0x%x, exp=%d ",x,expN-exp,exp);
  expN = expN - exp; // exponent bit confirmed
  frac2 = mask2 & ((Xb<<exp)>>5);
  frac = frac2>>2;
//  guard = (frac2 & 0x2)>>1;
 // tmp = (frac2 & 0x3);
  
  guard = (frac2 & 0x4)>>2;
  round = (frac2 & 0x2)>>1;
  sticky = !(!(Xb<<exp<<26));

  // GRX = 011 110 111
  if(round && (guard || sticky)) frac = frac+1;
  
//  printf("\t G=%d, R=%d, X=%d\n",guard,round,sticky);
//  printf("frac = 0x%x\n",frac);
  
  return (msb<<31) + (expN<<23) + frac;
}
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
  int exp = (uf>>23) & 0xFF;
  int msb = uf>>31;
  int rest = uf<<9>>9;

/* 5 cases
 * DENORM (EXP=0x0, rest!=0)
 * ZERO (EXP=0x0, rest=0)
 * NORM (EXP!=0x0 0xFF)
 * INFINITY (EXP=0xFF, rest=0)
 * NaN (EXP=0xFF, rest!=0)
 */


  if( !(uf<<1) | (!(exp^0xFF))){ // case ZERO,NaN,INFINITY
	return uf;
  }else if( !exp ){ // CASE DENORM
//	printf("msb=0x%x\n",msb);
	return (msb<<31)+(exp<<23)+(rest<<1);
  }else{
	  return (msb<<31) + ((exp+1)<<23) + (rest);
  }

  return 0;
}
