VPATH=.:../bobyqa

UNAME=$(shell uname)

CXX=mpicxx
FC=mpif77

BOOSTOPT=-lboost_program_options
LAPACK=-llapack

ifeq ($(UNAME), Darwin)
IMPVERSION?=20800
IMP?=/usr/local
IMPLIBDIR=${IMP}/lib
BOOST?=/usr/local
FLIB?=-L/usr/local/Cellar/gcc/7.2.0/lib/gcc/7 -lgfortran
EXTRA=-g3 -gdwarf-3 
# -isystem
else
IMPVERSION?=20700
IMP?=/software/imp/2.7.0
IMPLIBDIR=${IMP}/lib/x86_64-linux-gnu
FLIB?=-lgfortran
EXTRA=-g3
endif

BOOSTINC=-I${BOOST}/include

ifeq (${DEBUGGING_SYMBOLS}, YES)
OPT=-O0 -g
else
OPT=-O2
endif

SRC:=Curve.cpp C12Map.cpp MinChi.cpp BruteForce.cpp RandomWalk.cpp MonteCarlo.cpp STunel.cpp GdMin.cpp Result.cpp main.cpp calfun.cpp

BOBJ:=bobyqa.o bobyqb.o prelim.o trsbox.o update.o altmov.o rescue.o

OBJ:=${SRC:.cpp=.o}  ${BOBJ}

CXXFLAGS=${EXTRA} ${OPT} -I${IMP}/include ${BOOSTINC} -std=c++11 -DIMPVERSION=${IMPVERSION}
FFLAGS=${EXTRA} ${OPT}

IMPLIB=-L${IMPLIBDIR} -Wl,-rpath ${IMPLIBDIR} -limp_saxs -limp_kernel 

# FIXME:  parse-map extract-trace se nelinkuji s imp
# default: ensamble-fit parse-map extract-trace

default: ensamble-fit 

ensamble-fit: ${OBJ}
	${CXX} ${LDFLAGS} -o $@ ${OBJ} ${IMPLIB} ${FLIB}

parse-map: parse-map.o Curve.o C12Map.o
	${CXX} ${LDFLAGS} -o $@ parse-map.o Curve.o C12Map.o

test-fit: test-fit.o Curve.o
	${CXX} ${LDFLAGS} -o $@  test-fit.o Curve.o

test-ipol: test-ipol.o Curve.o C12Map.o
	${CXX} ${LDFLAGS} -o $@   test-ipol.o Curve.o C12Map.o

1dcut minc12 extract-trace: %: %.o
	${CXX} ${LDFLAGS} -o $@ $@.o

bobyqa-logreduce: %: %.o
	${CXX} ${LDFLAGS} -o $@ $@.o ${BOOSTOPT} ${LAPACK}

clean:
	rm -f ${OBJ} ensamble-fit

depend:
	touch .depend
	makedepend -f .depend -- $(CXXFLAGS) -- ${SRC}

-include .depend

# test
# DO NOT DELETE
