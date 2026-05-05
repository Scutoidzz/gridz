#include "py/mphal.h"

extern void python_term_write(const char *str, size_t len);

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    python_term_write(str, len);
}

mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    python_term_write(str, len);
    return len;
}

int mp_hal_stdin_rx_chr(void) {
    return -1;
}
