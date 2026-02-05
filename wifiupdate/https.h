#ifndef WIFIUPDATE_HTTPS_H
#define WIFIUPDATE_HTTPS_H

#ifdef USE_HTTPS
#include <mbedtls/ssl.h>

typedef struct {
    int fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
} https_session_t;

int https_connect(https_session_t *s, const char *host);
void https_session_free(https_session_t *s);
#endif

#endif
