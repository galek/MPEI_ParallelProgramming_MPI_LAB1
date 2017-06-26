#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

//http://www2.sscc.ru/Publikacii/Primery_Prll/Primery.htm

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) if(x){delete x; x=nullptr;}
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) if(x){delete []x; x=nullptr;}
#endif

struct Compute
{
	~Compute()
	{
		SAFE_DELETE_ARRAY(matrix);
		SAFE_DELETE_ARRAY(vector);
		SAFE_DELETE_ARRAY(Result);
	}

	void Init()
	{
		if (rank_proc == 0)
		{
			_PopulateMatrix();
			printf("\ntime_started!");
		}

		MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

		//Определение размера части данных, расположенных на конкретном процессе

		/*Число строк, ещё не распределённых по процессам*/
		int tmp_num_str = size;

		for (int i = 0; i < rank_proc; i++) {
			tmp_num_str -= tmp_num_str / (g_NumProc - i);
		}

		m_CountOfStrings = tmp_num_str / (g_NumProc - rank_proc);

		{
			tmp_matrix = new double[m_CountOfStrings*size];//выделяем память под строки матрицы
			tmp_vector = new double[m_CountOfStrings]; //память под элементы столбца свободных членов
			tmp_res = new double[m_CountOfStrings]; //память под элементы вектора результата

			NumsMainLines = new int[size]; //массив индексов ведущих строк системы на каждой итерации
			NumsMainLinesIter = new int[m_CountOfStrings]; /* итерация, на которой соответствующая строка системы, расположенная на процессе, выбрана  ведущей */

			_arrayCountOfProcesses = new int[g_NumProc];
			range = new int[g_NumProc];
		}

		for (int i = 0; i < m_CountOfStrings; i++) {
			NumsMainLinesIter[i] = -1;
		}

		/*Распределеяем задачи между процессами*/
		_TaskDistribution();
	}

	/**/
	void RunGauss()
	{
		_ForwardGauss();
		_BackGauss();

		//сбор данных, передача от всех одному (процессу c корнем 0)
		MPI_Gatherv(tmp_res, range[rank_proc], MPI_DOUBLE, Result, range, _arrayCountOfProcesses, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}

private:
	/*Заполняем матрицу изначальными значениями*/
	void _PopulateMatrix()
	{
		size = 7500;
		srand(p);

		/**/
		{
			matrix = new double[size*size];
			vector = new double[size];
			Result = new double[size];
		}

		/*Заполняем случайными значениями*/
		for (int i = 0; i < size*size; i++)
		{
			matrix[i] = rand();
		}

		/*Заполняем случайными значениями*/
		for (int i = 0; i < size; i++)
		{
			vector[i] = rand();
			/*Результирующий вектор по 0*/
			Result[i] = 0;
		}
	}

	/*Вычисляем множители и вычитаем главную строку*на множитель*/
	void _ExceptRaws(int iter_numb, double *glob_str)
	{
		double koef;
		/**/
		for (int i = 0; i < m_CountOfStrings; i++)//для каждой строки в процессе
		{
			if (NumsMainLinesIter[i] == -1)
			{
				koef = tmp_matrix[i*size + iter_numb] / glob_str[iter_numb];

				for (int j = iter_numb; j < size; j++)
				{
					tmp_matrix[i*size + j] -= glob_str[j] * koef;
				}

				tmp_vector[i] -= glob_str[size] * koef;
			}
		}
	}

	// Распределение исходных данных между процессами
	void _TaskDistribution()
	{
		/*Index of first element of matrixes what will sended to process*/
		int *index_to_matr;
		/*Count of elements of matrix what will sended to process*/
		int *range_to_proc;

		int _numProcStr;

		int tmp_num_str;

		int prev_sz;
		int prev_indx;

		int portion;

		index_to_matr = new int[g_NumProc];
		range_to_proc = new int[g_NumProc];

		tmp_num_str = size;

		for (int i = 0; i < g_NumProc; i++)  //Определяем, сколько элементов матрицы будет передано каждому процессу
		{
			prev_sz = (i == 0) ? 0 : range_to_proc[i - 1];

			prev_indx = (i == 0) ? 0 : index_to_matr[i - 1];

			//число строк, отданных предыдущему процессу
			portion = (i == 0) ? 0 : _numProcStr;

			tmp_num_str -= portion;

			_numProcStr = tmp_num_str / (g_NumProc - i);

			range_to_proc[i] = _numProcStr*size;

			index_to_matr[i] = prev_indx + prev_sz;
		}

		// Разошлем матрицы каждому процессу
		MPI_Scatterv(matrix, range_to_proc, index_to_matr, MPI_DOUBLE, tmp_matrix, range_to_proc[rank_proc], MPI_DOUBLE, 0, MPI_COMM_WORLD);

		//Разослали
		tmp_num_str = size;

		for (int i = 0; i < g_NumProc; i++)
		{
			int prev_sz = (i == 0) ? 0 : range[i - 1];
			int prev_indx = (i == 0) ? 0 : _arrayCountOfProcesses[i - 1];
			int portion = (i == 0) ? 0 : range[i - 1];
			tmp_num_str -= portion;
			range[i] = tmp_num_str / (g_NumProc - i);
			_arrayCountOfProcesses[i] = prev_indx + prev_sz;
		}

		{
			//Рассылка вектора
			/*
			Рассылается:
			-массив с количествами пересылаемых элеметов на каждый процесс
			-массив с номерами элеметов, с которых надо начинать рассылку из vector

			Принимается:
			Количество элементов на каждый процесс
			*/
			MPI_Scatterv(vector, range, _arrayCountOfProcesses, MPI_DOUBLE, tmp_vector, range[rank_proc], MPI_DOUBLE, 0, MPI_COMM_WORLD);
			status = 1;
			MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
			MPI_Bcast(&m, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		}

		SAFE_DELETE_ARRAY(range_to_proc);
		SAFE_DELETE_ARRAY(index_to_matr);
	}

	void _ForwardGauss()
	{
		int VedIndex;   // индекс ведущей строки на конкретном процессе
		struct { double max_val; int rank_proc; } loc_max, glob_max; //максимальный элемент+номер процесса, у которого он
		double* glb_maxst = new double[size + 1]; //т.е. строка матрицы+значение вектора

		for (int i = 0; i < size; i++) //главный цикл решения
		{
			// Вычисление ведущей строки
			double max_val = 0;
			int tmp_index = -1;

			for (int j = 0; j < m_CountOfStrings; j++)
			{
				tmp_index = j;
				if ((NumsMainLinesIter[j] == -1) && (max_val < fabs(tmp_matrix[i + size*j])))
				{
					max_val = fabs(tmp_matrix[i + size*j]);
					VedIndex = j;
				}
			}

			loc_max.max_val = max_val;
			loc_max.rank_proc = rank_proc;

			//каждый процесс рассылает свой локально максимальный элемент по всем столцам, все процесы принимают уже глобально максимальный элемент
			MPI_Allreduce(&loc_max, &glob_max, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);

			//Вычисление ведущей строки всей системы
			if (rank_proc == glob_max.rank_proc)
			{
				if (glob_max.max_val == 0)
				{
					status = 2;
					MPI_Barrier(MPI_COMM_WORLD);
					MPI_Send(&status, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
					NumsMainLinesIter[tmp_index] = i;
					NumsMainLines[i] = _arrayCountOfProcesses[rank_proc] + VedIndex;
					continue;
				}
				else
				{
					NumsMainLinesIter[VedIndex] = i;  // Номер итерации, на которой строка с локальным номером является ведущей для всей системы
					NumsMainLines[i] = _arrayCountOfProcesses[rank_proc] + VedIndex;   //Вычисленный номер ведущей строки системы
				}
			}
			MPI_Bcast(&NumsMainLines[i], 1, MPI_INT, glob_max.rank_proc, MPI_COMM_WORLD);
			if (rank_proc == glob_max.rank_proc)
			{
				for (int j = 0; j < size; j++) {
					glb_maxst[j] = tmp_matrix[VedIndex*size + j];
				}
				glb_maxst[size] = tmp_vector[VedIndex];
			}
			//Рассылка ведущей строки всем процессам
			MPI_Bcast(glb_maxst, size + 1, MPI_DOUBLE, glob_max.rank_proc, MPI_COMM_WORLD);
			//Исключение неизвестных в столбце с номером i
			_ExceptRaws(i, glb_maxst);
		};
	}

	/*Определяет ранг процесса, содержащего текущую ведущую строку, и номер этой строки на процессе*/
	void Frp(int stringIndex, // номер строки, которая была ведущей на определеной итерации
		int &iterationrank_proc, // процесс, на котором эта строка
		int &IterationItervedindex) // локальный номер этой строки (в рамках одного процесса)
	{
		for (int i = 0; i < g_NumProc - 1; i++) //Определяем ранг процесса, содержащего данную строку
		{
			if ((_arrayCountOfProcesses[i] <= stringIndex) && (stringIndex < _arrayCountOfProcesses[i + 1]))
				iterationrank_proc = i;
		}
		if (stringIndex >= _arrayCountOfProcesses[g_NumProc - 1])
			iterationrank_proc = g_NumProc - 1;
		IterationItervedindex = stringIndex - _arrayCountOfProcesses[iterationrank_proc];
	}

	/*Обратный ход*/
	void _BackGauss()
	{
		int Iterrank_proc;  // Ранг процесса, хранящего текущую ведущую строку
		int IndexVed;    // локальный на своем процессе номер текущей ведущей
		double IterRes;   // значение Xi, найденное на итерации
		double val;

		// Основной цикл
		for (int i = size - 1; i >= 0; i--)
		{
			// Определили ранг процесса, содержащего текущую ведущую строку, и номер этой строки на процессе
			Frp(NumsMainLines[i], Iterrank_proc, IndexVed);

			// Вычисляем значение неизвестной
			if (rank_proc == Iterrank_proc)
			{
				if (tmp_matrix[IndexVed*size + i] == 0)
				{
					if (tmp_vector[IndexVed] == 0)
						IterRes = m;
					else
					{
						status = 0;
						MPI_Barrier(MPI_COMM_WORLD);
						MPI_Send(&status, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
						break;
					}
				}
				else
					IterRes = tmp_vector[IndexVed] / tmp_matrix[IndexVed*size + i];
				tmp_res[IndexVed] = IterRes; //нашли значение переменной
			}

			MPI_Bcast(&IterRes, 1, MPI_DOUBLE, Iterrank_proc, MPI_COMM_WORLD);
			for (int j = 0; j < m_CountOfStrings; j++) //подстановка найденной переменной
				if (NumsMainLinesIter[j] < i)
				{
					val = tmp_matrix[size*j + i] * IterRes;
					tmp_vector[j] -= val;
				}
		}
	}

public:
	// Количество процессов
	int g_NumProc;

	// переменные для засечения времени
	double  startTime, time_end, DeltaTime;
	// Ранг процесса
	int rank_proc;
	//status - статус решения задачи
	int size, p, status;

private:
	int m_CountOfStrings; //m_CountOfStrings-число строк исходной матрицы, хранящихся на конкретном процессе
	double *matrix, *vector, *Result;
	/*Временные переменные*/
	/*tmp_matrix, tmp_vector, tmp_res - куски матрицы, вектора b и вектора x на определенном процессе*/
	double *tmp_matrix, *tmp_vector, *tmp_res, m = 0.0;
	/*
	массив размером с количество процессов. каждый элемент хранит индекс первого элемента из столбца свободных членов, который принадлежит процессу
	массив с количествами пересылаемых элеметов из столбца свободных членов на каждый процесс
	*/
	int *_arrayCountOfProcesses, *range;

	//массив номеров ведущих строк на каждой итерации // i-ая итерация - j-ая строка ведущая
	int *NumsMainLines;
	/* массив номеров итераций, на которых соответствующие строки системы, расположенные на процессе, выбраны  ведущими */
	int *NumsMainLinesIter;
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	setvbuf(stdout, 0, _IONBF, 0);

	MPI_Init(&argc, &argv);

	Compute compute;

	MPI_Comm_rank(MPI_COMM_WORLD, &compute.rank_proc);
	MPI_Comm_size(MPI_COMM_WORLD, &compute.g_NumProc);

	// Принудительная синхронизация
	MPI_Barrier(MPI_COMM_WORLD);

	compute.Init();
	// Принудительная синхронизация
	MPI_Barrier(MPI_COMM_WORLD);

	compute.startTime = MPI_Wtime();

	compute.RunGauss();

	MPI_Barrier(MPI_COMM_WORLD);
	compute.time_end = MPI_Wtime();
	compute.DeltaTime = (compute.time_end - compute.startTime);

	if (compute.rank_proc == 0)
	{
		if (compute.status == 1) // решение найдено
			printf("\nRoots: \n");
		if (compute.status == 0) printf("\nNo roots.\n"); // решений нет
		if (compute.status == 2) // решений бесконечно много
			printf("\nContinum. Solution: \n\n");

		printf("\n\nTime= %f\n", compute.DeltaTime);
	}
	if (compute.rank_proc == 0)
	{
		printf("\ntime_ended!");
	}
	MPI_Finalize();
}