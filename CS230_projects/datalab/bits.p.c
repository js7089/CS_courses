#line 173 "bits.c"
int bitAnd(int x, int y) {


  return ~((~x)|(~y));
#line 1 "<command-line>"
#include "/usr/include/stdc-predef.h"
#line 177 "bits.c"
}
#line 186
int getByte(int x, int n) {

  return (x>>(n<<3))&0xFF;
}
#line 198
int logicalShift(int x, int n) {
  int large=(  0x1 << 31);

  int cond=(  !((!n &(( x&large)>>31))&0x1))<<31;

  return (~((cond>>n)<<1)) &( x>>n);
#line 207
}
#line 215
int bitCount(int x) {
  int unit=  0x11;
  int unit_d=  unit | unit<<8 | unit<<16 | unit<<24;

  int x1=  x;
  int x2=  x>>1;
  int x3=  x>>2;
  int x4=  x>>3;
#line 227
  int sum=(  x1&unit_d) +( x2&unit_d) +( x3&unit_d) +( x4&unit_d);
#line 236
  return ((sum>>28)&(0xF)) +(( sum>>24)&(0xF)) +(( sum>>20)&(0xF)) +(( sum>>16)&(0xF)) +(( sum>>12)&(0xF)) +(( sum>>8)&(0xF)) +(( sum>>4)&0xF) +( sum&0xF);
}
#line 245
int bang(int x) {
#line 251
  int a1=(  x>>16) | x;
  int b=(  a1>>8) | a1;
  int c=(  b>>4) |( b);
  int d=(  c>>2) | c;
  int e=  ~(( d>>1) | d)   & 0x1;

  return e;
}
#line 265
int tmin(void) {

  return 0x1<<31;
}
#line 278
int fitsBits(int x, int n) {
#line 282
  int truncateV=  33+(~n);
#line 287
  return !((x<<truncateV>>truncateV)^x);

}
#line 298
int divpwr2(int x, int n) {
  int msb=  x>>31;
  int bias=(  0x1<<n)+(~0);
#line 306
  return (x+(bias&msb))>>n;
}
#line 315
int negate(int x) {

  return ~x+1;
}
#line 326
int isPositive(int x) {

  int msb=(  x>>31) &( 0x1);
  int iszero=  !x;
  return (!iszero) &( !msb);
}
#line 339
int isLessOrEqual(int x, int y) {
#line 347
  int msbx=(  x>>31)&0x1;
  int msby=(  y>>31)&0x1;
  int large=  ~(0x1<<31);
  int absx=  large&x;
  int absy=  large&y;
  int xeqy=  !(x^y);

  int xpym=  msby &( !msbx);
  int xmyp=  msbx &( !msby);
#line 360
  int yminusx=  absy +( ~absx + 1);
  int msb=(  yminusx>>31)&(0x1);

  return (xeqy | xmyp | !(( msb) |(  xpym|xmyp)));
}
#line 372
int ilog2(int x) {
#line 384
  int x1=  x;
  int b0=  !((x1>>16) &(( 0xFF<<8) + 0xFF));
  int b1=  !((x1>>((( !b0)<<4)+8)) &( 0xFF));
  int b2=  !((x1>>((( !b0)<<4) +(( !b1)<<3) + 4))  & 0xF);
  int b3=  !((x1>>((( !b0)<<4) +(( !b1)<<3) +(( !b2)<<2) + 2)) &( 0x3));
  int b4=  !((x1>>((( !b0)<<4) +(( !b1)<<3) +(( !b2)<<2) +(( !b3)<<1) +1)) & 0x1);

  return ((!b0)<<4) +(( !b1)<<3) +(( !b2)<<2) +(( !b3)<<1) + !b4;
}
#line 404
unsigned float_neg(unsigned uf) {
 unsigned expn=(  0xFF)&(uf>>23);

 if ((!(0xFF^expn)) & !(!(uf<<9)))  return uf;
 return (1<<31)^uf;
}
#line 419
unsigned float_i2f(int x) {
  int Xb=x;
  int msb=x>>31;
  int exp=  0;
  int expN=  157;
  int large=  0x1<<30;
  int frac;int frac2;

  int mask2=  ~(0xFE<<24);
  int guard;int round;int sticky;
  int tmp;


  if (!x) {
   return 0;
  }
  if (x==(0x1<<31)) {
    return 0xCF<<24;
  }
  if (msb) Xb= -x;
  while (!((large>>exp)&Xb))  {
 exp = exp +1;
        if (exp==31) break;
  }


  expN = expN - exp;
  frac2 = mask2 &(( Xb<<exp)>>5);
  frac = frac2>>2;
#line 451
  guard =( frac2 & 0x4)>>2;
  round =( frac2 & 0x2)>>1;
  sticky = !(!(Xb<<exp<<26));


  if (round&&( guard || sticky)) frac = frac+1;
#line 461
  return (msb<<31) +( expN<<23) + frac;
}
#line 474
unsigned float_twice(unsigned uf) {
  int exp=(  uf>>23) & 0xFF;
  int msb=  uf>>31;
  int rest=  uf<<9>>9;
#line 488
  if (!(uf<<1) |( !(exp^0xFF))) {
 return uf;
  }else if (!exp) {

 return (msb<<31)+(exp<<23)+(rest<<1);
  }else {
   return (msb<<31) +(( exp+1)<<23) +( rest);
  }

  return 0;
}
