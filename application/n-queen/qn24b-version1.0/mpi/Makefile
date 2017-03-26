########################################################
##### Makefile                     written by Kenji KISE

SRC = main_for_balance_20170327.c
TRG = qn24b_mpi_last_syuron

########################################################
$(TRG): $(SRC)
	mpicc -g -Wall -O2 $(SRC) -o $(TRG) -L/home/fss5/sawada/mpiP/prefix_mpiP/lib -lmpiP -lm -lbfd -liberty

#mpicc -g -Wall -O2 $(SRC) -o $(TRG) -L/home/fss5/sawada/mpiP/prefix_mpiP/lib -lmpiP -lm -lbfd -liberty

#mpicc -Wall -O2 $(SRC) -o $(TRG) # original
#mpicc -Wall -O2 $(SRC) -o $(TRG) -DDEBUG # sawada ver
#mpicc -Wall -static -O2 $(SRC) -o $(TRG)

########################################################
all:
	$(MAKE) $(TRG)
########################################################
clean:
	rm -f *~ core *.o *.s $(TRG)
########################################################
