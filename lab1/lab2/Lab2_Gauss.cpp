#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
//#include <sys/time.h>
//#include <time.h>
//#include <iostream>

#define	NUMBER_REPS	1000

#if _MSC_VER
#pragma comment(lib, "msmpi.lib")
//#pragma comment(lib, "Ws2_32.lib")
#endif

namespace MatrixCompute
{
	struct MatrixState
	{
		int m_SelectedRow;
		int m_SeletedRowForSwappingWithMain;
	};

}


int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	// Get the number of processes
	int worldSize = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	// Get the rank of the process
	int rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	MPI_Status status;

	if (worldSize < 2) {
		printf("World is too small\n");
		return 1;
	}

	if (rank == 0)
	{
		printf("World size is %d\n", worldSize);

	}


	MPI_Finalize();
	return 0;
}