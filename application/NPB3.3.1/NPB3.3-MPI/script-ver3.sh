#!/bin/sh

#IS
# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm bin/is.C.4 > IS_one_${i}.txt
# done

for i in 1 2 3 4 5 6 7
do 
 /opt/sawada-tmp/tmp_mpi/bin/mpirun -np 4 --mca btl ^sm -machinefile host bin/is.C.4 > IS_two_${i}.txt
done

#CG
# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm bin/cg.C.4 >> CG_one_${i}.txt
# done

for i in 1 2 3 4 5 6 7
do 
 /opt/sawada-tmp/tmp_mpi/bin/mpirun -np 4 --mca btl ^sm -machinefile host bin/cg.C.4 > CG_two_${i}.txt
done

#MG
# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm bin/mg.C.4 >> MG_one_${i}.txt
# done

# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm -machinefile host bin/mg.C.4 >> MG_two_${i}.txt
# done


#SP
for i in 1 2 3 4 5 6 7 8 
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/sp.C.4 > SP_one_${i}.txt
done

# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm -machinefile host bin/sp.A.4 >> SP_two_${i}.txt
# done

# #BT
# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm bin/bt.C.4 >> BT_one_${i}.txt
# done

# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm -machinefile host bin/bt.C.4 >> BT_two_${i}.txt
# done

#FT
# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm -machinefile host bin/ft.C.4 >> FT_one_${i}.txt
# done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/ft.C.4 >> FT_two_${i}.txt
done

# #EP
# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm bin/ep.C.4 >> EP_one_${i}.txt
# done

# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm -machinefile host bin/ep.C.4 >> EP_two_${i}.txt
# done

# #LU
# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm bin/lu.C.4 >> LU_one_${i}.txt
# done

# for i in 1 2 3 4 5 6 7 8 9
# do 
#  mpirun -np 4 --mca btl ^sm -machinefile host bin/lu.C.4 >> LU_two_${i}.txt
# done
