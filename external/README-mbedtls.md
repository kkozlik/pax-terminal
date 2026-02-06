# Build

```
git submodule update --init --recursive
cd mbedtls
scripts/config.py baremetal
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../arm-gcc-toolchain.cmake \
  -DENABLE_TESTING=OFF \
  -DENABLE_PROGRAMS=OFF \
  -DBUILD_SHARED_LIBS=OFF
cmake --build .
```

Výstupní knihovny jsou v `mbedtls/build/library/`:
`libmbedtls.a`, `libmbedx509.a`, `libmbedcrypto.a`.
