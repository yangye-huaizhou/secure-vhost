#export EXTRA_CFLAGS='-ggdb -O0'
make config T=$RTE_TARGET
make -j 16
make install DESTDIR=install
