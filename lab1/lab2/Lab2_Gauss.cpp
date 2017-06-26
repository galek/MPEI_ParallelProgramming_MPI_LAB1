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

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) if(x) { delete x; x=nullptr;}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) if(x) { delete []x; x=nullptr;}
#endif


namespace Utils
{
	static inline bool IsMain(int _rank) {
		return _rank == 0;
	}

	static const int MAIN_RANK = 0;
	static const int ACTION_RANK = 1;

	inline double CreateRandNorm() { return rand() / (double)RAND_MAX; }

	inline void InsertResultArrayToMatrix(int _equationCount, double *_matrix)
	{
		for (int row = 0; row < _equationCount; ++row) {
			_matrix[row] = CreateRandNorm() * 20 - 10;
		}
	}

	inline int GetIndexInLocalMatrix(int _equationCount, int _row, int _column) {
		return _row + _equationCount * _column;
	}

	inline void CreateColumnBlock(int _numberOfEquations, int _worldSize, int _index, double *_block)
	{
		int columnCount = _numberOfEquations / (_worldSize - 1);
		int column;
		int row;
		int mainIndex;
		int currentIndex;
		double normalizedRandom;

		for (column = 0; column < columnCount; ++column)
		{
			mainIndex = GetIndexInLocalMatrix(_numberOfEquations, _index * columnCount + column, column);
			for (row = 0; row < _numberOfEquations; ++row)
			{
				currentIndex = GetIndexInLocalMatrix(_numberOfEquations, row, column);
				normalizedRandom = CreateRandNorm();

				_block[currentIndex] = currentIndex == mainIndex
					? 10 + normalizedRandom * 100
					: normalizedRandom * 10;
			}
		}
	}

	// TODO: REPLACE ON ANOTHER CONSTANT
	inline int GetMinEquatCount(int dataTypeSizeInBytes, int numberOfComputationalNodes, int minimumSizeOfSystemPerNode)
	{
		return 5000;
	}

	inline void PrintMatrix(int _rowCount, int _columnCount, double *_matrix) {
		printf("[\n");
		for (int row = 0; row < _rowCount; ++row)
		{
			printf("[");
			{
				for (int column = 0; column < _columnCount; ++column)
				{
					printf(" %f,", _matrix[GetIndexInLocalMatrix(_rowCount, row, column)]);
				}
			}
			printf("]\n");
		}
		printf("]\n");
	}


	inline int GetCountOfEq(int worldSize)
	{
		int doubleSize = sizeof(double);
		int ram100mb = 100 * 1024 * 1024;
		int minValue = GetMinEquatCount(doubleSize, worldSize - 1, ram100mb);
		return minValue - minValue % (worldSize - 1) + worldSize - 1;
	}

}


namespace MatrixCompute
{
	using namespace Utils;

	struct MatrixState
	{
		int m_SelectedRow;
		int m_SeletedRowForSwappingWithMain;
	};

	inline void GenerateMatrix(double*_matrix, int _rank, int _worldSize, int _equationCount, int _oneBlockSize, MPI_Status&_status)
	{
		if (IsMain(_rank))
		{
			printf("Starting the sending of data\n");

			srand((unsigned int)time(0));

			for (int node = 1; node < _worldSize; ++node)
			{
				CreateColumnBlock(_equationCount, _worldSize, node - 1, _matrix);
				MPI_Send(_matrix, _oneBlockSize, MPI_DOUBLE, node, ACTION_RANK, MPI_COMM_WORLD);
			}

			SAFE_DELETE_ARRAY(_matrix);

			_matrix = new double[_equationCount];
			InsertResultArrayToMatrix(_equationCount, _matrix);
		}
		else
		{
			MPI_Recv(_matrix, _oneBlockSize, MPI_DOUBLE, MAIN_RANK, ACTION_RANK, MPI_COMM_WORLD, &_status);
		}
	}

}
using namespace MatrixCompute;


int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	// Число процессов
	int worldSize = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	// Ранг текущего процесса
	int rank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	MPI_Status status;

	if (worldSize < 2) {
		printf("World is too small\n");
		return 1;
	}

	/*Число уравнений*/
	int _EquationCount;
	/*Число столбцов на 1 участок кластера*/
	int _ColumnsPerNode;
	/*Размер 1 блока - задача занять весь класстер*/
	int _OneBlockSize;

	if (IsMain(rank))
	{
		printf("World size is %d\n", worldSize);

		_EquationCount = GetCountOfEq(worldSize);
		_ColumnsPerNode = _EquationCount / (worldSize - 1);
		_OneBlockSize = _EquationCount * _ColumnsPerNode;
		printf("Equat size %d\n with %d per node\n with size of %d\n", _EquationCount, _ColumnsPerNode, _OneBlockSize);
	}

	MPI_Bcast(&_EquationCount, 1, MPI_INT, MAIN_RANK, MPI_COMM_WORLD);
	MPI_Bcast(&_ColumnsPerNode, 1, MPI_INT, MAIN_RANK, MPI_COMM_WORLD);
	MPI_Bcast(&_OneBlockSize, 1, MPI_INT, MAIN_RANK, MPI_COMM_WORLD);



	double *matrix = nullptr;
	double _DeltaTime = 0;

	/*MatrixGeneration*/
	{
		auto _TimeStart = MPI_Wtime();
		matrix = new double[_OneBlockSize];
		GenerateMatrix(matrix, rank, worldSize, _EquationCount, _OneBlockSize, status);
		auto _TimeEnd = MPI_Wtime();
		_DeltaTime = _TimeEnd - _TimeStart;
	}


	if (IsMain(rank)) {
		printf(" GENERATION TIME %f\n", _DeltaTime);
	}

	/*Синхронизируемся*/
	MPI_Barrier(MPI_COMM_WORLD);


	MPI_Finalize();
	return 0;
}