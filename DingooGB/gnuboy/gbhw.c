#include <string.h>


#include "defs.h"
#include "cpu.h"
#include "gbhw.h"
#include "regs.h"
#include "lcd.h"
#include "mem.h"
#include "fastmem.h"


struct gbhw gbhw;



/*
 * hw_interrupt changes the virtual interrupt lines included in the
 * specified mask to the values the corresponding bits in i take, and
 * in doing so, raises the appropriate bit of R_IF for any interrupt
 * lines that transition from low to high.
 */

void gbhw_interrupt(byte i, byte mask)
{
	byte oldif = R_IF;
	i &= 0x1F & mask;
	R_IF |= i & (gbhw.ilines ^ i);

	/* FIXME - is this correct? not sure the docs understand... */
	if ((R_IF & (R_IF ^ oldif) & R_IE) && cpu.ime) cpu.halt = 0;
	/* if ((i & (gbhw.ilines ^ i) & R_IE) && cpu.ime) cpu.halt = 0; */
	/* if ((i & R_IE) && cpu.ime) cpu.halt = 0; */
	
	gbhw.ilines &= ~mask;
	gbhw.ilines |= i;
}


/*
 * gbhw_dma performs plain old memory-to-oam dma, the original dmg
 * dma. Although on the hardware it takes a good deal of time, the cpu
 * continues running during this mode of dma, so no special tricks to
 * stall the cpu are necessary.
 */

void gbhw_dma(byte b)
{
	int i;
	addr a;

	a = ((addr)b) << 8;
	for (i = 0; i < 160; i++, a++)
		lcd.oam.mem[i] = readb(a);
}



void gbhw_hdma_cmd(byte c)
{
	int cnt;
	addr sa;
	int da;

	/* Begin or cancel HDMA */
	if ((gbhw.hdma|c) & 0x80)
	{
		gbhw.hdma = c;
		R_HDMA5 = c & 0x7f;
		return;
	}
	
	/* Perform GDMA */
	sa = ((addr)R_HDMA1 << 8) | (R_HDMA2&0xf0);
	da = 0x8000 | ((int)(R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	cnt = ((int)c)+1;
	/* FIXME - this should use cpu time! */
	/*cpu_timers(102 * cnt);*/
	cnt <<= 4;
	while (cnt--)
		writeb(da++, readb(sa++));
	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5 = 0xFF;
}


void gbhw_hdma()
{
	int cnt;
	addr sa;
	int da;

	sa = ((addr)R_HDMA1 << 8) | (R_HDMA2&0xf0);
	da = 0x8000 | ((int)(R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	cnt = 16;
	while (cnt--)
		writeb(da++, readb(sa++));
	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5--;
	gbhw.hdma--;
}


/*
 * pad_refresh updates the P1 register from the pad states, generating
 * the appropriate interrupts (by quickly raising and lowering the
 * interrupt line) if a transition has been made.
 */

void pad_refresh()
{
	byte oldp1;
	oldp1 = R_P1;
	R_P1 &= 0x30;
	R_P1 |= 0xc0;
	if (!(R_P1 & 0x10))
		R_P1 |= (gbhw.pad & 0x0F);
	if (!(R_P1 & 0x20))
		R_P1 |= (gbhw.pad >> 4);
	R_P1 ^= 0x0F;
	if (oldp1 & ~R_P1 & 0x0F)
	{
		gbhw_interrupt(IF_PAD, IF_PAD);
		gbhw_interrupt(0, IF_PAD);
	}
}


/*
 * These simple functions just update the state of a button on the
 * pad.
 */

void pad_press(byte k)
{
	if (gbhw.pad & k)
		return;
	gbhw.pad |= k;
	pad_refresh();
}

void pad_release(byte k)
{
	if (!(gbhw.pad & k))
		return;
	gbhw.pad &= ~k;
	pad_refresh();
}

void pad_set(byte k, int st)
{
	st ? pad_press(k) : pad_release(k);
}

void gbhw_reset()
{
	gbhw.ilines = gbhw.pad = 0;

	memset(ram.hi, 0, sizeof ram.hi);

	R_P1 = 0xFF;
	R_LCDC = 0x91;
	R_BGP = 0xFC;
	R_OBP0 = 0xFF;
	R_OBP1 = 0xFF;
	R_SVBK = 0x01;
	R_HDMA5 = 0xFF;
	R_VBK = 0xFE;
}







