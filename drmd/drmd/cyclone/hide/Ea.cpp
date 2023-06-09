
#include "app.h"

// some ops use non-standard cycle counts for EAs, so are listed here.
// all constants borrowed from the MUSASHI core by Karl Stenerud.

/* Extra cycles for JMP instruction (000, 010) */
int g_jmp_cycle_table[8] =
{
	 4, /* EA_MODE_AI   */
	 6, /* EA_MODE_DI   */
	10, /* EA_MODE_IX   */
	 6, /* EA_MODE_AW   */
	 8, /* EA_MODE_AL   */
	 6, /* EA_MODE_PCDI */
	10, /* EA_MODE_PCIX */
	 0, /* EA_MODE_I    */
};

/* Extra cycles for JSR instruction (000, 010) */
int g_jsr_cycle_table[8] =
{
	 4, /* EA_MODE_AI   */
	 6, /* EA_MODE_DI   */
	10, /* EA_MODE_IX   */
	 6, /* EA_MODE_AW   */
	 8, /* EA_MODE_AL   */
	 6, /* EA_MODE_PCDI */
	10, /* EA_MODE_PCIX */
	 0, /* EA_MODE_I    */
};

/* Extra cycles for LEA instruction (000, 010) */
int g_lea_cycle_table[8] =
{
	 4, /* EA_MODE_AI   */
	 8, /* EA_MODE_DI   */
	12, /* EA_MODE_IX   */
	 8, /* EA_MODE_AW   */
	12, /* EA_MODE_AL   */
	 8, /* EA_MODE_PCDI */
	12, /* EA_MODE_PCIX */
	 0, /* EA_MODE_I    */
};

/* Extra cycles for PEA instruction (000, 010) */
int g_pea_cycle_table[8] =
{
	 6, /* EA_MODE_AI   */
	10, /* EA_MODE_DI   */
	14, /* EA_MODE_IX   */
	10, /* EA_MODE_AW   */
	14, /* EA_MODE_AL   */
	10, /* EA_MODE_PCDI */
	14, /* EA_MODE_PCIX */
	 0, /* EA_MODE_I    */
};

/* Extra cycles for MOVEM instruction (000, 010) */
int g_movem_cycle_table[8] =
{
	 0, /* EA_MODE_AI   */
	 4, /* EA_MODE_DI   */
	 6, /* EA_MODE_IX   */
	 4, /* EA_MODE_AW   */
	 8, /* EA_MODE_AL   */
	 0, /* EA_MODE_PCDI */
	 0, /* EA_MODE_PCIX */
	 0, /* EA_MODE_I    */
};

// add nonstandard EA
int Ea_add_ns(int *tab, int ea)
{
	if(ea<0x10)			return 0;
	if((ea&0x38)==0x10) return tab[0]; // (An) (ai)
	if(ea<0x28)			return 0;
	if(ea<0x30)			return tab[1]; // ($nn,An) (di)
	if(ea<0x38)			return tab[2]; // ($nn,An,Rn) (ix)
	if(ea==0x38)		return tab[3]; // (aw)
	if(ea==0x39)		return tab[4]; // (al)
	if(ea==0x3a)		return tab[5]; // ($nn,PC) (pcdi)
	if(ea==0x3b)		return tab[6]; // ($nn,pc,Rn) (pcix)
	if(ea==0x3c)		return tab[7]; // #$nnnn (i)
	return 0;
}


// ---------------------------------------------------------------------------
// Gets the offset of a register for an ea, and puts it in 'r'
// Shifted left by 'shift'
// Doesn't trash anything
static int EaCalcReg(int r,int ea,int mask,int forceor,int shift,int noshift=0)
{
  int i=0,low=0,needor=0;
  int lsl=0;

  for (i=mask|0x8000; (i&1)==0; i>>=1) low++; // Find out how high up the EA mask is
  mask&=0xf<<low; // This is the max we can do

  if (ea>=8) needor=1; // Need to OR to access A0-7

  if ((mask>>low)&8) if (ea&8) needor=0; // Ah - no we don't actually need to or, since the bit is high in r8

  if (forceor) needor=1; // Special case for 0x30-0x38 EAs ;)

  ot("  and r%d,r8,#0x%.4x\n",r,mask);
  if (needor) ot("  orr r%d,r%d,#0x%x ;@ A0-7\n",r,r,8<<low);

  // Find out amount to shift left:
  lsl=shift-low;

  if (lsl&&!noshift)
  {
    ot("  mov r%d,r%d,",r,r);
    if (lsl>0) ot("lsl #%d\n", lsl);
    else       ot("lsr #%d\n",-lsl);
  }

  return 0;
}

// EaCalc - ARM Register 'a' = Effective Address
// Trashes r0,r2 and r3
// size values 0, 1, 2 ~ byte, word, long
int EaCalc(int a,int mask,int ea,int size,int top)
{
  char text[32]="";
  int func=0;

  DisaPc=2; DisaGetEa(text,ea,size); // Get text version of the effective address
  func=0x68+(size<<2); // Get correct read handler

  if (ea<0x10)
  {
    int noshift=0;
    if (size>=2||(size==0&&top)) noshift=1; // Saves one opcode

    ot(";@ EaCalc : Get register index into r%d:\n",a);

    EaCalcReg(a,ea,mask,0,2,noshift);
    return 0;
  }
  
  ot(";@ EaCalc : Get '%s' into r%d:\n",text,a);
  // (An), (An)+, -(An)
  if (ea<0x28)
  {
    int step=1<<size, strr=a;
	int low=0,lsl,i;

    if ((ea&7)==7 && step<2) step=2; // move.b (a7)+ or -(a7) steps by 2 not 1

    EaCalcReg(2,ea,mask,0,0,1);
	if(mask)
      for (i=mask|0x8000; (i&1)==0; i>>=1) low++; // Find out how high up the EA mask is
	lsl=2-low; // Having a lsl #x here saves one opcode
    if      (lsl>=0) ot("  ldr r%d,[r7,r2,lsl #%i]\n",a,lsl);
    else if (lsl<0)  ot("  ldr r%d,[r7,r2,lsr #%i]\n",a,-lsl);

    if ((ea&0x38)==0x18) // (An)+
	{
      ot("  add r3,r%d,#%d ;@ Post-increment An\n",a,step);
	  strr=3;
	}

    if ((ea&0x38)==0x20) // -(An)
      ot("  sub r%d,r%d,#%d ;@ Pre-decrement An\n",a,a,step);

    if ((ea&0x38)==0x18||(ea&0x38)==0x20)
	{
      if      (lsl>=0) ot("  str r%d,[r7,r2,lsl #%i]\n",strr,lsl);
      else if (lsl<0)  ot("  str r%d,[r7,r2,lsr #%i]\n",strr,-lsl);
    }

    if ((ea&0x38)==0x20) Cycles+=size<2 ? 6:10; // -(An) Extra cycles
    else                 Cycles+=size<2 ? 4:8;  // (An),(An)+ Extra cycles
    return 0;
  }

  if (ea<0x30) // ($nn,An) (di)
  {
    EaCalcReg(2,8,mask,0,0);
    ot("  ldrsh r0,[r4],#2 ;@ Fetch offset\n");
    ot("  ldr r2,[r7,r2,lsl #2]\n");
    ot("  add r%d,r0,r2 ;@ Add on offset\n",a);
    Cycles+=size<2 ? 8:12; // Extra cycles
    return 0;
  }

  if (ea<0x38) // ($nn,An,Rn) (ix)
  {
    ot(";@ Get extension word into r3:\n");
    ot("  ldrh r3,[r4],#2 ;@ ($Disp,PC,Rn)\n");
    ot("  mov r2,r3,lsr #10\n");
    ot("  tst r3,#0x0800 ;@ Is Rn Word or Long\n");
    ot("  and r2,r2,#0x3c ;@ r2=Index of Rn\n");
    ot("  ldreqsh r2,[r7,r2] ;@ r2=Rn.w\n");
    ot("  ldrne   r2,[r7,r2] ;@ r2=Rn.l\n");
    ot("  mov r0,r3,asl #24 ;@ r0=Get 8-bit signed Disp\n");
    ot("  add r3,r2,r0,asr #24 ;@ r3=Disp+Rn\n");

    EaCalcReg(2,8,mask,1,0);
    ot("  ldr r2,[r7,r2,lsl #2]\n");
    ot("  add r%d,r2,r3 ;@ r%d=Disp+An+Rn\n",a,a);
    Cycles+=size<2 ? 10:14; // Extra cycles
    return 0;
  }

  if (ea==0x38) // (aw)
  {
    ot("  ldrsh r%d,[r4],#2 ;@ Fetch Absolute Short address\n",a);
    Cycles+=size<2 ? 8:12; // Extra cycles
    return 0;
  }

  if (ea==0x39) // (al)
  {
    ot("  ldrh r2,[r4],#2 ;@ Fetch Absolute Long address\n");
    ot("  ldrh r0,[r4],#2\n");
    ot("  orr r%d,r0,r2,lsl #16\n",a);
    Cycles+=size<2 ? 12:16; // Extra cycles
    return 0;
  }

  if (ea==0x3a) // ($nn,PC) (pcdi)
  {
    ot("  ldr r0,[r7,#0x60] ;@ Get Memory base\n");
    ot("  sub r0,r4,r0 ;@ Real PC\n");
    ot("  ldrsh r2,[r4],#2 ;@ Fetch extension\n");
    ot("  mov r0,r0,lsl #8\n");
    ot("  add r%d,r2,r0,asr #8 ;@ ($nn,PC)\n",a);
    Cycles+=size<2 ? 8:12; // Extra cycles
    return 0;
  }

  if (ea==0x3b) // ($nn,pc,Rn) (pcix)
  {
    ot("  ldr r0,[r7,#0x60] ;@ Get Memory base\n");
    ot("  ldrh r3,[r4] ;@ Get extension word\n");
    ot("  sub r0,r4,r0 ;@ r0=PC\n");
    ot("  add r4,r4,#2\n");
    ot("  mov r0,r0,asl #8 ;@ use only 24bits of PC\n");
    ot("  mov r2,r3,lsr #10\n");
    ot("  tst r3,#0x0800 ;@ Is Rn Word or Long\n");
    ot("  and r2,r2,#0x3c ;@ r2=Index of Rn\n");
    ot("  ldreqsh r2,[r7,r2] ;@ r2=Rn.w\n");
    ot("  ldrne   r2,[r7,r2] ;@ r2=Rn.l\n");
    ot("  mov r3,r3,asl #24 ;@ r3=Get 8-bit signed Disp\n");
    ot("  add r2,r2,r3,asr #24 ;@ r2=Disp+Rn\n");
    ot("  add r%d,r2,r0,asr #8 ;@ r%d=Disp+PC+Rn\n",a,a);
    Cycles+=size<2 ? 10:14; // Extra cycles
    return 0;
  }

  if (ea==0x3c) // #$nnnn (i)
  {
    if (size<2)
    {
      ot("  ldr%s r%d,[r4],#2 ;@ Fetch immediate value\n",Sarm[size&3],a);
      Cycles+=4; // Extra cycles
      return 0;
    }

    ot("  ldrh r2,[r4],#2 ;@ Fetch immediate value\n");
    ot("  ldrh r0,[r4],#2\n");
    ot("  orr r%d,r0,r2,lsl #16\n",a);
    Cycles+=8; // Extra cycles
    return 0;
  }

  return 1;
}

// ---------------------------------------------------------------------------
// Read effective address in (ARM Register 'a') to ARM register 'v'
// 'a' and 'v' can be anything but 0 is generally best (for both)
// If (ea<0x10) nothing is trashed, else r0-r3 is trashed
// If 'top' is given, the ARM register v shifted to the top, e.g. 0xc000 -> 0xc0000000
// Otherwise the ARM register v is sign extended, e.g. 0xc000 -> 0xffffc000

int EaRead(int a,int v,int ea,int size,int mask,int top)
{
  char text[32]="";
  int shift=0;
  
  shift=32-(8<<size);

  DisaPc=2; DisaGetEa(text,ea,size); // Get text version of the effective address

  if (ea<0x10)
  {
    int lsl=0,low=0,i;
    if (size>=2||(size==0&&top)) {
	  if(mask)
        for (i=mask|0x8000; (i&1)==0; i>>=1) low++; // Find out how high up the EA mask is
	  lsl=2-low; // Having a lsl #2 here saves one opcode
	}

    ot(";@ EaRead : Read register[r%d] into r%d:\n",a,v);

    if      (lsl>0) ot("  ldr%s r%d,[r7,r%d,lsl #%i]\n",Narm[size&3],v,a,lsl);
    else if (lsl<0) ot("  ldr%s r%d,[r7,r%d,lsr #%i]\n",Narm[size&3],v,a,-lsl);
    else            ot("  ldr%s r%d,[r7,r%d]\n",Sarm[size&3],v,a);

    if (top && shift) ot("  mov r%d,r%d,asl #%d\n",v,v,shift);

    ot("\n"); return 0;
  }

  ot(";@ EaRead : Read '%s' (address in r%d) into r%d:\n",text,a,v);

  if (ea==0x3c)
  {
    int asl=0;

    if (top) asl=shift;

    if (v!=a || asl) ot("  mov r%d,r%d,asl #%d\n",v,a,asl);
    ot("\n"); return 0;
  }

  if (a!=0) ot("  mov r0,r%d\n",a);

  if (ea>=0x3a && ea<=0x3b) MemHandler(2,size); // Fetch
  else                      MemHandler(0,size); // Read

  if (v!=0 || shift) {
    if (shift) ot("  mov r%d,r0,asl #%d\n",v,shift);
    else       ot("  mov r%d,r0\n",v);
  }
  if (top==0 && shift) ot("  mov r%d,r%d,asr #%d\n",v,v,shift);

  ot("\n"); return 0;
}

// Return 1 if we can read this ea
int EaCanRead(int ea,int size)
{
  if (size<0)
  {
    // LEA:
    // These don't make sense?:
    if (ea< 0x10) return 0; // Register
    if (ea==0x3c) return 0; // Immediate
    if (ea>=0x18 && ea<0x28) return 0; // Pre/Post inc/dec An
  }

  if (ea<=0x3c) return 1;
  return 0;
}

// ---------------------------------------------------------------------------
// Write effective address (ARM Register 'a') with ARM register 'v'
// Trashes r0-r3,r12,lr; 'a' can be 0 or 2+, 'v' can be 1 or higher
// If a==0 and v==1 it's faster though.
int EaWrite(int a,int v,int ea,int size,int mask,int top)
{
  char text[32]="";
  int shift=0;

  if(a == 1) { printf("Error! EaWrite a==1 !\n"); return 1; }

  if (top) shift=32-(8<<size);

  DisaPc=2; DisaGetEa(text,ea,size); // Get text version of the effective address

  if (ea<0x10)
  {
    int lsl=0,low=0,i;
    if (size>=2||(size==0&&top)) {
	  if(mask)
        for (i=mask|0x8000; (i&1)==0; i>>=1) low++; // Find out how high up the EA mask is
	  lsl=2-low; // Having a lsl #x here saves one opcode
	}

    ot(";@ EaWrite: r%d into register[r%d]:\n",v,a);
    if (shift)  ot("  mov r%d,r%d,asr #%d\n",v,v,shift);

    if      (lsl>0) ot("  str%s r%d,[r7,r%d,lsl #%i]\n",Narm[size&3],v,a,lsl);
    else if (lsl<0) ot("  str%s r%d,[r7,r%d,lsr #%i]\n",Narm[size&3],v,a,-lsl);
    else            ot("  str%s r%d,[r7,r%d]\n",Narm[size&3],v,a);

    ot("\n"); return 0;
  }

  ot(";@ EaWrite: Write r%d into '%s' (address in r%d):\n",v,text,a);

  if (ea==0x3c) { ot("Error! Write EA=0x%x\n\n",ea); return 1; }

  if (a!=0 && v!=0)  ot("  mov r0,r%d\n",a);
  if (v!=1 || shift) ot("  mov r1,r%d,asr #%d\n",v,shift);
  if (a!=0 && v==0)  ot("  mov r0,r%d\n",a);

  MemHandler(1,size); // Call write handler

  ot("\n"); return 0;
}

// Return 1 if we can write this ea
int EaCanWrite(int ea)
{
  if (ea<=0x39) return 1; // 3b?
  return 0;
}
// ---------------------------------------------------------------------------

// Return 1 if EA is An reg
int EaAn(int ea)
{
  if((ea&0x38)==8) return 1;
  return 0;
}

