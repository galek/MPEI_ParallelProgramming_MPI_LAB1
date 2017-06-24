// lab1.cpp: определяет точку входа для консольного приложения.
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

#define SAFE_DELETE_ARRAY(x) if(x) delete[]x;
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

/*
1.	Измерение латентности и пропускной способности каналов обмена данными при использовании функций MPI. Задачи:
1.	Вычислить латентность и пропускную способность при передаче данных с помощью функций MPI_Send/MPI_Recv. Учесть, что обмен при выполнении программы может на самом деле происходить как между процессорами одного хоста, так и между хостами.
2.	Сравнить производительность функций MPI_Bcast и MPI_Send/MPI_Recv при разном количестве задействованных узлов.
3.	Сравнить производительность функций MPI_Reduce и MPI_Send/MPI_Recv при разном количестве задействованных узлов.
*/

namespace Benchmarking_1
{
	void func(int reps, int size, int bytessize, int rank, MPI_Status& status)
	{
		double sumT = 0;

		char* msg = new char[bytessize];


		for (auto i = 0; i < reps; i++)
		{
			if (rank == 0)
			{
				for (auto j = 1; j < size; j++)
				{
					auto _startTime = MPI_Wtime();     /* start time */
													   /* send message to worker - message tag set to 1.  */
													   /* If return code indicates error quit */
					int rc = MPI_Send(msg, bytessize, MPI_BYTE, j, 1, MPI_COMM_WORLD);

					if (rc != MPI_SUCCESS)
					{
						printf("Send error in task 0!\n");
						MPI_Abort(MPI_COMM_WORLD, rc);
						exit(1);
					}

					/* Now wait to receive the echo reply from the worker  */
					/* If return code indicates error quit */
					rc = MPI_Recv(msg, bytessize, MPI_BYTE, j, 2, MPI_COMM_WORLD, &status);

					if (rc != MPI_SUCCESS)
					{
						printf("Receive error in task 0!\n");
						MPI_Abort(MPI_COMM_WORLD, rc);
						exit(1);
					}

					auto _endTime = MPI_Wtime();     /* end time */

					/* calculate round trip time and print */
					auto deltaT = _endTime - _startTime;

					sumT += deltaT;
				}
			}
			else
			{
				MPI_Recv(msg, bytessize, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
				MPI_Send(msg, bytessize, MPI_BYTE, 0, 2, MPI_COMM_WORLD);
			}
		}

		{
			SAFE_DELETE_ARRAY(msg);
			if (rank == 0) {
				double cap = 2.0*reps*bytessize*(size - 1) / sumT;
				printf("Channel capacity = %.0f bytes per second, found in %d repeats, data length = %d bytes\n", cap, reps, bytessize);
			}
		}
	}
}

int main(int argc, char*argv[])
{
	//int _len;
	//if (sscanf(argv[0], "%i", &_len) != 1) {
	//	fprintf(stderr, "error - not an integer");
	//	return 1;
	//}



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

	int CapacityReps = 10;/*Количество измерений*/
	int MBSize = 1; /*Размер МБ*/
	int CapacityDataSize = MBSize * 1024 * 1024;

	double TimeStart = 0;
	double TimeDelta = 0;

	printf("Capacity checking Started \n");
	for (int i = 1; i <= 8; i++) {
		TimeStart = MPI_Wtime();
		Benchmarking_1::func(CapacityReps, size, CapacityDataSize*i, rank, status);
		TimeDelta = MPI_Wtime() - TimeStart;
	}

	if (rank == 0)
		printf("Capacity checking DONE (measuring) %f\n", TimeDelta);

	MPI_Finalize();
	exit(0);
	return 0;
}