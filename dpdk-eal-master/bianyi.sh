#export EXTRA_CFLAGS='-ggdb -O0'
make install -j 16 T=$RTE_TARGET DESTDIR=install
