# WiFi Update Configuration

This module is configured through `wifi_config.h` (defaults) and an optional local
override file `wifi_config.local.h` (not tracked by git).

## Quick start

1) Create `wifi_config.local.h` in this folder.
2) Define the required settings (see below).
3) Build as usual.

Example:

```c
// wifi_config.local.h
#define HOST "example.org"
#define PORT 80
#define BASE_URL_PATH "/pax/"
#define LIST_FILE_REMOTE "list.txt"
#define LIST_FILE_LOCAL "list.txt"
#define USE_HTTPS 0
```

## Build and install

### HTTPS dependency (MbedTLS)

If `USE_HTTPS` is set to `1`, you must build MbedTLS first. The static libraries
are expected at `../external/mbedtls/build/library/`.
`make mbedtls` does not initialize submodules, so ensure the submodule is present.
If the build fails, see the MbedTLS documentation:
```
https://github.com/Mbed-TLS/mbedtls
```
Dependencies and generated sources for development builds:
```
https://github.com/Mbed-TLS/mbedtls#generated-source-files-in-the-development-branch
```

From the repo root:

```sh
git submodule update --init --recursive
make -C wifiupdate mbedtls
```

Or from this directory:

```sh
git submodule update --init --recursive
make mbedtls
```

### Build

From this directory:

```sh
make
```

### Install to terminal

This uses the existing `client.py` uploader. Specify the serial device if needed.

```sh
make install
```

If the device is not `ACM0`, override it:

```sh
make install DEVICE=ACM1
```

## Configuration options

- `HOST`
  Server hostname or IP address (string).

- `PORT`
  TCP port number (integer). For HTTP typically 80, for HTTPS typically 443.

- `BASE_URL_PATH`
  Base path on the server (string), e.g. `"/pax/"`.

- `LIST_FILE_REMOTE`
  Remote list filename (string), e.g. `"list.txt"`.

- `LIST_FILE_LOCAL`
  Local list filename to store the downloaded list (string).

- `USE_HTTPS`
  Set to `1` to enable HTTPS, `0` to use plain HTTP (integer).

## Notes

- If any required option is missing or empty, the build fails with a clear
  compile-time error pointing to `wifi_config.local.h`.
- `wifi_config.local.h` is ignored by git; it is safe to keep environment-specific
  settings there.
