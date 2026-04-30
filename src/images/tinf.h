/*
 * tinf - tiny inflate library (inflate, gzip, zlib)
 *
 * Copyright (c) 2003-2019 Joergen Ibsen
 */

#ifndef TINF_H
#define TINF_H

#define TINFCC

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TINF_OK         = 0,
    TINF_DATA_ERROR = -3,
    TINF_BUF_ERROR  = -5
} tinf_error_code;

int TINFCC tinf_uncompress(void *dest, unsigned int *destLen,
                           const void *source, unsigned int sourceLen);

int TINFCC tinf_zlib_uncompress(void *dest, unsigned int *destLen,
                                const void *source, unsigned int sourceLen);

unsigned int TINFCC tinf_adler32(const void *data, unsigned int length);

#ifdef __cplusplus
}
#endif

#endif /* TINF_H */
