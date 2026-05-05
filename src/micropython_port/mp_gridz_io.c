#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"

extern void python_start_input_mode(void);
extern const char* python_get_input_line(void);
extern bool python_input_ready(void);
extern void python_input_clear(void);

static mp_obj_t mp_gridz_input(mp_obj_t prompt_o) {
    const char *prompt = mp_obj_str_get_str(prompt_o);
    extern void python_term_write(const char *str, size_t len);

    // Print prompt
    size_t prompt_len = 0;
    const char *p = prompt;
    while (*p++) prompt_len++;
    python_term_write(prompt, prompt_len);

    // Enter input mode
    python_start_input_mode();

    // Busy-wait for input
    volatile int wait_count = 0;
    while (!python_input_ready()) {
        wait_count++;
        if (wait_count > 1000000) {
            python_input_clear();
            return mp_obj_new_str("", 0);
        }
        __asm__ volatile("pause");
    }

    // Get input and convert to Python string
    const char *result = python_get_input_line();
    size_t result_len = 0;
    const char *r = result;
    while (*r++) result_len++;

    python_input_clear();

    return mp_obj_new_str(result, result_len);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_gridz_input_obj, mp_gridz_input);

static mp_obj_t mp_gridz_print(mp_obj_t obj) {
    extern void python_term_write(const char *str, size_t len);
    const char *str = mp_obj_str_get_str(obj);
    int len = 0;
    while (str[len]) len++;
    python_term_write(str, len);
    python_term_write("\n", 1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_gridz_print_obj, mp_gridz_print);

static const mp_rom_map_elem_t mp_module_gridz_io_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_gridz_io) },
    { MP_ROM_QSTR(MP_QSTR_input), MP_ROM_PTR(&mp_gridz_input_obj) },
};
MP_DEFINE_CONST_DICT(mp_module_gridz_io_globals, mp_module_gridz_io_globals_table);

const mp_obj_module_t mp_module_gridz_io = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_gridz_io_globals,
};
MP_REGISTER_MODULE(MP_QSTR_gridz_io, mp_module_gridz_io);
