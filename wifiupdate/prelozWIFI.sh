echo Tohle je jen spoustec aktualizace pres WIFI - ta musí být nastavena jinak se to posere !!
echo
echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#rm wifi.so
#arm-none-eabi-gcc -shared -fPIC -I /usr/arm-linux-gnueabi/include/  -o wifi.so wifi.c -nostartfiles -static

# python3 ../client.py ACM0 push wifi.so /data/app/MAINAPP/lib/wifiupdate.so
python3 ../client.py ACM0 push wifi.so /data/app/MAINAPP/lib/libosal.so
echo jeste Loader
python3 ../client.py ACM0 push L /data/app/MAINAPP/Loader
echo ted reboot a stahne se to samo ... pokud mas nastavenou wifi

