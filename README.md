# CS230_projects

##  Lab 1. Doubly Linked Lists
### - Introductory lab on C pointers


##  Lab 2. Data Lab
### - Bitwise operations on (unsigned) integers
### - Float operations


## Lab 3. Bomb Lab
### - Defuse your binary bomb


## Lab 4. Attack Lab
### - CTARGET(buffer overflow attacks) : phase1 ~ phase3
### - RTARGET(return oriented programming) : phase4 ~ phase5
      phase2,phase4 is missing because it's been overwritten by phase3,phase5
      (They're easy anyway)


## Lab 5. Shell Lab
### - tsh.c : Implement your own tiny shell
      Passed all 16 traces


## Lab 6. Malloc Lab
### - mm.c : Malloc package with Segregated Free Lists
      Passing all traces (Util 44/60 + Thru 1/40 = 45/100)
      Throughput problem for mm_realloc(), which was simply implemented with mm_free() and mm_malloc().
      

## Lab 7. Proxy Lab
### - proxy.c : Incomplete code(transferring binary files)
      ** Cacheing is not dealt in this project **
      basic : 3 PASSES + 2 FAILS (binary transfer) = 24/40
      concurrency : 15/15
      (cacheing) : 0/0
      total : 39/55
