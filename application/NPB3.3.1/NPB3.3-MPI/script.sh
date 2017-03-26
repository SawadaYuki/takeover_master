#!/bin/sh

#IS
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/is.C.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/is.C.4
done

#CG
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/cg.C.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/cg.C.4
done

#MG
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/mg.C.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/mg.C.4
done

#SP
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/sp.A.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/sp.A.4
done

#BT
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/bt.C.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/bt.C.4
done

#FT
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/ft.C.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/ft.C.4
done

#EP
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/ep.C.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/ep.C.4
done

#LU
for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm bin/lu.C.4
done

for i in 1 2 3 4 5 6 7 8 9
do 
 mpirun -np 4 --mca btl ^sm -machinefile host bin/lu.C.4
done
