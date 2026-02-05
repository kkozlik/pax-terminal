echo Tohle je jen spoustec aktualizace pres WIFI - ta musí být nastavena jinak se to posere !!
echo
echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

rm wifi.so
#arm-none-eabi-gcc -shared -fPIC -I /usr/arm-linux-gnueabi/include/  -o wifi.so wifi.c -nostartfiles -static

# HTTPS (MbedTLS 4.x, PSA RNG; ne linkovat -lssl/-lcrypto)
MBEDTLS_DIR=../external/mbedtls
MBEDTLS_BUILD=$MBEDTLS_DIR/build
MBEDTLS_LIBS="$MBEDTLS_BUILD/library/libmbedtls.a \
    $MBEDTLS_BUILD/library/libmbedx509.a \
    $MBEDTLS_BUILD/library/libmbedcrypto.a"

arm-none-eabi-gcc -shared -fPIC -I /usr/arm-linux-gnueabi/include/ \
    -I $MBEDTLS_DIR/include/ \
    -I $MBEDTLS_DIR/tf-psa-crypto/include/ \
    -I $MBEDTLS_DIR/tf-psa-crypto/drivers/builtin/include/ \
    -DUSE_HTTPS -o wifi.so wifi.c mbedtls_entropy.c $MBEDTLS_LIBS -nostartfiles -static
    # -o wifi.so wifi.c mbedtls_entropy.c -nostartfiles -static



# python3 ../client.py ACM0 push wifi.so /data/app/MAINAPP/lib/wifiupdate.so
python3 ../client.py ACM0 push wifi.so /data/app/MAINAPP/lib/libosal.so
echo jeste Loader
python3 ../client.py ACM0 push L /data/app/MAINAPP/Loader
echo ted reboot a stahne se to samo ... pokud mas nastavenou wifi
