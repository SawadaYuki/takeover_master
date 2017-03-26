########################################################
##### Makefile                     written by Kenji KISE

## setenv OMP_NUM_THREADS 1
## setenv OMP_NUM_THREADS 2
## setenv OMP_NUM_THREADS 3
## setenv OMP_NUM_THREADS 4
########################################################

SRC = main.c
TRG = qn24b_openmp

########################################################
$(TRG): $(SRC)
	gcc -Wall -static -O2 $(SRC) -o $(TRG)

intelc:
	icc -openmp -static -O2 $(SRC) -o $(TRG)

clean :
	rm -f *.il *~ core *.o *.s $(TRG)

all:
	$(MAKE) $(TRG)
##### End of Makefile ##################################
