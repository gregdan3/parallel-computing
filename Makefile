.PHONY: clean display* run*
MPIFLAGS = mpiexec --oversubscribe --mca opal_warn_on_missing_libcuda 0

all: life life_openmp life_mpi proc

life: life.c
	gcc ./life.c -o life -std=c99 -Wall -Ofast
life_openmp: life_openmp.c
	gcc ./life_openmp.c -o life_openmp -std=c99 -Wall -fopenmp -Ofast
life_mpi: life_mpi.c
	mpicc ./life_mpi.c -o life_mpi -std=c99 -Wall -Ofast
proc: proc.c
	mpicc ./proc.c -o proc -std=c99 -Wall -Ofast
clean:
	rm ./life ./life_openmp ./life_mpi ./proc


run:
	./life -h 1000 -w 1000 -g 1000
display:
	clear
	./life -h 20 -w 20 -g 20 -s

run-openmp:
	./life_openmp -h 1000 -w 1000 -g 1000 -x 4 -y 4
display-openmp:
	clear
	./life -h 20 -w 20 -g 20 -x 2 -y 2 -s

run-mpi:
	$(MPIFLAGS) -n 4 ./life_mpi -h 1000 -w 1000 -g 1000
run-mpi-nb:
	$(MPIFLAGS) -n 4 ./life_mpi -h 1000 -w 1000 -g 1000 -n
display-mpi:
	clear
	./life -h 20 -w 20 -g 20 -x 2 -y 2 -s

run-proc:
	echo -e "\n\n4 proc"
	$(MPIFLAGS) -n 4 ./proc
	echo -e "\n\n8 proc"
	$(MPIFLAGS) -n 8 ./proc
