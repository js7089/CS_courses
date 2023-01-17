#ifndef __CESAR_H__
#define __CESAR_H__

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

unsigned char cesar(char payload, int opcode, int shift);

#endif
