// lab1.cpp: ���������� ����� ����� ��� ����������� ����������.
//

//#include "stdafx.h"

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
//#include <sys/time.h>
#include <time.h>
//#include <iostream>

#define	NUMBER_REPS	1000

#if _MSC_VER
#pragma comment(lib, "msmpi.lib")
//#pragma comment(lib, "Ws2_32.lib")
#endif

#define SAFE_DELETE_ARRAY(x) if(x) delete[]x;
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

/*
2.	�������� ������������������ ������� MPI_Bcast � MPI_Send/MPI_Recv ��� ������ ���������� ��������������� �����.
*/

namespace Benchmarking_1
{
	void Task1(int reps, double &SumTimeDelta, int size, int rank, MPI_Status& status, double&BCastTimeDelta, bool _compare)
	{
		for (auto i = 0; i < reps; i++)
		{
			if (rank == 0)
			{
				for (auto j = 1; j < size; j++)
				{
					auto _startTime = MPI_Wtime();     /* start time */
													   /* send message to worker - message tag set to 1.  */
													   /* If return code indicates error quit */
					int rc = MPI_Send(NULL, 0, MPI_BYTE, j, 1, MPI_COMM_WORLD);

					if (rc != MPI_SUCCESS)
					{
						printf("Send error in task 0!\n");
						MPI_Abort(MPI_COMM_WORLD, rc);
						exit(1);
					}

					auto _endTime = MPI_Wtime();     /* end time */

					/* calculate round trip time and print */
					auto deltaT = _endTime - _startTime;

					SumTimeDelta += deltaT;
				}
			}
			else
			{
				/* Now wait to receive the echo reply from the worker  */
				auto rc = MPI_Recv(NULL, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
				if (rc != MPI_SUCCESS)
				{
					printf("Receive error in task 0!\n");
					MPI_Abort(MPI_COMM_WORLD, rc);
					exit(1);
				}
			}
		}

		{
			if ((rank == 0) && (_compare))
			{
				auto _startTime = MPI_Wtime();     /* start time */

				for (int i = 0; i < reps; i++)
				{
					MPI_Bcast(NULL, 0, MPI_BYTE, 0, MPI_COMM_WORLD);
				}

				auto _endTime = MPI_Wtime();     /* end time */

				/* calculate round trip time and print */
				auto deltaT = _endTime - _startTime;
				BCastTimeDelta += deltaT;
			}
		}
	}

	inline void BenchmarkBcastAndSendRecv(int _count, double&SumTimeDelta, double&BCastTimeDelta, int size, int rank, MPI_Status& status)
	{
		Task1(_count, SumTimeDelta, size, rank, status, BCastTimeDelta, true);
	}

	inline void RunTask1(int _count, double&SumTimeDelta, double&BCastTimeDelta, int size, int rank, MPI_Status& status)
	{
		Task1(_count, SumTimeDelta, size, rank, status, BCastTimeDelta, false);
	}
}

namespace Benchmarking_2
{
}

int main(int argc, char*argv[])
{
	int 		tag,                    /* MPI message tag parameter */

		size,               /* number of MPI tasks */

		rank,                   /* my MPI task number */
		dest, source,           /* send/receive task designators */
		avgT,                   /* average time per rep in microseconds */
		rc,                     /* return code */
		n;
	double T1, T2,              /* start/end times per rep */
		sumT,                   /* sum of all reps times */
		deltaT;                 /* time for one rep */
	char* msg;                   /* buffer containing 1 byte message */


	MPI_Status status;          /* MPI receive routine parameter */

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// �������
	MPI_Barrier(MPI_COMM_WORLD);

	sumT = 0;
	tag = 1;

	double SumTimeDelta = 0;
	double BCastTimeDelta = 0;

	//printf("Capacity checking Started \n");

	/*���������� ���������*/
	int Count = 10;
	{
		using namespace Benchmarking_1;
		BenchmarkBcastAndSendRecv(Count, SumTimeDelta, BCastTimeDelta, size, rank, status);
	}

	if (rank == 0)
	{
		printf("Send/Recv time %f ms for operations %i Performance %f operations for per-ms \n", SumTimeDelta*(1000000 / CLOCKS_PER_SEC), Count, SumTimeDelta*(1000000 / CLOCKS_PER_SEC) / Count);
		printf("BCast time %f ms for operations %i Performance %f operations for per-ms \n", BCastTimeDelta*(1000000 / CLOCKS_PER_SEC), Count, SumTimeDelta *(1000000 / CLOCKS_PER_SEC) / Count);
	}

	MPI_Finalize();
	exit(0);
	return 0;
}