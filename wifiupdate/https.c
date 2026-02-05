#include "wifi_config.h"

#if USE_HTTPS
#include "https.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <mbedtls/error.h>
#include <psa/crypto.h>

// Provided by wifi.c
void my_log(const char *format, ...);

static void https_session_init(https_session_t *s) {
    s->fd = -1;
    mbedtls_ssl_init(&s->ssl);
    mbedtls_ssl_config_init(&s->conf);
}

void https_session_free(https_session_t *s) {
    mbedtls_ssl_free(&s->ssl);
    mbedtls_ssl_config_free(&s->conf);
    if (s->fd >= 0) {
        close(s->fd);
        s->fd = -1;
    }
}

// Custom BIO callbacks to avoid MBEDTLS_NET_C (non-POSIX platforms).
static int https_send_cb(void *ctx, const unsigned char *buf, size_t len) {
    int fd = *(int *)ctx;
    int ret = send(fd, buf, len, 0);
    if (ret >= 0) return ret;
    if (errno == EAGAIN || errno == EWOULDBLOCK) return MBEDTLS_ERR_SSL_WANT_WRITE;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

// Map socket recv to Mbed TLS non-blocking semantics.
static int https_recv_cb(void *ctx, unsigned char *buf, size_t len) {
    int fd = *(int *)ctx;
    int ret = recv(fd, buf, len, 0);
    if (ret > 0) return ret;
    if (ret == 0) return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
    if (errno == EAGAIN || errno == EWOULDBLOCK) return MBEDTLS_ERR_SSL_WANT_READ;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

// Separate connector so HTTPS can use port 443 while HTTP uses PORT.
static int connect_socket_port(const char *host, int port) {
    struct hostent *h = gethostbyname(host);
    if (!h) return -1;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) return -1;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)h->h_addr);
    memset(&(server_addr.sin_zero), 0, 8);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int https_connect(https_session_t *s, const char *host) {
    int ret;

    https_session_init(s);

    // TLS uses PSA RNG in MbedTLS 4.x.
    if ((ret = psa_crypto_init()) != PSA_SUCCESS) {
        my_log("   [ERR] PSA init fail: %d\n", ret);
        return -1;
    }

    s->fd = connect_socket_port(host, 443);
    if (s->fd < 0) {
        my_log("   [ERR] TLS connect fail: %s\n", host);
        return -1;
    }

    if ((ret = mbedtls_ssl_config_defaults(&s->conf, MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        my_log("   [ERR] TLS conf fail: -0x%04x\n", -ret);
        return -1;
    }

    mbedtls_ssl_conf_authmode(&s->conf, MBEDTLS_SSL_VERIFY_NONE);

    if ((ret = mbedtls_ssl_setup(&s->ssl, &s->conf)) != 0) {
        my_log("   [ERR] TLS setup fail: -0x%04x\n", -ret);
        return -1;
    }

    if ((ret = mbedtls_ssl_set_hostname(&s->ssl, host)) != 0) {
        my_log("   [ERR] TLS hostname fail: -0x%04x\n", -ret);
        return -1;
    }

    // Wire TLS to our socket callbacks.
    mbedtls_ssl_set_bio(&s->ssl, &s->fd, https_send_cb, https_recv_cb, NULL);

    while ((ret = mbedtls_ssl_handshake(&s->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            my_log("   [ERR] TLS handshake fail: -0x%04x\n", -ret);
            return -1;
        }
    }

    return 0;
}
#endif
