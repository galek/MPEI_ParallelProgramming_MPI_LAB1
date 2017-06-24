// lab1.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
//#include <sys/time.h>
#include <time.h>
#include <iostream>
#include "windows.h"

#define	NUMBER_REPS	1000

#pragma comment(lib, "msmpi.lib")
#pragma comment(lib, "Ws2_32.lib")

/*
1.	Измерение латентности и пропускной способности каналов обмена данными при использовании функций MPI. Задачи:
1.	Вычислить латентность и пропускную способность при передаче данных с помощью функций MPI_Send/MPI_Recv. Учесть, что обмен при выполнении программы может на самом деле происходить как между процессорами одного хоста, так и между хостами.
2.	Сравнить производительность функций MPI_Bcast и MPI_Send/MPI_Recv при разном количестве задействованных узлов.
3.	Сравнить производительность функций MPI_Reduce и MPI_Send/MPI_Recv при разном количестве задействованных узлов.
*/


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

	// Расчеты

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

#if 0
	//{
	//	if (rank == 0) {
	//		/* round-trip latency timing test */
	//		printf("task %d has started...\n", rank);
	//		printf("Beginning latency timing test. Number of reps = %d.\n", reps);
	//		printf("***************************************************\n");
	//		printf("Rep#       T1               T2            deltaT\n");
	//		dest = 1;
	//		source = 1;
	//		for (n = 1; n <= reps; n++) {
	//			T1 = MPI_Wtime();     /* start time */
	//								  /* send message to worker - message tag set to 1.  */
	//								  /* If return code indicates error quit */
	//			rc = MPI_Send(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
	//			if (rc != MPI_SUCCESS) {
	//				printf("Send error in task 0!\n");
	//				MPI_Abort(MPI_COMM_WORLD, rc);
	//				exit(1);
	//			}
	//			/* Now wait to receive the echo reply from the worker  */
	//			/* If return code indicates error quit */
	//			rc = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD,
	//				&status);
	//			if (rc != MPI_SUCCESS) {
	//				printf("Receive error in task 0!\n");
	//				MPI_Abort(MPI_COMM_WORLD, rc);
	//				exit(1);
	//			}
	//			T2 = MPI_Wtime();     /* end time */

	//								  /* calculate round trip time and print */
	//			deltaT = T2 - T1;
	//			printf("%4d  %8.8f  %8.8f  %2.8f\n", n, T1, T2, deltaT);
	//			sumT += deltaT;
	//		}
	//		avgT = (sumT * 1000000) / reps;
	//		printf("***************************************************\n");
	//		printf("\n*** Avg round trip time = %d microseconds\n", avgT);
	//		printf("*** Avg one way latency = %d microseconds\n", avgT / 2);
	//	}

	//	else if (rank == 1) {
	//		printf("task %d has started...\n", rank);
	//		dest = 0;
	//		source = 0;
	//		for (n = 1; n <= reps; n++) {
	//			rc = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD,
	//				&status);
	//			if (rc != MPI_SUCCESS) {
	//				printf("Receive error in task 1!\n");
	//				MPI_Abort(MPI_COMM_WORLD, rc);

	//				exit(1);
	//				return 1;
	//			}
	//			rc = MPI_Send(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
	//			if (rc != MPI_SUCCESS) {
	//				printf("Send error in task 1!\n");
	//				MPI_Abort(MPI_COMM_WORLD, rc);

	//				exit(1);
	//				return 1;
	//			}
	//		}
	//	}
	//}
#else
	{
		using namespace std;

		char host[256];
		gethostname(host, sizeof(host) / sizeof(host[0])); // machine we are running on 

		cout << "Process " << rank << " of " << numtasks << " is running on '" << host << "'." << endl;


	}
#endif

	if (rank == 0) {
		printf("\n TASK FINISHED \r\n");
	}

	MPI_Finalize();
	exit(0);
	return 0;
}

