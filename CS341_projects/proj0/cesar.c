#include "cesar.h"

unsigned char cesar(char payload, int opcode, int shift){
    char res = payload;

    if(payload<=122 && 97<=payload){
        res += (shift*(1-2*opcode))%26;
        if(res>122) res-=26;
        if(res<97) res+=26;
        return res;
    }else if(payload<=90 && 65<=payload){
        return cesar(payload+32, opcode, shift);
    }
    else {
        return res;
    }
}
