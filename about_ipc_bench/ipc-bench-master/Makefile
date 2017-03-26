
#CFLAGS = -g -Wall -O3 -lrt
CFLAGS = -g -Wall -O3 

all: 	unix_thr \
	tcp_thr \
	shm

run:
	./unix_thr 1000 100000000
	./tcp_thr 1000 100000000

clean:
	rm -f *~ core 
	rm -f unix_thr 
	rm -f tcp_thr 
	rm -f shm 
