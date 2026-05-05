#include "py/runtime.h"
#include "py/obj.h"

extern void python_start_input_mode(void);
extern const char* python_get_input_line(void);

static uint64_t _random_state = 12345;

static uint64_t _lcg_next(void) {
    _random_state = _random_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return _random_state >> 32;
}

static mp_obj_t mp_random_randint(mp_obj_t min_o, mp_obj_t max_o) {
    mp_int_t lo = mp_obj_get_int(min_o);
    mp_int_t hi = mp_obj_get_int(max_o);

    if (lo > hi) {
        mp_raise_ValueError(MP_ERROR_TEXT("randint: min > max"));
    }

    uint64_t range = hi - lo + 1;
    uint64_t val = _lcg_next() % range;
    return mp_obj_new_int(lo + val);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_random_randint_obj, mp_random_randint);

static const mp_rom_map_elem_t mp_module_random_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_random) },
    { MP_ROM_QSTR(MP_QSTR_randint), MP_ROM_PTR(&mp_random_randint_obj) },
};
MP_DEFINE_CONST_DICT(mp_module_random_globals, mp_module_random_globals_table);

const mp_obj_module_t mp_module_random = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_random_globals,
};
MP_REGISTER_MODULE(MP_QSTR_random, mp_module_random);

// Built-in load_file function for loading Python files
static mp_obj_t mp_builtin_load_file(mp_obj_t filename_o) {
    const char *filename = mp_obj_str_get_str(filename_o);
    extern void python_term_write(const char *str, size_t len);

    python_term_write("load_file: ", 11);
    python_term_write(filename, 0);
    while (filename[0]) filename++;
    python_term_write(" (stub)\n", 8);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_load_file_obj, mp_builtin_load_file);
