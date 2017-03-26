USE_TEST_SUITE="yes"
USE_M32=0
DEBUG="yes"
HBICT_DELTACOMP="no"
ARM_HOST="no"

# We may be running a user's python, but we should only test with canonical one
HAS_PS="yes"
HAS_PYTHON="yes"
HAS_READLINE="no"
HAS_DASH="yes"
HAS_TCSH="yes"
HAS_ZSH="yes"
HAS_VIM="yes"
VIM="/usr/bin/vim"
HAS_EMACS="yes"
HAS_EMACS_NOX="no"
HAS_SCRIPT="yes"
HAS_SCREEN="yes"
SCREEN="/usr/bin/screen"
HAS_STRACE="yes"
HAS_GDB="yes"
HAS_JAVA="yes"
HAS_JAVAC="yes"
HAS_SSH="yes"
HAS_CILK="no"
HAS_GCL="no"
GCL="no"
HAS_MATLAB="no"
MATLAB="no"
TEST_POSIX_MQ="yes"

OPENMP_CFLAGS="-fopenmp"
if OPENMP_CFLAGS != "":
  HAS_OPENMP="yes"
else:
  HAS_OPENMP="no"

HAS_MPICH="no"
MPICH_MPD=""
MPICH_MPDBOOT=""
MPICH_MPDALLEXIT=""
MPICH_MPIEXEC=""
MPICH_MPDCLEANUP=""

# USES_OPENMPI_ORTED="@USES_OPENMPI_ORTED@"
HAS_OPENMPI="yes"
OPENMPI_MPICC="/usr/lib64/openmpi/bin/mpicc"
OPENMPI_MPIRUN="/usr/lib64/openmpi/bin/mpirun"

if USE_M32:
  HAS_READLINE="no"
  HAS_MPICH="no"
  HAS_OPENMPI="no"
  HAS_CILK="no"

