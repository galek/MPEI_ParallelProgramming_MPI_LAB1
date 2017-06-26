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

struct Compute {

	~Compute()
	{
		SAFE_DELETE_ARRAY(matrix);
		SAFE_DELETE_ARRAY(vector);
		SAFE_DELETE_ARRAY(Result);
	}

	double *matrix, *vector, *Result, *_matrix, *_vector, *_res, m = 0.0;
	//_matrix, _vector, _res - ����� �������, ������� b � ������� x �� ������������ ��������
	int  size, p, status, num_str; //num_str-����� ����� �������� �������, ���������� �� ���������� ��������
								   //status - ������ ������� ������
								   //stringcount - ���������� ����� �� ��������
	double  startTime, time_end, dt; // ���������� ��� ��������� �������
	int *NomeraStrokVed;    //������ ������� ������� ����� �� ������ �������� // i-�� �������� - j-�� ������ �������
	int *NomerStrokIterVed;   /* ������ ������� ��������, �� ������� ��������������� ������ �������, ������������� �� ��������, �������  �������� */

	int Numproc, rank_proc, *mass1,//-������ �������� � ���������� ���������. ������ �������
								   //������ ������ ������� �������� �� ������� ��������� ������, ������� ����������� ��������
		*range;  //������ � ������������ ������������ �������� �� ������� ��������� ������ �� ������ �������
				 //------------------------------------------------------------------------------

	void Vvod()
	{
		int show;
		size = 7500;

#if 0
		printf("\nprint matrix. true 1/false 0: ");
		//   scanf("%i", &show);
		show = 0;
#endif

		srand(p);

		/**/
		{
			matrix = new double[size*size];
			vector = new double[size];
			Result = new double[size];
		}

		for (int i = 0; i < size*size; i++)
		{
			matrix[i] = rand();
#if 0
			if (show == 1) printf("\nmatrix[%i]=%f", i, matrix[i]);
#endif
		};

		for (int i = 0; i < size; i++)
		{
			vector[i] = rand();
#if 0
			if (show == 1) printf("\nb[%i]=%f", i, vector[i]);
#endif

			Result[i] = 0;
		};
	}

	void Init()
	{
		int tmp_num_str; //����� �����, ��� �� ������������� �� ���������
		if (rank_proc == 0)
		{

			Vvod();
			printf("\ntime_started!");
		}

		MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

		//����������� ������� ����� ������, ������������� �� ���������� ��������

		tmp_num_str = size;

		for (int i = 0; i < rank_proc; i++)
			tmp_num_str = tmp_num_str - tmp_num_str / (Numproc - i);

		num_str = tmp_num_str / (Numproc - rank_proc);
		_matrix = new double[num_str*size];//�������� ������ ��� ������ �������
		_vector = new double[num_str]; //������ ��� �������� ������� ��������� ������
		_res = new double[num_str]; //������ ��� �������� ������� ����������
		NomeraStrokVed = new int[size]; //������ �������� ������� ����� ������� �� ������ ��������
		NomerStrokIterVed = new int[num_str]; /* ��������, �� ������� ��������������� ������ �������, ������������� �� ��������, �������  ������� */
		mass1 = new int[Numproc];
		range = new int[Numproc];

		for (int i = 0; i < num_str; i++)
			NomerStrokIterVed[i] = -1;
	}

	// ������������� �������� ������ ����� ����������

	void Raspred()
	{
		int *index_to_matr;             //������ ������� �������� �������, ������������� ��������
		int *range_to_proc;            //����� ��������� �������, ������������ ��������
		int numprocstr;
		int tmp_num_str;
		int prev_sz;
		int prev_indx;
		int portion;
		index_to_matr = new int[Numproc];
		range_to_proc = new int[Numproc];

		tmp_num_str = size;

		for (int i = 0; i < Numproc; i++)  //����������, ������� ��������� ������� ����� �������� ������� ��������
		{
			prev_sz = (i == 0) ? 0 : range_to_proc[i - 1];

			prev_indx = (i == 0) ? 0 : index_to_matr[i - 1];

			portion = (i == 0) ? 0 : numprocstr;             //����� �����, �������� ����������� ��������

			tmp_num_str -= portion;

			numprocstr = tmp_num_str / (Numproc - i);

			range_to_proc[i] = numprocstr*size;

			index_to_matr[i] = prev_indx + prev_sz;
		};

		MPI_Scatterv(matrix, range_to_proc, index_to_matr, MPI_DOUBLE, _matrix, range_to_proc[rank_proc], MPI_DOUBLE, 0, MPI_COMM_WORLD);

		//���������

		tmp_num_str = size;

		for (int i = 0; i < Numproc; i++)
		{
			int prev_sz = (i == 0) ? 0 : range[i - 1];
			int prev_indx = (i == 0) ? 0 : mass1[i - 1];
			int portion = (i == 0) ? 0 : range[i - 1];
			tmp_num_str -= portion;
			range[i] = tmp_num_str / (Numproc - i);
			mass1[i] = prev_indx + prev_sz;
		};

		//�������� �������

		/*
		-��� ���������
		-������ � ������������ ������������ �������� �� ������ �������
		-������ � �������� ��������, � ������� ���� �������� �������� �� vector

		-���� �������
		-������� ��������� �� ������ �������
		*/

		MPI_Scatterv(vector, range, mass1, MPI_DOUBLE, _vector, range[rank_proc], MPI_DOUBLE, 0, MPI_COMM_WORLD);
		status = 1;
		MPI_Bcast(&status, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&m, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		delete[] range_to_proc;
		delete[] index_to_matr;
	}

	void raw_elm(int iter_numb, double *glob_str)
	{
		double koef;
		for (int i = 0; i < num_str; i++)//��� ������ ������ � ��������
		{
			if (NomerStrokIterVed[i] == -1)
			{
				koef = _matrix[i*size + iter_numb] / glob_str[iter_numb];
				for (int j = iter_numb; j < size; j++)
				{
					_matrix[i*size + j] -= glob_str[j] * koef;
				};
				_vector[i] -= glob_str[size] * koef;

			};
		};
	}

	void gauss_el()
	{
		int VedIndex;   // ������ ������� ������ �� ���������� ��������
		struct { double max_val; int rank_proc; } loc_max, glob_max; //������������ �������+����� ��������, � �������� ��
		double* glb_maxst = new double[size + 1]; //�.�. ������ �������+�������� �������
		for (int i = 0; i < size; i++) //������� ���� �������
		{
			// ���������� ������� ������
			double max_val = 0;
			int tmp_index = -1;
			for (int j = 0; j < num_str; j++)
			{
				tmp_index = j;
				if ((NomerStrokIterVed[j] == -1) && (max_val < fabs(_matrix[i + size*j])))
				{
					max_val = fabs(_matrix[i + size*j]);
					VedIndex = j;
				};
			};

			loc_max.max_val = max_val;
			loc_max.rank_proc = rank_proc;

			//������ ������� ��������� ���� �������� ������������ ������� �� ���� �������, ��� ������� ��������� ��� ��������� ������������ �������
			MPI_Allreduce(&loc_max, &glob_max, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);

			//���������� ������� ������ ���� �������
			if (rank_proc == glob_max.rank_proc)
			{
				if (glob_max.max_val == 0)
				{
					status = 2;
					MPI_Barrier(MPI_COMM_WORLD);
					MPI_Send(&status, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
					NomerStrokIterVed[tmp_index] = i;
					NomeraStrokVed[i] = mass1[rank_proc] + VedIndex;
					continue;

				}
				else
				{
					NomerStrokIterVed[VedIndex] = i;  // ����� ��������, �� ������� ������ � ��������� ������� �������� ������� ��� ���� �������
					NomeraStrokVed[i] = mass1[rank_proc] + VedIndex;   //����������� ����� ������� ������ �������
				};
			};
			MPI_Bcast(&NomeraStrokVed[i], 1, MPI_INT, glob_max.rank_proc, MPI_COMM_WORLD);
			if (rank_proc == glob_max.rank_proc)
			{
				for (int j = 0; j < size; j++)
					glb_maxst[j] = _matrix[VedIndex*size + j];
				glb_maxst[size] = _vector[VedIndex];
			};
			//�������� ������� ������ ���� ���������
			MPI_Bcast(glb_maxst, size + 1, MPI_DOUBLE, glob_max.rank_proc, MPI_COMM_WORLD);
			//���������� ����������� � ������� � ������� i
			raw_elm(i, glb_maxst);
		};
	}


	void Frp(int stringIndex, // ����� ������, ������� ���� ������� �� ����������� ��������
		int &iterationrank_proc, // �������, �� ������� ��� ������
		int &IterationItervedindex) // ��������� ����� ���� ������ (� ������ ������ ��������)
	{
		for (int i = 0; i < Numproc - 1; i++) //���������� ���� ��������, ����������� ������ ������
		{
			if ((mass1[i] <= stringIndex) && (stringIndex < mass1[i + 1]))
				iterationrank_proc = i;
		}
		if (stringIndex >= mass1[Numproc - 1])
			iterationrank_proc = Numproc - 1;
		IterationItervedindex = stringIndex - mass1[iterationrank_proc];

	}

	void gauss_els_rev()
	{
		int Iterrank_proc;  // ���� ��������, ��������� ������� ������� ������
		int IndexVed;    // ��������� �� ����� �������� ����� ������� �������
		double IterRes;   // �������� Xi, ��������� �� ��������
		double val;
		// �������� ����
		for (int i = size - 1; i >= 0; i--)
		{
			Frp(NomeraStrokVed[i], Iterrank_proc, IndexVed);
			// ���������� ���� ��������, ����������� ������� ������� ������, � ����� ���� ������ �� ��������
			// ��������� �������� �����������
			if (rank_proc == Iterrank_proc)
			{
				if (_matrix[IndexVed*size + i] == 0)
				{
					if (_vector[IndexVed] == 0)
						IterRes = m;
					else
					{
						status = 0;
						MPI_Barrier(MPI_COMM_WORLD);
						MPI_Send(&status, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD);
						break;
					};
				}
				else
					IterRes = _vector[IndexVed] / _matrix[IndexVed*size + i];
				_res[IndexVed] = IterRes; //����� �������� ����������
			};
			MPI_Bcast(&IterRes, 1, MPI_DOUBLE, Iterrank_proc, MPI_COMM_WORLD);
			for (int j = 0; j < num_str; j++) //����������� ��������� ����������
				if (NomerStrokIterVed[j] < i)
				{
					val = _matrix[size*j + i] * IterRes;
					_vector[j] -= val;
				};
		};
	}

	//------------------------------------------------------------------------------

	void gauss_els()
	{
		gauss_el();
		gauss_els_rev();
		//���� ������, �������� �� ���� ������ (�������� ��������)
		MPI_Gatherv(_res, range[rank_proc], MPI_DOUBLE, Result, range, mass1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}

};





//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	setvbuf(stdout, 0, _IONBF, 0);

	MPI_Init(&argc, &argv);

	Compute compute;


	MPI_Comm_rank(MPI_COMM_WORLD, &compute.rank_proc);
	MPI_Comm_size(MPI_COMM_WORLD, &compute.Numproc);

	// �������������� �������������
	MPI_Barrier(MPI_COMM_WORLD);

	compute.Init();
	compute.Raspred();

	// �������������� �������������
	MPI_Barrier(MPI_COMM_WORLD);

	compute.startTime = MPI_Wtime();

	compute.gauss_els();

	MPI_Barrier(MPI_COMM_WORLD);
	compute.time_end = MPI_Wtime();
	compute.dt = (compute.time_end - compute.startTime);
	if (compute.rank_proc == 0)
	{
		if (compute.status == 1) // ������� �������
		{
			printf("\nRoots: \n");
			//                 for (int i = 0; i<size; i++)
			//                         printf(" \nres[%i] = %f ", i, res[NomeraStrokVed[i]]);
		};
		if (compute.status == 0) printf("\nNo roots.\n"); // ������� ���
		if (compute.status == 2) // ������� ���������� �����
		{
			printf("\nContinum. Solution: \n\n");
			//                 for (int i = 0; i<size; i++)
			//                         printf("\nres[%d] = %f ", i, res[NomeraStrokVed[i]]);
		};
		printf("\n\nTime= %f\n", compute.dt);
	};
	if (compute.rank_proc == 0)
	{
		printf("\ntime_ended!");
		//scanf("%i", &size);

		//_getch();
	};
	MPI_Finalize();
}