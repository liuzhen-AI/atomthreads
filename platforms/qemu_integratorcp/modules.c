/*
 * Copyright (c) 2012, Natie van Rooyen. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. No personal names or organizations' names associated with the
 *    Atomthreads project may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ATOMTHREADS PROJECT AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "modules.h"
#include <stdio.h>
#include <stdarg.h>
#include "atomport.h"
#include "atomport-private.h"
#include "atom.h"
#include "atomport.h"
#include "types.h"

/** Imports required by C startup code */
extern unsigned long _end_text, _start_data, _end_data, _start_bss, _end_bss;
extern int main(void);

/** Board-specific registers */
ICP_TIMER_T * const board_timer_0 = (ICP_TIMER_T*)BOARD_BASE_ADDRESS_TIMER_0;
ICP_PIC_T *   const board_pic     = (ICP_PIC_T*)BOARD_BASE_ADDRESS_PIC;

/** TIMER0 clock speed (Hz) */
#define TIMER0_CLOCK_SPEED 		40000000


/**
 * \b dbg_format_msg
 *
 * Same as printf. 
 *
 */
void 
dbg_format_msg (char *format, ...)
{
    va_list         args;
    static char     msg[256] ;
    CRITICAL_STORE ;

    va_start (args, format) ;
    CRITICAL_START() ;
    vsnprintf ((char*)msg, 256, (char*)format, args) ;
    printf (msg) ;
    CRITICAL_END() ;

}


/**
 * \b _mainCRTStartup
 *
 * C startup code for environments without a suitable built-in one.
 * May be provided by the compiler toolchain in some cases.
 *
 */
extern void _mainCRTStartup (void) __attribute__((weak));
void _mainCRTStartup(void)
{
    unsigned long *src;
#ifdef ROM
    unsigned long *dst;
#endif

#ifdef ROM
    // Running from ROM: copy data section to RAM
    src = &_end_text;
    dst = &_start_data;
    while(dst < &_end_data)
        *(dst++) = *(src++);
#endif

    // Clear BSS
    src = &_start_bss;
    while(src < &_end_bss)
        *(src++) = 0;

    // Jump to main application entry point
    main();
}


/**
 * \b low_level_init
 *
 * Initializes the PIC and start the system timer tick intrerupt.
 *
 */
int
low_level_init (void)
{

    board_pic->IRQ_ENABLECLR = ICP_PIC_IRQ_TIMERINT0 ;
    board_timer_0->INTCLR = 1 ;
    board_pic->IRQ_ENABLESET |= ICP_PIC_IRQ_TIMERINT0 ;

	/* Set the timer to go off 100 times per second (input clock speed is 40MHz) */
    board_timer_0->LOAD = TIMER0_CLOCK_SPEED / SYSTEM_TICKS_PER_SEC ;
    board_timer_0->BGLOAD = TIMER0_CLOCK_SPEED / SYSTEM_TICKS_PER_SEC ;
    board_timer_0->CONTROL = ICP_TIMER_CONTROL_ENABLE |
                            ICP_TIMER_CONTROL_MODE |
                            ICP_TIMER_CONTROL_IE |
                            /*ICP_TIMER_CONTROL_PRESCALE_256 |*/
                            ICP_TIMER_CONTROL_TIMER_SIZE ;

    return 0 ;
}


/**
 * \b __context_preempt_handler
 *
 * System timer tic interupt handler.
 *
 */
void
__context_preempt_handler (void) 
{
    unsigned int status = board_pic->IRQ_STATUS ;

    if (status | ICP_PIC_IRQ_TIMERINT0) {
        
        atomIntEnter();

        /* Call the OS system tick handler */
        atomTimerTick();

        /* ack the interrupt */
        board_timer_0->INTCLR = 0x1 ;

        /* Call the interrupt exit routine */
        atomIntExit(TRUE);

    }


}

