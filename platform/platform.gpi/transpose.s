
	.data


       .text
       .global transpose_320x240_to_240x320
  
  
transpose_320x240_to_240x320:
     stmfd sp!,{r3-r12,lr}
     ;@ r0 = src
     ;@ r1 = dst
     
     add r1,r1,#480
     sub r1,r1,#4
     add r2,r0,#640
     ;@ r10,r11,r12,lr free

     ;@ dst pixels are seperated by stride
     ;@ 
     ldr r7,=#0xFFFF
     mov r12,r1
     mov r11,#120
2:     
     mov r10,#80
1:
     ldmia r0!,{r3,r4}
     ldmia r2!,{r5,r6}
     
     mov r8,r3,lsr#16
     mov r9,r5,lsr#16

     bic r5,r5,r7,lsl#16
     orr r5,r5,r3,lsl#16
     str r5,[r1],#480

     orr r9,r9,r8,lsl#16
     str r9,[r1],#480
     
     mov r8,r4,lsr#16
     mov r9,r6,lsr#16
     
     bic r6,r6,r7,lsl#16
     orr r6,r6,r4,lsl#16
     str r6,[r1],#480
     
     orr r9,r9,r8,lsl#16
     str r9,[r1],#480

     subs r10,r10,#1
     bne 1b
     
     sub r12,r12,#4
     mov r1,r12
     add r0,r0,#640
     add r2,r2,#640
     subs r11,r11,#1
     bne 2b
     
     ldmfd sp!,{r3-r12,pc}
     