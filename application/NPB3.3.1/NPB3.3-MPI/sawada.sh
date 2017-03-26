#!/bin/sh


./calc-cg.sh mpirun -np 4 --mca btl ^sm -machinefile host_remote bin/cg.C.4

# ./calc-lu.sh mpirun -np 4 --mca btl ^sm -machinefile host bin/lu.C.4

# ./calc-ep.sh mpirun -np 4 --mca btl ^sm -machinefile host bin/ep.C.4

# ./calc-sp.sh mpirun -np 4 --mca btl ^sm -machinefile host bin/sp.A.4

# ./calc-bt.sh mpirun -np 4 --mca btl ^sm -machinefile host bin/bt.C.4

# ./calc-ft.sh mpirun -np 4 --mca btl ^sm -machinefile host bin/ft.C.4

# ./calc-is.sh mpirun -np 4 --mca btl ^sm -machinefile host bin/is.C.4

# ./calc-mg.sh mpirun -np 4 --mca btl ^sm -machinefile host bin/mg.C.4

