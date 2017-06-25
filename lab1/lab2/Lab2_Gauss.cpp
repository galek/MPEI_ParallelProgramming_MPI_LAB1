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

	MPI_Finalize();
	return 0;
}