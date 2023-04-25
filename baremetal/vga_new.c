#include "BOARD/board.h"
#include <sys_devcon.h>
#include <sys/kmem.h>

// https://tomverbeure.github.io/video_timings_calculator

// clang-format off

#define MPX                 2

#define VGA_WIDTH           (799 << MPX)
#define VGA_VISIBLE_WIDTH   (640 << MPX)
#define VGA_HFP             (16 << MPX)
#define VGA_HB              (96 << MPX)
#define VGA_HBP             (48 << MPX)

#define VGA_HEIGHT          600
#define VGA_VISIBLE_HEIGHT  480
#define VGA_VFP             10
#define VGA_VB              2
#define VGA_VBP             33

#define H_OFFSET            100
#define V_OFFSET            30

extern uint8_t __attribute__((coherent, aligned(16))) 
frame_buffer[192][280 * 4 + 1]; // pixel * 4 + 1 column blank

// B4 VBLANK
#define V_PIN               (4)
#define V_0()               LATBCLR = 1 << V_PIN
#define V_1()               LATBSET = 1 << V_PIN
#define V_INIT()            (TRISBCLR = 1 << V_PIN, V_1())
// #define MUX_OC1()        RPE8R = 12 // E8 HBLANK

// clang-format on

volatile unsigned int current_vga_y = 0;
static int v_line = 0, y_line = 0;

//#define DMA_ISR
#ifdef DMA_ISR
volatile unsigned vga_dma_busy;
void __attribute__((vector(_DMA0_VECTOR), interrupt(IPL7SRS))) DMA0_Handler()
{
    IFS4CLR = _IFS4_DMA0IF_MASK;
    IEC4CLR = _IEC4_DMA0IE_MASK;
    vga_dma_busy = 0;
}
#endif

void __attribute__((vector(_OUTPUT_COMPARE_1_VECTOR), interrupt(IPL6SRS), nomips16)) OC1Handler(void)
{
    IFS0CLR = _IFS0_OC1IF_MASK;

    switch (++v_line)
    {
    case VGA_VISIBLE_HEIGHT + VGA_VFP:
        V_0(); // V blank begin
        break;
    case VGA_VISIBLE_HEIGHT + VGA_VFP + VGA_VB:
        V_1(); // V blank end
        break;
    case VGA_VISIBLE_HEIGHT + VGA_VFP + VGA_VB + VGA_VBP: // 525
        v_line = 0;
        break;
    default:
        current_vga_y = y_line;
        DCH0SSA = KVA_TO_PA(&frame_buffer[y_line][0]); // init DMA line
        DCH0CONSET = _DCH0CON_CHEN_MASK;
#ifdef DMA_ISR
        DCH0INT = _DCH0INT_CHBCIE_MASK;
        IEC4SET = _IEC4_DMA0IE_MASK;
#endif
        break;
    }
}

void __attribute__((vector(_OUTPUT_COMPARE_2_VECTOR), interrupt(IPL6SRS), nomips16)) OC2Handler(void)
{
    IFS0CLR = _IFS0_OC2IF_MASK;

    if (v_line >= V_OFFSET) // vertical center offset
    {
        y_line = (v_line - V_OFFSET) >> 1; // draw line twice
        if (y_line < 192)
        {
#ifdef DMA_ISR
            vga_dma_busy = 1;
#endif
            DCH0ECONSET = _DCH0ECON_CFORCE_MASK; // start DMA
            // while (dma_busy) continue; // blocked
        }
    }
}

// #pragma GCC optimize("O0")
void vga_init()
{
    printf("FB: %p\n", frame_buffer);

    TRISECLR = 134; // TODO Enable pixel ports LATE[0-7]

    V_INIT(); // H init in board.c

    IEC4CLR = _IEC4_DMA0IE_MASK;
    IFS4CLR = _IFS4_DMA0IF_MASK;
    IPC33SET = 7 << _IPC33_DMA0IP_POSITION;
    DCH0CON = 0; // prio only
    DCH0ECON = 0;
    DCH0INT = 0;
    DCH0DSA = KVA_TO_PA(&LATE);
    DCH0SSA = KVA_TO_PA(frame_buffer);
    DCH0DSIZ = 1;
    DCH0CSIZ = 280 * 4 + 1;
    DCH0SSIZ = 280 * 4 + 1;
#ifdef DMA_ISR
    DCH0INT = _DCH0INT_CHBCIE_MASK;
#endif
    DCH0CONSET = _DCH0CON_CHEN_MASK;
    DMACONSET = 0x8000;

    T3CON = (0 << _T3CON_TCKPS_POSITION); // 100/50/25 MHz
    PR3 = VGA_WIDTH;                      // 800 px per H Line
    TMR3 = 0;

    IPC1SET = 6 << _IPC1_OC1IP_POSITION;
    IFS0CLR = _IFS0_OC1IF_MASK;
    IEC0SET = _IEC0_OC1IE_MASK; // 1/2 H Blank

    IPC3SET = 6 << _IPC3_OC2IP_POSITION;
    IFS0CLR = _IFS0_OC2IF_MASK;
    IEC0SET = _IEC0_OC2IE_MASK;

    OC1CON = OC2CON = 13; // @ Timer 3

    OC1R = (VGA_HB >> 1);                                 // half H blank
    OC1RS = OC1R + VGA_HBP + VGA_VISIBLE_WIDTH + VGA_HFP; //
    OC2R = OC1R - 1;                                      // less OC1R
    OC2RS = OC1R + VGA_HBP + H_OFFSET;                    // DMA start + H offset

    OC1CONSET = _OC1CON_ON_MASK;
    OC2CONSET = _OC2CON_ON_MASK;
    T3CONSET = _T3CON_ON_MASK;
}

/*

H LINE GENERATOR ( 800 / 640 px )

        96        48  | 640 DMA DATA  | 16
---             -------------------------
   |           |      .               .  |
   |           |      .               .  |
   |  48   48  |      .               .  |
   b-----0-----u      |                  d---0---
ISR     T=0         OC2(V)             OC1(H)

*/
