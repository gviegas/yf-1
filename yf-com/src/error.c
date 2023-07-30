/*
 * YF
 * error.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <string.h>

#ifdef __STDC_NO_THREADS__
# error "C11 threads required"
#endif

#include "yf-error.h"

#undef YF_STR_MAXLEN
#define YF_STR_MAXLEN 128

/* Current error code/string. */
static _Thread_local int err_ = YF_ERR_UNKNOWN;
static _Thread_local char info_[YF_STR_MAXLEN] = {'\0'};

void yf_seterr(int err, YF_UNUSED const char *info)
{
    err_ = err;

#ifdef YF_DEVEL
    if (info != NULL)
        strncpy(info_, info, YF_STR_MAXLEN-1)[YF_STR_MAXLEN-1] = '\0';
    else
        info_[0] = '\0';

    yf_printerr();
#endif
}

int yf_geterr(void)
{
    return err_;
}

char *yf_geterrinfo(char *dst, size_t n)
{
    if (strlen(info_) < n)
        return strcpy(dst, info_);

    return NULL;
}

void yf_printerr(void)
{
    char *s = NULL;

    switch (err_) {
    case YF_ERR_UNKNOWN:
        s = "Unknown";
        break;
    case YF_ERR_NOMEM:
        s = "No memory";
        break;
    case YF_ERR_INVARG:
        s = "Invalid argument";
        break;
    case YF_ERR_NILPTR:
        s = "Unexpected null pointer";
        break;
    case YF_ERR_NOFILE:
        s = "No such file or directory";
        break;
    case YF_ERR_INVFILE:
        s = "Invalid file or directory";
        break;
    case YF_ERR_INUSE:
        s = "In use";
        break;
    case YF_ERR_BUSY:
        s = "Busy";
        break;
    case YF_ERR_INVCMD:
        s = "Invalid command";
        break;
    case YF_ERR_QFULL:
        s = "Queue full";
        break;
    case YF_ERR_NOTFND:
        s = "Not found";
        break;
    case YF_ERR_EXIST:
        s = "Exist";
        break;
    case YF_ERR_INVWIN:
        s = "Invalid window";
        break;
    case YF_ERR_UNSUP:
        s = "Unsupported";
        break;
    case YF_ERR_OFLOW:
        s = "Overflow";
        break;
    case YF_ERR_LIMIT:
        s = "Limit";
        break;
    case YF_ERR_DEVGEN:
        s = "Device-specific";
        break;
    case YF_ERR_OTHER:
        s = "Unspecified";
        break;
    }

    fprintf(stderr, "\n[YF] ERROR:\n! %s (%d)\n", s, err_);
    if (info_[0] != '\0')
        fprintf(stderr, "! %s\n\n", info_);
    else
        fprintf(stderr, "\n");
}
