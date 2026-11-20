/*
 * messages.c - Turkish descriptions for common errno values.
 */
#include <errno.h>
#include <string.h>

#include "messages.h"

const char *msg_error(int errnum)
{
    switch (errnum) {
    case ENOENT:       return "böyle bir dosya ya da dizin yok";
    case EACCES:       return "izin reddedildi";
    case EPERM:        return "bu işleme izin verilmiyor";
    case ENOTDIR:      return "bir dizin değil";
    case EISDIR:       return "bu bir dizin";
    case EEXIST:       return "zaten var";
    case ENOEXEC:      return "çalıştırılabilir bir dosya değil";
    case ENAMETOOLONG: return "ad çok uzun";
    case ELOOP:        return "çok fazla sembolik bağlantı";
    case ENOMEM:       return "bellek yetersiz";
    case E2BIG:        return "argüman listesi çok uzun";
    default:           return strerror(errnum);
    }
}
