VPATH=.:../bobyqa

CXX=mpic++
FC=mpif77

SRC:=Curve.cpp C12Map.cpp MinChi.cpp BruteForce.cpp MonteCarlo.cpp STunel.cpp main.cpp calfun.cpp

BOBJ:=bobyqa.o bobyqb.o prelim.o trsbox.o update.o altmov.o rescue.o

OBJ:=${SRC:.cpp=.o}  ${BOBJ}

CXXFLAGS=-g3 ${OPT}

default: ensamble-fit parse-map extract-trace

ensamble-fit: ${OBJ}
	${CXX} ${LDFLAGS} -o $@ ${OBJ} -lifcore

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
