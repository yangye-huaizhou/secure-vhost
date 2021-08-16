#export EXTRA_CFLAGS='-ggdb -O0'
make install -j 16 T=x86_64-native-linuxapp-gcc DESTDIR=install
