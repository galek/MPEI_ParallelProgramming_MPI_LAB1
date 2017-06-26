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
#define SAFE_DELETE(x) if(x) { delete x; x=nullptr;};
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) if(x) { delete []x; x=nullptr;};
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

	int GetMaximalElementInBounds(double *_matrix, int _lowerBound, int _upperBound)
	{
		double max = _matrix[_lowerBound];
		int maxIndex = _lowerBound;

		for (int i = _lowerBound + 1; i < _upperBound; i++)
		{
			if (max < _matrix[i])
			{
				max = _matrix[i];
				maxIndex = i;
			}
		}
		return maxIndex;
	}

	inline void SwapRows(int _equationCount, int _columnsPerNode, int _firstRow, int _secondRow, double *_matrix)
	{
		double buf;
		int mainRowElementIndex;
		int secondRowElementIndex;

		//column
		for (int j = 0; j < _columnsPerNode; j++)
		{
			mainRowElementIndex = GetIndexInLocalMatrix(_equationCount, _firstRow, j);
			secondRowElementIndex = GetIndexInLocalMatrix(_equationCount, _secondRow, j);

			buf = _matrix[mainRowElementIndex];

			_matrix[mainRowElementIndex] = _matrix[secondRowElementIndex];

			_matrix[secondRowElementIndex] = buf;
		}
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

	void CalcMultipliersForConvertationOfMatrix(int _mainRow, int _mainColumn, int _equationCount, double *_matrix, double *_multipliers) {
		int mainIndex = GetIndexInLocalMatrix(_equationCount, _mainRow, _mainColumn);
		//row
		for (int i = _mainRow + 1; i < _equationCount; i++)
		{
			_multipliers[i] = _matrix[GetIndexInLocalMatrix(_equationCount, i, _mainColumn)] / _matrix[mainIndex];
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

	struct MatrixMinor
	{
		int m_SelectedRow;
		int m_SeletedRowForSwappingWithMain;
	};

	// Элементарные преобразования на прямом пути - вычитает из строк элементы главной строки*на множители
	inline void MatrixModificateOnDirectWay(int _equationCount, int _columnsPerNode, int _mainRow, double *_matrix, double *multipliers) {
		//row
		for (int i = _mainRow + 1; i < _equationCount; ++i)
		{
			if (multipliers[i] != 0)
			{
				for (int j = 0; j < _columnsPerNode; ++j)
				{
					int indexInMatrix = GetIndexInLocalMatrix(_equationCount, i, j);
					int mainIndexInColumn = GetIndexInLocalMatrix(_equationCount, _mainRow, j);

					_matrix[indexInMatrix] = _matrix[indexInMatrix] - _matrix[mainIndexInColumn] * multipliers[i];
				}
			}
		}
	}

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

	inline void DirectWay(double*_matrix, int _worldSize, int _columnsPerNode, int _equationCount, int _rank, double _timeAfterGenerating)
	{
		//Прямой ход метода Гаусса, приведение к треугольному виду.
		MatrixMinor minor;

		for (int activeNode = 1; activeNode < _worldSize; ++activeNode)
		{
			//column
			for (int j = 0; j < _columnsPerNode; ++j)
			{
				if (_rank == activeNode)
				{
					int mainRow = _columnsPerNode * (activeNode - 1) + j;
					int mainIndex = GetIndexInLocalMatrix(_equationCount, mainRow, j);
					int upperBound = GetIndexInLocalMatrix(_equationCount, _equationCount, j);
					int maxIndex = GetMaximalElementInBounds(_matrix, mainIndex, upperBound);

					minor.m_SelectedRow = mainRow;
					minor.m_SeletedRowForSwappingWithMain = mainRow + maxIndex - mainIndex;
				}

				MPI_Bcast(&minor, 1, MPI_2INT, activeNode, MPI_COMM_WORLD);
				if (minor.m_SelectedRow != minor.m_SeletedRowForSwappingWithMain)
				{
					SwapRows(_equationCount, IsMain(_rank) ? 1 : _columnsPerNode, minor.m_SelectedRow, minor.m_SeletedRowForSwappingWithMain, _matrix);
				}

				// запоминаем главную строку
				int mainRow = minor.m_SelectedRow;

				double *multipliers = new double[_equationCount];

				if (_rank == activeNode)
				{
					CalcMultipliersForConvertationOfMatrix(mainRow, j, _equationCount, _matrix, multipliers);
				}

				MPI_Bcast(multipliers, _equationCount, MPI_DOUBLE, activeNode, MPI_COMM_WORLD);

				MatrixModificateOnDirectWay(_equationCount, IsMain(_rank) ? 1 : _columnsPerNode, mainRow, _matrix, multipliers);

				SAFE_DELETE(multipliers);
			}
		}

		double _EndTimeOfDirectRound = MPI_Wtime();
		if (IsMain(_rank))
		{
			printf("Direct round time = %f\n", _EndTimeOfDirectRound - _timeAfterGenerating);
		}
	}

	// _row- вне
	inline void RevertWay(double*_row, double*_solution, int _rank, int _equationCount, int _columnsPerNode, int _worldSize)
	{
		if (IsMain(_rank))
		{
			_solution = new double[_equationCount];
			_row = new double[_equationCount];
		}
		else
		{
			_row = new double[_columnsPerNode];
		}

		int *_dataMap = new int[_worldSize];
		int *_receiveOffsets = new int[_worldSize];


		for (int nonMasterNode = 1; nonMasterNode < _worldSize; ++nonMasterNode)
		{
			_dataMap[nonMasterNode] = _columnsPerNode;
			_receiveOffsets[nonMasterNode] = _columnsPerNode * (nonMasterNode - 1);
		}
	}

}
using namespace MatrixCompute;
using namespace Utils;

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



	double *matrix /*= nullptr*/;
	double _DeltaTime = 0;

	/*MatrixGeneration*/
	//{
	auto _TimeStart = MPI_Wtime();
	matrix = new double[_OneBlockSize];
	GenerateMatrix(matrix, rank, worldSize, _EquationCount, _OneBlockSize, status);
	auto _TimeEnd = MPI_Wtime();
	_DeltaTime = _TimeEnd - _TimeStart;
	//}
	if (IsMain(rank)) {
		printf(" GENERATION TIME %f\n", _DeltaTime);
	}

	/*Синхронизируемся*/
	MPI_Barrier(MPI_COMM_WORLD);

	/*Прямой ход-гоним к треугольному виду*/
#if 0
	DirectWay(matrix, worldSize, _ColumnsPerNode, _EquationCount, rank, _DeltaTime);

	/*Обратный ход - гоним обратный метод гаусса*/

	double *solution = nullptr;
	double *row = nullptr;
	RevertWay(row, solution, rank, _EquationCount, _ColumnsPerNode, worldSize);

#else
	   //Прямой ход метода Гаусса, приведение к треугольному виду.
	MatrixMinor pair;
	int activeNode;
	for (activeNode = 1; activeNode < worldSize; ++activeNode)
	{
		int column;
		for (column = 0; column < _ColumnsPerNode; ++column)
		{
			if (rank == activeNode)
			{
				int mainRow = _ColumnsPerNode * (activeNode - 1) + column;
				int mainIndex = GetIndexInLocalMatrix(_EquationCount, mainRow, column);
				int upperBound = GetIndexInLocalMatrix(_EquationCount, _EquationCount, column);
				int maxIndex = GetMaximalElementInBounds(matrix, mainIndex, upperBound);
				pair.m_SelectedRow = mainRow;
				pair.m_SeletedRowForSwappingWithMain = mainRow + maxIndex - mainIndex;
			}

			MPI_Bcast(&pair, 1, MPI_2INT, activeNode, MPI_COMM_WORLD);
			if (pair.m_SelectedRow != pair.m_SeletedRowForSwappingWithMain)
			{
				SwapRows(_EquationCount, IsMain(rank) ? 1 : _ColumnsPerNode, pair.m_SelectedRow, pair.m_SeletedRowForSwappingWithMain, matrix);
			}

			//запоминаем главную строку
			int mainRow = pair.m_SelectedRow;

			double *multipliers = (double*)calloc((size_t)_EquationCount, sizeof(double));

			if (rank == activeNode)
			{
				CalcMultipliersForConvertationOfMatrix(mainRow, column, _EquationCount, matrix, multipliers);
			}

			MPI_Bcast(multipliers, _EquationCount, MPI_DOUBLE, activeNode, MPI_COMM_WORLD);
			MatrixModificateOnDirectWay(_EquationCount, IsMain(rank) ? 1 : _ColumnsPerNode, mainRow, matrix, multipliers);
			free(multipliers);
		}
	}

	double timeAfterDirectRound = MPI_Wtime();
	if (IsMain(rank))
	{
		printf("Time for direct round = %f\n", timeAfterDirectRound - _DeltaTime);
	}

	//=====================================================
	// ВОТ ТУТ Я НЕ УВЕРЕН

	////Матрица треугольная, обратный ход метода Гаусса.
	double *solution = NULL;
	double *row;
	if (IsMain(rank)) {
		solution = (double*)calloc((size_t)_EquationCount, sizeof(double));
		row = (double*)calloc((size_t)_EquationCount, sizeof(double));
	}
	else {
		row = (double*)calloc((size_t)_ColumnsPerNode, sizeof(double));
	}

	int *dataMap = (int*)calloc((size_t)worldSize, sizeof(int));
	int *receiveOffsets = (int*)calloc((size_t)worldSize, sizeof(int));
	int nonMasterNode;

	for (nonMasterNode = 1; nonMasterNode < worldSize; ++nonMasterNode)
	{
		dataMap[nonMasterNode] = _ColumnsPerNode;
		receiveOffsets[nonMasterNode] = _ColumnsPerNode * (nonMasterNode - 1);
	}

	//=====================================================
#endif

	if (IsMain(rank)) {
		printf(" RESULT CALCULATION ON NEXT STEP \n");
	}

	{
		int equation;
		for (equation = _EquationCount - 1; equation >= 0; --equation) {
			if (!IsMain(rank)) {
				int column;
				for (column = 0; column < _ColumnsPerNode; ++column) {
					row[column] = matrix[GetIndexInLocalMatrix(_EquationCount, equation, column)];
				}
			}
			int i;
			MPI_Gatherv(row, _ColumnsPerNode, MPI_DOUBLE, row, dataMap, receiveOffsets, MPI_DOUBLE, MAIN_RANK,
				MPI_COMM_WORLD);
			if (IsMain(rank)) {
				solution[equation] = matrix[equation];
				int solutionIndex;
				for (solutionIndex = _EquationCount - 1; solutionIndex > equation; --solutionIndex) {
					solution[equation] -= solution[solutionIndex] * row[solutionIndex];
				}
				solution[equation] = solution[equation] / row[equation];
			}
		}

		double timeAfterReverseRound = MPI_Wtime();
		if (IsMain(rank)) {
			printf("Time to get solution = %f\n", timeAfterReverseRound - timeAfterDirectRound);
		}
		if (IsMain(rank)) {
			printf("Result is : [");
			for (equation = 0; equation < _EquationCount; ++equation) {
				printf(" %f,", solution[equation]);
			}
			printf("]\n");
			free(solution);
		}
		free(row);
		free(dataMap);
		free(receiveOffsets);
		free(matrix);
	}


	MPI_Finalize();
	return 0;
}