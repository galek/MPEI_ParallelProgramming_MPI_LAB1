// lab1.cpp: определяет точку входа для консольного приложения.
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
3.	Сравнить производительность функций MPI_Reduce и MPI_Send/MPI_Recv при разном количестве задействованных узлов.
*/

namespace Benchmarking
{
	enum class MODE
	{
		LATENCY = 0,
		BCAST_SENDRECV,
		REDUCE
	};


	void Task1(int reps, double &SumTimeDelta, int size, int rank, MPI_Status& status, double&BCastTimeDelta, double&ReduceTimeDelta, MODE _compare)
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
					int rc = MPI_Send(nullptr, 0, MPI_BYTE, j, 1, MPI_COMM_WORLD);

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
				auto rc = MPI_Recv(nullptr, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
				if (rc != MPI_SUCCESS)
				{
					printf("Receive error in task 0!\n");
					MPI_Abort(MPI_COMM_WORLD, rc);
					exit(1);
				}
			}
		}

		{
			if ((rank == 0) && (_compare == MODE::BCAST_SENDRECV))
			{
				auto _startTime = MPI_Wtime();     /* start time */

				for (int i = 0; i < reps; i++)
				{
					MPI_Bcast(nullptr, 0, MPI_BYTE, 0, MPI_COMM_WORLD);
				}

				auto _endTime = MPI_Wtime();     /* end time */

												 /* calculate round trip time and print */
				auto deltaT = _endTime - _startTime;
				BCastTimeDelta += deltaT;
			}
		}

	}

	/*
	3.	Сравнить производительность функций MPI_Reduce и MPI_Send/MPI_Recv при разном количестве задействованных узлов.
	*/
	// Просто считает сумму, если рангу узла
	void ReduceTask(int reps, double &SumTimeDelta, int size, int rank, MPI_Status& status, double&BCastTimeDelta, double&ReduceTimeDelta, MODE _compare)
	{
		{
			int globalsum = 0;

			if (rank == 0)
			{
				auto _startTime = MPI_Wtime();     /* start time */

				int receive;

				//sending
				for (int i = 0; i < reps; ++i) {
					globalsum = 0;
					for (int j = 1; j < size; j++) {
						MPI_Recv(&receive, 1, MPI_INT, j, 1, MPI_COMM_WORLD, &status);
						globalsum += receive;
					}
				}

				auto _endTime = MPI_Wtime();     /* end time */


				 /* calculate round trip time and print */
				auto deltaT = _endTime - _startTime;

				SumTimeDelta += deltaT;

		}
			else {

				int localsum = 0;
				if (rank % 2 == 1)
				{
					localsum += 5;
				}
				else if (rank > 0 && (rank % 2 == 0))
				{
					localsum += 10;
				}

				//sending
				for (int i = 0; i < reps; i++) {
					MPI_Send(&localsum, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
				}
			}

			if (rank == 0)
				printf("SEND_RECV globalsum = %i \n", globalsum);
	}


		{
			if ((_compare == MODE::REDUCE))
			{
				MPI_Barrier(MPI_COMM_WORLD);

				auto _startTime = MPI_Wtime();     /* start time */

												   //{

												   //}
												   //int receive = 0;

												   //for (int sending = 0; sending < 11; ++sending)
												   //{
												   //	printf("1\n");
												   //	MPI_Reduce(&rank, &receive, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
												   //}



												   // Works
				int localsum = 0;
				int globalsum = 0;

				if (rank % 2 == 1)
				{
					localsum += 5;
				}
				else if (rank > 0 && (rank % 2 == 0))
				{
					localsum += 10;
				}

				// TODO: Переписать общую сумму
				MPI_Reduce(&localsum, &globalsum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

				if (rank == 0)
				{
					printf("globalsum = %d \n", globalsum);
				}

				auto _endTime = MPI_Wtime();     /* end time */

												 /* calculate round trip time and print */
				auto deltaT = _endTime - _startTime;
				ReduceTimeDelta += deltaT;

			}
		}
}

	inline void RunTask2(int _count, double&SumTimeDelta, double&BCastTimeDelta, double&ReduceTimeDelta, int size, int rank, MPI_Status& status)
	{
		Task1(_count, SumTimeDelta, size, rank, status, BCastTimeDelta, ReduceTimeDelta, MODE::BCAST_SENDRECV);
	}

	inline void RunTask1(int _count, double&SumTimeDelta, double&BCastTimeDelta, double&ReduceTimeDelta, int size, int rank, MPI_Status& status)
	{
		Task1(_count, SumTimeDelta, size, rank, status, BCastTimeDelta, ReduceTimeDelta, MODE::LATENCY);
	}

	/*
	3.	Сравнить производительность функций MPI_Reduce и MPI_Send/MPI_Recv при разном количестве задействованных узлов.
	*/
	inline void RunTask3(int _count, double&SumTimeDelta, double&BCastTimeDelta, double&ReduceTimeDelta, int size, int rank, MPI_Status& status)
	{
		ReduceTask(_count, SumTimeDelta, size, rank, status, BCastTimeDelta, ReduceTimeDelta, MODE::REDUCE);
	}
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

	// Расчеты
	MPI_Barrier(MPI_COMM_WORLD);

	sumT = 0;
	tag = 1;

	double SumTimeDelta = 0;
	double BCastTimeDelta = 0;
	double ReduceTimeDelta = 0;


	/*Количество измерений*/
	int Count = 10;
	{
		using namespace Benchmarking;
		{
			//RunTask2(Count, SumTimeDelta, BCastTimeDelta, ReduceTimeDelta, size, rank, status);
		}
		{
			RunTask3(Count, SumTimeDelta, BCastTimeDelta, ReduceTimeDelta, size, rank, status);
		}
	}

	if (rank == 0)
	{
		printf("Send/Recv time %f ms for operations %i Performance %f operations for per-ms \n", SumTimeDelta*(1000000 / CLOCKS_PER_SEC), Count, SumTimeDelta*(1000000 / CLOCKS_PER_SEC) / Count);
		printf("BCast time %f ms for operations %i Performance %f operations for per-ms \n", BCastTimeDelta*(1000000 / CLOCKS_PER_SEC), Count, SumTimeDelta *(1000000 / CLOCKS_PER_SEC) / Count);
		printf("ReduceTimeDelta time %f ms for operations %i Performance %f operations for per-ms \n", ReduceTimeDelta*(1000000 / CLOCKS_PER_SEC), Count, ReduceTimeDelta *(1000000 / CLOCKS_PER_SEC) / Count);
	}

	MPI_Finalize();
	exit(0);
	return 0;
}