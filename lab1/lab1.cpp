// lab1.cpp: ���������� ����� ����� ��� ����������� ����������.
//

//#include "stdafx.h"

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


/*
1.	��������� ����������� � ���������� ����������� ������� ������ ������� ��� ������������� ������� MPI. ������:
1.	��������� ����������� � ���������� ����������� ��� �������� ������ � ������� ������� MPI_Send/MPI_Recv. ������, ��� ����� ��� ���������� ��������� ����� �� ����� ���� ����������� ��� ����� ������������ ������ �����, ��� � ����� �������.
2.	�������� ������������������ ������� MPI_Bcast � MPI_Send/MPI_Recv ��� ������ ���������� ��������������� �����.
3.	�������� ������������������ ������� MPI_Reduce � MPI_Send/MPI_Recv ��� ������ ���������� ��������������� �����.
*/

namespace Benchmarking_1
{
	void func(int&rank, int&dest, int&source, int reps, int&rc, double &sumT, int &avgT, char msg, MPI_Status& status, int tag, int numtasks)
	{

		if (rank == 0)
		{
			/* round-trip latency timing test */
			printf("task %d has started...\n", rank);
			printf("Beginning latency timing test. Number of reps = %d.\n", reps);
			printf("***************************************************\n");
			printf("Rep#       T1               T2            deltaT\n");
			dest = 1;
			source = 1;

			for (auto i = 1; i <= reps; i++)
			{
				for (auto j = 1; j < numtasks; j++)
				{
					auto _startTime = MPI_Wtime();     /* start time */
													   /* send message to worker - message tag set to 1.  */
													   /* If return code indicates error quit */
					rc = MPI_Send(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);

					if (rc != MPI_SUCCESS)
					{
						printf("Send error in task 0!\n");
						MPI_Abort(MPI_COMM_WORLD, rc);
						exit(1);
					}

					/* Now wait to receive the echo reply from the worker  */
					/* If return code indicates error quit */
					rc = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);

					if (rc != MPI_SUCCESS)
					{
						printf("Receive error in task 0!\n");
						MPI_Abort(MPI_COMM_WORLD, rc);
						exit(1);
					}

					auto _endTime = MPI_Wtime();     /* end time */

													 /* calculate round trip time and print */
					auto deltaT = _endTime - _startTime;
					{
						//deltaT /= CLOCKS_PER_SEC; // To Milliseconds; 1 microsecond=0,001ms
					}

					printf("%4d  %8.8f  %8.8f  %2.8f\n", i, _startTime, _endTime, deltaT);
					sumT += deltaT;
				}
			}
			avgT = (sumT * 1000000) / reps;
			printf("***************************************************\n");
			printf("\n*** Avg round trip time = %d microseconds\n", avgT);
			printf("*** Avg one way latency = %d microseconds\n", avgT / 2);
		}

		else if (rank == 1)
		{
			printf("task %d has started...\n", rank);
			dest = 0;
			source = 0;

			for (auto i = 1; i <= reps; i++)
			{
				for (auto j = 1; j < numtasks; j++)
				{
					rc = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);

					if (rc != MPI_SUCCESS)
					{
						printf("Receive error in task 1!\n");
						MPI_Abort(MPI_COMM_WORLD, rc);

						exit(1);
						return;
					}

					rc = MPI_Send(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);

					if (rc != MPI_SUCCESS)
					{
						printf("Send error in task 1!\n");
						MPI_Abort(MPI_COMM_WORLD, rc);

						exit(1);
						return;
					}
				}
			}
		}
	}
}

int main(int argc, char*argv[])
{
	int reps,                   /* number of samples per test */
		tag,                    /* MPI message tag parameter */

		numtasks,               /* number of MPI tasks */

		rank,                   /* my MPI task number */
		dest, source,           /* send/receive task designators */
		avgT,                   /* average time per rep in microseconds */
		rc,                     /* return code */
		n;
	double T1, T2,              /* start/end times per rep */
		sumT,                   /* sum of all reps times */
		deltaT;                 /* time for one rep */
	char msg;                   /* buffer containing 1 byte message */
	MPI_Status status;          /* MPI receive routine parameter */

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0 && numtasks != 2) {
		printf("Number of tasks = %d\n", numtasks);
		printf("Only need 2 tasks - extra will be ignored...\n");
	}

	// �������
	MPI_Barrier(MPI_COMM_WORLD);

	/*
	NODE-1:
	rank<=0
	size<=N

	NODE-2:
	rank<=1
	size<=N

	NODE-N:
	rank<=N-1
	size<=N
	*/

	sumT = 0;
	msg = 'x';
	tag = 1;
	reps = NUMBER_REPS;

	Benchmarking_1::func(rank, dest, source, reps, rc, sumT, avgT, msg, status, tag, numtasks);


	if (rank == 0) {
		//printf("\n TASK FINISHED \r\n");

		// ��� ������ - � 2 �����

		{
			auto sumTOneWay = sumT;
			/*sumTOneWay /= 2.0* (numtasks - 1)*reps;*/
			sumTOneWay /= 2;
			printf("Latency of channel is %2.8f seconds (One way %2.8f seconds), found in %d iterations\n", sumT, /*sumT / 2*/sumTOneWay, reps);
		}
		//avgT = (sumT * 1000000) / reps;
		//printf("***************************************************\n");
		//printf("\n*** Avg round trip time = %d microseconds\n", avgT);
		//printf("*** Avg one way latency = %d microseconds\n", avgT / 2);
	}

	MPI_Finalize();
	exit(0);
	return 0;
}