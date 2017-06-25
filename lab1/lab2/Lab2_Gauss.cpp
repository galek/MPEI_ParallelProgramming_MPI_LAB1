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

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) if(x) { delete x; x=nullptr;}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) if(x) { delete []x; x=nullptr;}
#endif


namespace MatrixCompute
{
	struct MatrixState
	{
		int m_SelectedRow;
		int m_SeletedRowForSwappingWithMain;
	};

	static inline bool IsMain(int rank) {
		return rank == 0;
	}

	static const int MAIN_RANK = 0;

	void GenerateMatrix(double*matrix, int rank, int worldSize, int _equationCount)
	{
		if (IsMain(rank))
		{
			for (int node = 1; node < worldSize; ++node)
			{
			// сгенерируем столбец
			}
		}
		else
		{
		}
	}

}
using namespace MatrixCompute;


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


	auto _timeStart = MPI_Wtime();

	auto *matrix = new double[10];
	GenerateMatrix(matrix, rank, worldSize, 10);

	auto _timeEnd = MPI_Wtime();


	if (IsMain(rank)) {
		printf("Time for generating = %f\n", _timeEnd - _timeStart);
	}
	MPI_Barrier(MPI_COMM_WORLD);


	MPI_Finalize();
	return 0;
}