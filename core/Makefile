VPATH=.:../bobyqa

CXX=mpicxx
FC=mpif77
FLIB?=-lifcore

IMP=../imp-2.6.1/build
BOOST=/opt/local

SRC:=Curve.cpp C12Map.cpp MinChi.cpp BruteForce.cpp RandomWalk.cpp MonteCarlo.cpp STunel.cpp Result.cpp main.cpp calfun.cpp

BOBJ:=bobyqa.o bobyqb.o prelim.o trsbox.o update.o altmov.o rescue.o

OBJ:=${SRC:.cpp=.o}  ${BOBJ}

CXXFLAGS=-g3 ${OPT} -I${IMP}/include -I${BOOST}/include
FFLAGS=-g3 ${OPT}

IMPLIB=-L${IMP}/lib -limp_saxs -limp_kernel

default: ensamble-fit parse-map extract-trace

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

clean:
	rm -f ${OBJ} ensamble-fit


# test
