VPATH=.:../bobyqa

CXX=mpicxx
FC=mpif77
FLIB?=-lgfortran
OPT=-O2

# Mac
#IMP=../imp-2.6.1/build

#BOOST=/opt/local
#BOOSTINC=-I${BOOST}/include

BOOSTOPT=-lboost_program_options
LAPACK=-llapack

IMP=/software/imp/2.6.1
IMPLIBDIR=${IMP}/lib/x86_64-linux-gnu

SRC:=Curve.cpp C12Map.cpp MinChi.cpp BruteForce.cpp RandomWalk.cpp MonteCarlo.cpp STunel.cpp Result.cpp main.cpp calfun.cpp

BOBJ:=bobyqa.o bobyqb.o prelim.o trsbox.o update.o altmov.o rescue.o

OBJ:=${SRC:.cpp=.o}  ${BOBJ}

CXXFLAGS=-g3 ${OPT} -isystem ${IMP}/include ${BOOSTINC} -std=c++11
FFLAGS=-g3 ${OPT}

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


# test
