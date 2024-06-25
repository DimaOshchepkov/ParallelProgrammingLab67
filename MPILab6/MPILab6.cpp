#include <iostream>
#include <math.h>
#include <mpi.h>
#include <format>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>


int task1(int argc, char* argv[]) {
	int proc_rank, proc_num;
	long long N, sum = 0, tmpsum = 0;
	MPI_Status st;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_num);

	std::cout << std::format("I am {} process from {} processes", proc_rank, proc_num);
	
	MPI_Finalize();
	return 0;
}

int task2(int argc, char* argv[]) {
	int proc_rank, proc_num;
	long long N, sum = 0, tmpsum = 0;
	MPI_Status st;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_num);
	if (proc_rank == 0)
		std::cout << std::format("{}: processes\n", proc_num);
	if (proc_rank % 2 == 0)
		std::cout << std::format("{}: First\n", proc_rank);
	else
		std::cout << std::format("{}: Second\n", proc_rank);

	MPI_Finalize();
	return 0;
}


int task3(int argc, char* argv[])
{
	constexpr auto MSGLEN = 32768;
	constexpr auto TAG_A = 100;
	constexpr auto TAG_B = 200;

	std::vector<float> message1(MSGLEN), message2(MSGLEN);
	int rank, dest, source, send_tag, recv_tag;
	MPI_Status status;
	MPI_Request request_send, request_recv; // Запросы для неблокирующих операций

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	for (int i = 0; i < MSGLEN; i++) {
		message1[i] = 1 - 2 * rank;
	}

	if (rank == 0) {
		dest = 1;
		source = 1;
		send_tag = TAG_A;
		recv_tag = TAG_B;
	}
	else if (rank == 1) {
		dest = 0;
		source = 0;
		send_tag = TAG_B;
		recv_tag = TAG_A;
	}

	std::cout << "Task " << rank << " has prepared the message" << std::endl;

	// Инициируем неблокирующую отправку сообщения
	MPI_Isend(message1.data(), MSGLEN, MPI_FLOAT, dest, send_tag,
		MPI_COMM_WORLD, &request_send);

	// Инициируем неблокирующее получение сообщения
	MPI_Irecv(message2.data(), MSGLEN, MPI_FLOAT, source, recv_tag,
		MPI_COMM_WORLD, &request_recv);

	// Ожидаем завершения обеих неблокирующих операций
	MPI_Wait(&request_send, &status);
	MPI_Wait(&request_recv, &status);

	std::cout << "Task " << rank << " has sent and received the message" << std::endl;

	MPI_Finalize();
	return 0;
}

int task4(int argc, char* argv[])
{

	int rank, dest, source;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	std::cout << "Task " << rank << " has sent and received the message" << std::endl;

	MPI_Finalize();
	return 0;
}

int task5(int argc, char** argv) {
	int rank, size, number, received_number;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// Инициализация генератора случайных чисел
	srand(time(NULL) + rank);

	if (rank == 0) {
		// Нулевой процесс генерирует случайное число
		number = rand() % 100;  // Генерируем число от 0 до 99
		printf("Process 0 generated number: %d\n", number);

		// Отправляем число первому процессу
		MPI_Send(&number, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

		// Получаем число от последнего процесса
		MPI_Recv(&received_number, 1, MPI_INT, size - 1, 0, MPI_COMM_WORLD, &status);

		// Проверка результата
		if (received_number == number + size - 1) {
			printf("Correct!\n");
		}
		else {
			printf("Error!\n");
		}

	}
	else {
		// Остальные процессы
		// Получаем число от предыдущего процесса
		MPI_Recv(&number, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
		printf("Process %d received number: %d\n", rank, number);

		// Увеличиваем число на 1
		number++;

		// Передаем число следующему процессу
		MPI_Send(&number, 1, MPI_INT, (rank + 1) % size, 0, MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}


using namespace std;

// Функция для генерации вектора случайных чисел
vector<int> generate_vector(int size) {
	random_device rd;
	mt19937 generator(rd());
	uniform_int_distribution<> distribution(0, 99);

	vector<int> vec(size);
	for (int& element : vec) {
		element = distribution(generator);
	}
	return vec;
}


int task6(int argc, char** argv) {
	const int N = 10; // Количество массивов
	const int M = 100; // Размер каждого массива

	int rank, size;
	vector<vector<int>> arrays(N); // Матрица векторов
	int current_array = 0; // Индекс текущего обрабатываемого вектора
	int total_sum = 0; // Глобальная сумма
	int local_sum = 0; // Сумма, вычисленная рабочим процессом
	MPI_Status status;
	int* buf = new int[M];

	const int LAST_ARRAY = 1;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);


	if (rank == 0) {
		while (current_array < N)
		{
			for (int i = current_array; i < min(current_array + size, N); i++)
				arrays[i] = generate_vector(M);

			// Распределение векторов по рабочим процессам
			for (int i = 1; i < size; ++i) {
				if (current_array == N - 1) {
					MPI_Send(arrays[current_array].data(), M, MPI_INT, i, LAST_ARRAY, MPI_COMM_WORLD);
				}
				else {
					MPI_Send(arrays[current_array].data(), M, MPI_INT, i, 0, MPI_COMM_WORLD);
				}
				current_array++;
			}

			for (int proc = 1; proc < size; proc++) {
				MPI_Recv(&local_sum, 1, MPI_INT, proc, MPI_ANY_TAG,
					MPI_COMM_WORLD, &status);
				total_sum += local_sum;
			}
		}

		cout << "Total sum: " << total_sum << endl;

	}
	else {
		while (true)
		{
			MPI_Recv(buf, M, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			// Вычисление суммы элементов вектора
			local_sum = std::accumulate(buf, std::next(buf, M), 0);

			// Отправка результата обратно нулевому процессу
			MPI_Send(&local_sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

			if (status.MPI_TAG == LAST_ARRAY) break;
		}
	}
	
	MPI_Finalize();
	delete[] buf;
	return 0;
}

void printMatrix(const std::vector<std::vector<int>>& matrix) {
	int rows = matrix.size();
	if (rows == 0) {
		std::cout << "Matrix is empty" << std::endl;
		return;
	}

	int cols = matrix[0].size();

	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			std::cout << matrix[i][j] << "\t";
		}
		std::cout << std::endl;
	}
}

int task6_i(int argc, char** argv) {
	const int N = 2;  // Количество массивов
	const int M = 2; // Размер каждого массива

	int rank, size;
	vector<vector<int>> arrays(N); // Матрица векторов
	int total_sum = 0;             // Глобальная сумма
	int local_sum = 0;             // Сумма, вычисленная рабочим процессом
	MPI_Status status;
	int* buf = new int[M];

	const int LAST_ARRAY = 1;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (rank == 0) {
		// Генерация всех векторов
		for (int i = 0; i < N; ++i) {
			arrays[i] = generate_vector(M);
		}
		//printMatrix(arrays);

		int current_array = 0;
		int active_requests = 0;
		vector<MPI_Request> send_requests(size - 1);

		// Отправляем начальные массивы всем рабочим процессам
		for (int proc = 1; proc < size && current_array < N; ++proc) {
			int tag = (current_array == N - 1) ? LAST_ARRAY : 0;
			MPI_Isend(arrays[current_array].data(), M, MPI_INT, proc, tag, MPI_COMM_WORLD, &send_requests[proc - 1]);
			current_array++;
			active_requests++;
		}

		while (current_array < N || active_requests > 0) {
			// Проверяем, завершилась ли отправка на какой-либо процесс
			int index;
			int flag;
			MPI_Testany(size - 1, send_requests.data(), &index, &flag, &status);

			if (flag && index != MPI_UNDEFINED) {
				active_requests--;

				// Получаем результат от освободившегося процесса
				MPI_Irecv(&local_sum, 1, MPI_INT, index+1, MPI_ANY_TAG, MPI_COMM_WORLD, &send_requests[index]);
				MPI_Wait(&send_requests[index], MPI_STATUS_IGNORE);
				total_sum += local_sum;

				if (current_array < N) {
					// Отправляем следующий массив этому процессу
					int tag = (current_array == N - 1) ? LAST_ARRAY : 0;
					MPI_Isend(arrays[current_array].data(), M, MPI_INT, index + 1, tag, MPI_COMM_WORLD, &send_requests[index]);
					current_array++;
					active_requests++;
				}
			}
		}

		cout << "Total sum: " << total_sum << endl;

	}
	else {
		while (true) {
			MPI_Request recv_request;
			MPI_Irecv(buf, M, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_request);
			MPI_Wait(&recv_request, &status);

			// Вычисление суммы элементов вектора
			local_sum = std::accumulate(buf, buf + M, 0);

			// Отправка результата обратно нулевому процессу
			MPI_Request send_request;
			MPI_Isend(&local_sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &send_request);
			MPI_Wait(&send_request, MPI_STATUS_IGNORE);

			if (status.MPI_TAG == LAST_ARRAY) break;
		}
	}

	delete[] buf;
	MPI_Finalize();
	return 0;
}

int task7(int argc, char* argv[])
{
#define MASTER  0
#define TAG     0
#define MSGSIZE 1000
#define MAX 25

	int my_rank, source, num_nodes, workers;
	int buffer[MSGSIZE];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
	workers = num_nodes - 1;

	// Build matrix to fit the number of workers, but won't work if workers are greater tand 1000
	int** matrix;
	matrix = new int* [workers];
	for (int i = 0; i < workers; i++) {
		matrix[i] = new int[workers];
	}
	for (int i = 0, x = 0; i < workers; i++)
	{
		for (int j = 0; j < workers; j++, x++)
		{
			matrix[i][j] = x;
		}
	}

	if (my_rank != MASTER) {
		int send[1];

		for (int i = workers - my_rank, x = 0; x < workers; i++, x++) {
			// Can't wander off the array
			if (i >= workers)
				i = 0;

			// Can't talk to yourself
			if ((i + 1) == my_rank)
				continue;

			send[0] = matrix[my_rank - 1][i];

			// Send first
			if (my_rank < (i + 1)) {
				MPI_Send(send, 1, MPI_INT, (i + 1), TAG, MPI_COMM_WORLD);
				MPI_Recv(buffer, 1, MPI_INT, (i + 1), TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				matrix[my_rank - 1][i] = buffer[0];
			}

			// Receive first
			else {
				MPI_Recv(buffer, 1, MPI_INT, (i + 1), TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				matrix[my_rank - 1][i] = buffer[0];
				MPI_Send(send, 1, MPI_INT, (i + 1), TAG, MPI_COMM_WORLD);
			}
		}

		MPI_Send(matrix[my_rank - 1], MSGSIZE, MPI_INT, MASTER, TAG, MPI_COMM_WORLD);
	}
	else {
		//sleep(20);
		printf("Num_nodes: %d\n", num_nodes);
		printf("Hello from Master (process %d)!\n", my_rank);
		cout << "Initial array:" << endl;
		for (int i = 0, x = 0; i < workers; i++)
		{
			for (int j = 0; j < workers; j++, x++)
			{
				if (matrix[i][j] < 10)
					cout << " ";
				cout << matrix[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl;

		// Get results
		for (source = 1; source < num_nodes; source++) {
			MPI_Recv(buffer, MSGSIZE, MPI_INT, source, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			for (int i = 0; i < workers; i++)
				matrix[source - 1][i] = buffer[i];
		}

		cout << "Transposed array:" << endl;
		for (int i = 0, x = 0; i < workers; i++)
		{
			for (int j = 0; j < workers; j++, x++)
			{
				if (matrix[i][j] < 10)
					cout << " ";
				cout << matrix[i][j] << " ";
			}
			cout << endl;
		}
	}

	MPI_Finalize();

	return 0;
}


int main(int argc, char* argv[])
{
	task7(argc, argv);
}