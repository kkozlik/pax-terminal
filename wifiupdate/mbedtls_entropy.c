#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <psa/crypto.h>
#include <mbedtls/platform.h>

int mbedtls_platform_get_entropy(psa_driver_get_entropy_flags_t flags,
                                 size_t *estimate_bits,
                                 unsigned char *output, size_t output_size) {
    (void)flags;

    // Use /dev/urandom as a platform entropy source.
    if (output_size == 0) {
        if (estimate_bits) *estimate_bits = 0;
        return PSA_SUCCESS;
    }

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return PSA_ERROR_INSUFFICIENT_ENTROPY;

    size_t total = 0;
    while (total < output_size) {
        ssize_t r = read(fd, output + total, output_size - total);
        if (r > 0) {
            total += (size_t)r;
            continue;
        }
        if (r < 0 && errno == EINTR) continue;
        close(fd);
        return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }
    close(fd);

    if (estimate_bits) *estimate_bits = output_size * 8;
    return PSA_SUCCESS;
}
