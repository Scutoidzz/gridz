#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"

extern void python_term_write(const char *str, size_t len);
extern void mp_embed_exec_str(const char *src);

extern void    *fat32_read_file(const char *filename, size_t *out_size);
extern int      fat32_list_directory(uint32_t cluster, void *out, int max_out, int slave);
extern uint32_t fat32_root_cluster(void);
extern int      fat32_is_initialized(void);

static void term_puts(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    python_term_write(s, len);
}

// DirInfo mirror — must match fat32.hpp layout exactly
typedef struct {
    char     name[13];
    uint32_t size;
    uint8_t  is_dir;
    uint32_t cluster;
} DirInfo;

// Walk root to find a subdirectory's cluster.
// Returns 0 if not found or not a directory.
// We re-use fat32_list_directory scanning root entries ourselves — actually
// just delegate to the kernel: call list_directory on root and scan names.
static uint32_t find_dir_cluster(const char *dirname) {
    DirInfo entries[64];
    int count = fat32_list_directory(fat32_root_cluster(), entries, 64, 0);
    for (int i = 0; i < count; i++) {
        if (!entries[i].is_dir) continue;
        // Case-insensitive compare
        const char *a = entries[i].name, *b = dirname;
        int match = 1;
        while (*a && *b) {
            char ca = *a >= 'a' && *a <= 'z' ? *a - 32 : *a;
            char cb = *b >= 'a' && *b <= 'z' ? *b - 32 : *b;
            if (ca != cb) { match = 0; break; }
            a++; b++;
        }
        if (match && !*a && !*b) return entries[i].cluster;
    }
    return 0;
}

static mp_obj_t mp_load_file(mp_obj_t filename_o) {
    const char *filename = mp_obj_str_get_str(filename_o);

    if (!fat32_is_initialized()) {
        term_puts("Error: filesystem not ready\n");
        return mp_const_none;
    }

    // Try as given first, then as python/<filename>
    size_t file_size = 0;
    char *src = (char *)fat32_read_file(filename, &file_size);

    if (!src || file_size == 0) {
        // Build "python/<filename>" path
        char path[128];
        int pi = 0;
        const char *prefix = "python/";
        while (prefix[pi]) { path[pi] = prefix[pi]; pi++; }
        const char *fn = filename;
        while (*fn && pi < 127) { path[pi++] = *fn++; }
        path[pi] = '\0';
        src = (char *)fat32_read_file(path, &file_size);
    }

    if (!src || file_size == 0) {
        term_puts("Error: file not found: ");
        term_puts(filename);
        term_puts("\n");
        return mp_const_none;
    }

    term_puts("Running: ");
    term_puts(filename);
    term_puts("\n");

    mp_embed_exec_str(src);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_load_file_obj, mp_load_file);

static mp_obj_t mp_list_files(void) {
    if (!fat32_is_initialized()) {
        term_puts("Filesystem not ready\n");
        return mp_const_none;
    }

    // List the python/ subdirectory
    uint32_t py_cluster = find_dir_cluster("python");
    if (py_cluster == 0) {
        term_puts("No /python directory found\n");
        return mp_const_none;
    }

    DirInfo entries[32];
    int count = fat32_list_directory(py_cluster, entries, 32, 0);

    term_puts("Python files:\n");
    for (int i = 0; i < count; i++) {
        if (entries[i].is_dir) continue;
        term_puts("  ");
        term_puts(entries[i].name);
        term_puts("\n");
    }
    if (count == 0) term_puts("  (none)\n");
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_list_files_obj, mp_list_files);

static const mp_rom_map_elem_t mp_module_files_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_files) },
    { MP_ROM_QSTR(MP_QSTR_load),     MP_ROM_PTR(&mp_load_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_ls),       MP_ROM_PTR(&mp_list_files_obj) },
};
MP_DEFINE_CONST_DICT(mp_module_files_globals, mp_module_files_globals_table);

const mp_obj_module_t mp_module_files = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_files_globals,
};
MP_REGISTER_MODULE(MP_QSTR_files, mp_module_files);
