CC=gcc
CXX=g++
CFLAGS=-g -Wall -O2 -I../C/include
CXXFLAGS=$(CFLAGS)
PROGS=kbtree_test khash_keith khash_keith2 khash_test klist_test kseq_test kseq_bench \
		kseq_bench2 ksort_test ksort_test-stl kvec_test kmin_test kstring_bench kstring_bench2 kstring_test \
		kavl_test kavl-lite_test kthread_test2

all:$(PROGS)

clean:
		rm -fr $(PROGS) *.dSYM a.out *.o

kavl_test:kavl_test.c ../C/include/kavl.h
		$(CC) $(CFLAGS) -o $@ kavl_test.c

kavl-lite_test:kavl-lite_test.c ../C/include/kavl-lite.h
		$(CC) $(CFLAGS) -o $@ kavl-lite_test.c

kbtree_test:kbtree_test.c ../C/include/kbtree.h
		$(CC) $(CFLAGS) -o $@ kbtree_test.c

khash_keith:khash_keith.c ../C/include/khash.h
		$(CC) $(CFLAGS) -o $@ khash_keith.c

khash_keith2:khash_keith2.c ../C/include/khash.h
		$(CC) $(CFLAGS) -o $@ khash_keith2.c

khash_test:khash_test.c ../C/include/khash.h
		$(CC) $(CFLAGS) -o $@ khash_test.c

klist_test:klist_test.c ../C/include/klist.h
		$(CC) $(CFLAGS) -o $@ klist_test.c

kseq_test:kseq_test.c ../C/include/kseq.h
		$(CC) $(CFLAGS) -o $@ kseq_test.c -lz

kseq_bench:kseq_bench.c ../C/include/kseq.h
		$(CC) $(CFLAGS) -o $@ kseq_bench.c -lz

kseq_bench2:kseq_bench2.c ../C/include/kseq.h
		$(CC) $(CFLAGS) -o $@ kseq_bench2.c -lz

ksort_test:ksort_test.c ../C/include/ksort.h
		$(CC) $(CFLAGS) -o $@ ksort_test.c

ksort_test-stl:ksort_test.cc ../C/include/ksort.h
		$(CXX) $(CXXFLAGS) -o $@ ksort_test.cc

kvec_test:kvec_test.cc ../C/include/kvec.h
		$(CXX) $(CXXFLAGS) -o $@ kvec_test.cc

kmin_test:kmin_test.c ../C/kmath/kmath.h ../C/kmath/kmath.c
		$(CC) $(CFLAGS) -o $@ kmin_test.c -I../C/kmath ../C/kmath/kmath.c -lm

kstring_bench:kstring_bench.c ../C/kstring/kstring.h ../C/kstring/kstring.c
		$(CC) $(CFLAGS) -o $@ kstring_bench.c -I../C/kstring ../C/kstring/kstring.c

kstring_bench2:kstring_bench2.c ../C/kstring/kstring.h ../C/kstring/kstring.c
		$(CC) $(CFLAGS) -o $@ kstring_bench2.c -I../C/kstring ../C/kstring/kstring.c

kstring_test:kstring_test.c ../C/kstring/kstring.h ../C/kstring/kstring.c
		$(CC) $(CFLAGS) -o $@ kstring_test.c -I../C/kstring ../C/kstring/kstring.c

kthread_test:kthread_test.c ../C/kthread/kthread.h ../C/kthread/kthread.c
		$(CC) $(CFLAGS) -fopenmp -o $@ kthread_test.c -I../C/kthread ../C/kthread/kthread.c

kthread_test2:kthread_test2.c ../C/kthread/kthread.h ../C/kthread/kthread.c
		$(CC) $(CFLAGS) -o $@ kthread_test2.c -I../C/kthread ../C/kthread/kthread.c -lpthread

ketopt_test:ketopt_test.c ../C/include/ketopt.h
		$(CC) $(CFLAGS) -o $@ ketopt_test.c
