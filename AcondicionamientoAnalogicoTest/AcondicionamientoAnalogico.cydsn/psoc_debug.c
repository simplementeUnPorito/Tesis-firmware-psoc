#include "psoc_debug.h"

#ifdef CY_UART_PC_H

void psoc_debug_start(void)
{
    PC_Start();
}

void psoc_debug_print(const char *msg)
{
    PC_PutString(msg);
}

/* Conversion manual a decimal, sin sprintf/stdio: este proyecto no tiene
 * syscalls (_sbrk/_write) provistos, asi que cualquier camino de stdio que
 * toque el heap (incluso sprintf con un buffer local) falla el link con
 * "undefined reference to '_sbrk'". */
static uint8 u32_to_dec(uint32 value, char *out)
{
    char tmp[10];
    uint8 n = 0u;
    uint8 i;
    if (value == 0UL) {
        out[0] = '0';
        return 1u;
    }
    while (value > 0UL && n < (uint8)sizeof(tmp)) {
        tmp[n] = (char)('0' + (value % 10UL));
        value /= 10UL;
        n++;
    }
    for (i = 0u; i < n; i++) {
        out[i] = tmp[n - 1u - i];
    }
    return n;
}

static const char HEX_DIGITS[16] = "0123456789ABCDEF";

void psoc_debug_print_u32(const char *prefix, uint32 value)
{
    char buf[16];
    uint8 n = u32_to_dec(value, buf);
    PC_PutString(prefix);
    buf[n] = '\0';
    PC_PutString(buf);
    PC_PutString("\r\n");
}

void psoc_debug_print_i32(const char *prefix, int32 value)
{
    char buf[16];
    uint8 n;
    uint32 magnitude;
    PC_PutString(prefix);
    if (value < 0) {
        PC_PutChar('-');
        magnitude = (uint32)(-value);
    } else {
        magnitude = (uint32)value;
    }
    n = u32_to_dec(magnitude, buf);
    buf[n] = '\0';
    PC_PutString(buf);
    PC_PutString("\r\n");
}

void psoc_debug_print_evt(uint8 event, uint8 value, uint8 state)
{
    char hex[3];
    char dec[4];
    uint8 n;

    hex[0] = HEX_DIGITS[(event >> 4) & 0x0Fu];
    hex[1] = HEX_DIGITS[event & 0x0Fu];
    hex[2] = '\0';

    PC_PutString("EVT=0x");
    PC_PutString(hex);
    PC_PutString(" val=");
    n = u32_to_dec(value, dec);
    dec[n] = '\0';
    PC_PutString(dec);
    PC_PutString(" state=");
    n = u32_to_dec(state, dec);
    dec[n] = '\0';
    PC_PutString(dec);
    PC_PutString("\r\n");
}

#else

void psoc_debug_start(void) { }
void psoc_debug_print(const char *msg) { (void)msg; }
void psoc_debug_print_u32(const char *prefix, uint32 value) { (void)prefix; (void)value; }
void psoc_debug_print_i32(const char *prefix, int32 value) { (void)prefix; (void)value; }
void psoc_debug_print_evt(uint8 event, uint8 value, uint8 state) { (void)event; (void)value; (void)state; }

#endif
