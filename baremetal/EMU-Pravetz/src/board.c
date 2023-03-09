
#include <xc.h>
#include <sys/attribs.h>
#include <stdio.h>
#include "user_config.h"

#define mCTClearIntFlag() (IFS0CLR = _IFS0_CTIF_MASK)
#define mCTIntEnable(enable) (IEC0CLR = _IEC0_CTIE_MASK, IEC0SET = ((enable) << _IEC0_CTIE_POSITION))
#define mCTSetIntPriority(priority) (IPC0CLR = _IPC0_CTIP_MASK, IPC0SET = ((priority) << _IPC0_CTIP_POSITION))
#define mCTSetIntSubPriority(subPriority) (IPC0CLR = _IPC0_CTIS_MASK, IPC0SET = ((subPriority) << _IPC0_CTIS_POSITION))

static volatile uint32_t gMillis = 0;
static volatile uint32_t gOldCoreTime = 0;

void __ISR(_CORE_TIMER_VECTOR, IPL4AUTO) CoreTimerHandler(void)
{
    uint32_t current = _CP0_GET_COUNT();
    uint32_t elapsed = current - gOldCoreTime;
    uint32_t increment = elapsed / CORE_TIMER_INTERRUPT_TICKS;
    gMillis += increment;
    _CP0_SET_COMPARE(current + CORE_TIMER_INTERRUPT_TICKS);
    mCTClearIntFlag();
    gOldCoreTime = current;
}

uint32_t millis(void) { return gMillis; }

uint32_t micros(void)
{
#if 1
    asm volatile("di");
    uint32_t result = millis() * 1000;
    uint32_t current = _CP0_GET_COUNT();
    current -= gOldCoreTime;
    current += CORE_TIMER_TICKS_PER_MICROSECOND / 2;
    current /= CORE_TIMER_TICKS_PER_MICROSECOND;
    asm volatile("ei");
    return (result + current);
#else
    return millis() * 1000;
#endif
}

void delay(uint32_t ms)
{
    uint32_t startTime = millis();
    while ((millis() - startTime) < ms)
        continue;
}

static inline void uart_init(void)
{
    RPD15R = 1;
    U1STA = U1MODE = 0;
    U1BRG = (((SYS_CLK_BUS_PERIPHERAL_1 >> 4) + (UART_BRG >> 1)) / UART_BRG) - 1;
    U1STA = _U1STA_UTXEN_MASK;
    U1MODE = _U1MODE_ON_MASK;
    __XC_UART = 1; // stdio - UART1
}

void spi_set_pins(void)
{
    SDI1R = 11;            // MISO_1
    RPD3R = 5;             // MOSI_1
    SPI1CONbits.MSSEN = 0; // disable chip select
}

void board_init(void)
{
    // Lock
    asm volatile("di");
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;

    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    ANSELF = 0;

    // Use multi vectored interrupts
    _CP0_BIS_CAUSE(0x00800000U);
    INTCONSET = _INTCON_MVEC_MASK;

    // Set up prefetch
    PRECONbits.PFMSECEN = 0; // Flash SEC Interrupt Enable (Do not generate an interrupt when the PFMSEC bit is set)
    PRECONbits.PREFEN = 3;   // Predictive Prefetch Enable (Enable predictive prefetch for any address)
    PRECONbits.PFMWS = 2;    // PFM Access Time Defined in Terms of SYSCLK Wait States (Two wait states)

    // Set up caching
    unsigned int cp0 = _mfc0(16, 0);
    cp0 &= ~0x07;
    cp0 |= 0b011; // K0 = Cacheable, non-coherent, write-back, write allocate
    _mtc0(16, 0, cp0);

    uart_init();

    // CORE TIMER 1 ms
    int ticks = CORE_TIMER_INTERRUPT_TICKS;
    asm volatile("mtc0   $0,$9");
    asm volatile("mtc0   %0,$11" : "+r"(ticks));
    _CP0_SET_COMPARE(ticks);
    mCTSetIntPriority(4);
    mCTSetIntSubPriority(0);
    mCTIntEnable(1);

    // Unlock
    SYSKEY = 0x33333333;
    asm volatile("ei");
    
    TRISECLR = 1 << LED;
    TRISGbits.TRISG12 = 1;
}

static unsigned int _excep_code;
static unsigned int _excep_addr;
static char *_cause_str;
static char *cause[] = {
    "Interrupt",
    "Undefined",
    "Undefined",
    "Undefined",
    "Load/Fetch address error",
    "Store address error",
    "Instruction bus error",
    "Data bus error",
    "Syscall",
    "Breakpoint",
    "Reserved instruction",
    "Coprocessor unusable",
    "Arithmetic overflow",
    "Trap",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

void _general_exception_handler(void)
{
    _excep_code = (_CP0_GET_CAUSE() & 0x0000007C) >> 2;
    _excep_addr = _CP0_GET_EPC();
    _cause_str = cause[_excep_code];
    printf("[TRAP] General Exception '%s' (cause = %d, addr = 0x%X)\n", _cause_str, _excep_code, _excep_addr);
    while (1)
        continue;
}