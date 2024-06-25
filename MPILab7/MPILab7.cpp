// MPILab7.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <mpi.h>
#include <iostream>
#include <math.h>
#include <format>
#include <vector>
#include <random>
#include <algorithm>

using namespace std;

void print_array(const std::vector<int>& arr) {
    std::cout << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        std::cout << arr[i];
        if (i < arr.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

int task1(int argc, char** argv, int N) {
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);


    // Вычисление диапазона индексов для каждого процесса
    int base_chunk_size = N / world_size;
    int remainder = N % world_size;
    int start_index = world_rank * base_chunk_size + std::min(world_rank, remainder) + 1;
    int end_index = start_index + base_chunk_size + (world_rank < remainder ? 1 : 0);

    double local_sum = 0;
    for (int k = start_index; k < end_index; k++) {
        local_sum += std::pow(-1, k - 1) / (float)k;
    }

    double total_sum = 0;
    MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        std::cout << std::format("ln2 = {}", total_sum);
    }
    
    MPI_Finalize();

    return 0;
}

int task1_par(int argc, char** argv, int N) {
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Вычисление диапазона индексов для каждого процесса
    int base_chunk_size = N / world_size;
    int remainder = N % world_size;
    int start_index = world_rank * base_chunk_size + std::min(world_rank, remainder) + 1;
    int end_index = start_index + base_chunk_size + (world_rank < remainder ? 1 : 0);

    double local_sum = 0;
    for (int k = start_index; k < end_index; k++) {
        local_sum += std::pow(-1, k - 1) / (float)k;
    }

    // Отправка локальных сумм на нулевой процесс
    if (world_rank != 0) {
        MPI_Send(&local_sum, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }
    
    if (world_rank == 0) {
        double total_sum = local_sum;
        double received_sum;

        // Прием локальных сумм от других процессов
        
        for (int i = 1; i < world_size; ++i) {
            MPI_Recv(&received_sum, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            total_sum += received_sum;
        }

        std::cout << std::format("ln2 = {}\n", total_sum);
    }

    MPI_Finalize();
    return 0;
}

int is_sorted(int* arr, int start, int end) {
    // Проверяем, упорядочен ли массив на заданном диапазоне
    for (int i = start + 1; i < end; ++i) {
        if (arr[i] < arr[i - 1]) {
            return 0; // Не упорядочен
        }
    }
    return 1; // Упорядочен
}

std::vector<int> generate_random_array(int size, int min_value, int max_value) {
    std::random_device rd;  // Источник энтропии для генератора
    std::mt19937 gen(rd()); // Вихрь Мерсенна (генератор случайных чисел)
    std::uniform_int_distribution<> distrib(min_value, max_value); // Равномерное распределение

    std::vector<int> array(size);
    for (int& element : array) {
        element = distrib(gen);
    }

    return array;
}

int task2(int argc, char** argv, int N) {
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    std::vector<int> full_array;

    // Процесс 0 создает и заполняет массив
    if (world_rank == 0) {
        full_array = generate_random_array(N, 0, 100);
        print_array(full_array);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Вычисление размера части массива для каждого процесса
    int base_chunk_size = N / world_size;
    int remainder = N % world_size;
    int local_size = base_chunk_size + (world_rank < remainder ? 1 : 0);

    // Выделение памяти под локальную часть массива
    int* local_array = new int[local_size + 1];


    // Распределение массива по процессам
    int* sendcounts = new int[world_size];
    int* displs = new int[world_size];

    for (int i = 0; i < world_size; ++i) {
        sendcounts[i] = base_chunk_size + (i < remainder ? 1 : 0) +
            (i > 0 ? 1: 0);
        displs[i] = (i > 0) ? (displs[i - 1] + sendcounts[i - 1] - 1) : 0;
    }

    MPI_Scatterv(full_array.data(), sendcounts, displs, MPI_INT,
        local_array, local_size + 1, MPI_INT,
        0, MPI_COMM_WORLD);

    delete[] sendcounts;
    delete[] displs;


    // Проверка упорядоченности на локальном участке
    int local_sorted = is_sorted(local_array, 0, local_size + (world_rank != 0 ? 1 : 0));

    // Редукция: проверяем, упорядочены ли ВСЕ части массива
    int global_sorted = 0;
    MPI_Reduce(&local_sorted, &global_sorted, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        if (global_sorted) {
            std::cout << "Ordered\n";
        }
        else {
            std::cout << "Not ordered\n";
        }
    }

    MPI_Finalize();
    return 0;
}

void print_vector(const double* vector, int size) {
    cout << "[";
    for (int i = 0; i < size; ++i) {
        cout << vector[i];
        if (i < size - 1) {
            cout << ", ";
        }
    }
    cout << "]" << endl;
}

void print_matrix(const double* matrix_data, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        cout << "[";
        for (int j = 0; j < cols; ++j) {
            cout << matrix_data[i * cols + j]; // Обратите внимание на индексацию
            if (j < cols - 1) {
                cout << ", ";
            }
        }
        cout << "]" << endl;
    }
}

int task3(int argc, char** argv, int N) {
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Инициализация матрицы и вектора на процессе 0
    double* matrix_data = nullptr;
    double** matrix_rows = nullptr;
    double* vector = new double[N];
    double* result = nullptr;

    if (world_rank == 0) {
        matrix_data = new double[N * N];
        matrix_rows = new double* [N];
        result = new double[N];

        // Заполнение матрицы, вектора и инициализация указателей на строки
        for (int i = 0; i < N; ++i) {
            matrix_rows[i] = &matrix_data[i * N];
            for (int j = 0; j < N; ++j) {
                matrix_rows[i][j] = i + j;
            }
            vector[i] = i;
        }
        print_vector(vector, N);
        std::cout << '\n';
        print_matrix(matrix_data, N, N);
        std::cout << '\n';
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Вычисление размера части массива для каждого процесса
    int base_chunk_size = N / world_size;
    int remainder = N % world_size;
    int local_size = base_chunk_size + (world_rank < remainder ? 1 : 0);
    local_size *= N;


    int* sendcounts = new int[world_size];
    int* displs = new int[world_size];

    for (int i = 0; i < world_size; ++i) {
        sendcounts[i] = (base_chunk_size + (i < remainder ? 1 : 0)) * N;
        displs[i] = (i > 0) ? (displs[i - 1] + sendcounts[i - 1]) : 0;
    }

    double* local_matrix = new double[local_size];
    MPI_Scatterv(matrix_data, sendcounts, displs, MPI_DOUBLE,
        local_matrix, local_size, MPI_DOUBLE,
        0, MPI_COMM_WORLD);

    
    MPI_Bcast(vector, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    
    // Локальное вычисление
    double* local_result = new double[local_size/N];
    for (int i = 0; i < local_size / N; ++i) {
        local_result[i] = 0;
        for (int j = 0; j < N; ++j) {
            local_result[i] += local_matrix[i*N + j] * vector[j];
        }
    }

    for (int i = 0; i < world_size; ++i) {
        sendcounts[i] = (base_chunk_size + (i < remainder ? 1 : 0));
        displs[i] = (i > 0) ? (displs[i - 1] + sendcounts[i - 1]) : 0;
    }
    MPI_Gatherv(local_result, local_size / N, MPI_DOUBLE,
        result, sendcounts, displs, MPI_DOUBLE,
        0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        print_vector(result, N);
        delete[] matrix_data;
        delete[] matrix_rows;
        delete[] result;
    }

    delete[] vector;
    delete[] local_result;

    MPI_Finalize();
    return 0;
}

double find_min(const vector<double>& arr) {
    double min_value = numeric_limits<double>::max();
    for (double val : arr) {
        if (val < min_value) {
            min_value = val;
        }
    }
    return min_value;
}

double calculate_dot_product(const vector<double>& vec1, const vector<double>& vec2) {
    double dot_product = 0;
    if (vec1.size() != vec2.size()) {
        return dot_product; // Или обработайте ошибку другим способом
    }
    for (size_t i = 0; i < vec1.size(); ++i) {
        dot_product += vec1[i] * vec2[i];
    }
    return dot_product;
}

vector<double> generate_random_vector_double(size_t size, double lower_bound, double upper_bound) {
    // Используем стандартный генератор случайных чисел Mersenne Twister
    std::random_device rd;
    std::mt19937 generator(rd());

    // Создаем распределение для типа double в заданном диапазоне
    std::uniform_real_distribution<double> distribution(lower_bound, upper_bound);

    // Создаем вектор нужного размера
    std::vector<double> random_vector(size);

    // Заполняем вектор случайными числами
    for (double& value : random_vector) {
        value = (int)distribution(generator);
    }

    return random_vector;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        os << vec[i];
        if (i < vec.size() - 1) {
            os << ", ";
        }
    }
    os << "]\n";
    return os;
}

int task4(int argc, char** argv, int N) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Group world_group, even_group, odd_group;
    vector<int> even_ranks(size / 2 + (size % 2)); // Массив для четных рангов
    vector<int> odd_ranks(size / 2);              // Массив для нечетных рангов
    int even_count = 0, odd_count = 0;
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            even_ranks[even_count++] = i;
        }
        else {
            odd_ranks[odd_count++] = i;
        }
    }
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_incl(world_group, even_count, even_ranks.data(), &even_group);
    MPI_Group_incl(world_group, odd_count, odd_ranks.data(), &odd_group);

    // Создаем коммуникаторы для каждой группы
    MPI_Comm even_comm, odd_comm;
    MPI_Comm_create(MPI_COMM_WORLD, even_group, &even_comm);
    MPI_Comm_create(MPI_COMM_WORLD, odd_group, &odd_comm);

    int even_rank = -1, odd_rank = -1; // Инициализируем значения по умолчанию

    if (rank % 2 == 0) { // Четный процесс
        MPI_Comm_rank(even_comm, &even_rank);
    }
    else { // Нечетный процесс
        MPI_Comm_rank(odd_comm, &odd_rank);
    }

    int base_chunk_size_odd = N / odd_count;
    int base_chunk_size_even = N / even_count;

    int remainder_odd = N % odd_count;
    int remainder_even = N % even_count;

    int local_size_odd = base_chunk_size_odd + (rank < remainder_odd ? 1 : 0);
    int local_size_even = base_chunk_size_even + (rank < remainder_even ? 1 : 0);

    double global_min = 0, global_dot_product = 0;


    vector<int> sendcounts, sendcounts_odd;
    vector<int> displs, displs_odd;
    vector<double> array, array_buf;
    vector<double> vector_a, vector_b, vector_a_buf, vector_b_buf;
    if (even_rank == 0) {
        array = generate_random_vector_double(N, 0, 100); 
       // std::cout << array;
    }

    if (odd_rank == 0) {
        vector_a = generate_random_vector_double(N, 0, 10);
        vector_b = generate_random_vector_double(N, 0, 10);
        //std::cout << vector_a << vector_b;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    int start_time = MPI_Wtime();
    if (rank % 2 == 0) {
        array_buf.resize(local_size_even);

        sendcounts.resize(even_count);
        displs.resize(even_count);
        int offset = 0;
        for (int i = 0; i < even_count; ++i) {
            sendcounts[i] = base_chunk_size_even + (i < remainder_even ? 1 : 0);
            displs[i] = offset;
            offset += sendcounts[i];
        }
        // Рассылка данных с помощью MPI_Scatterv
        MPI_Scatterv(array.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, // Данные для отправки
            array_buf.data(), local_size_even, MPI_DOUBLE,    // Буфер для получения
            0, even_comm);

        double local_min = find_min(array_buf);
        MPI_Reduce(&local_min, &global_min, 1, MPI_DOUBLE, MPI_MIN, 0, even_comm);
    }
    else {
        sendcounts_odd.resize(odd_count);
        displs_odd.resize(odd_count);
        int offset = 0;
        for (int i = 0; i < odd_count; ++i) {
            sendcounts_odd[i] = base_chunk_size_odd + (i < remainder_odd ? 1 : 0);
            displs_odd[i] = offset;
            offset += sendcounts_odd[i];
        }

        vector_a_buf.resize(local_size_odd);
        vector_b_buf.resize(local_size_odd);

        // Рассылка vector_a
        MPI_Scatterv(vector_a.data(), sendcounts_odd.data(), displs_odd.data(), MPI_DOUBLE,
            vector_a_buf.data(), local_size_odd, MPI_DOUBLE,
            0, odd_comm);

        // Рассылка vector_b
        MPI_Scatterv(vector_b.data(), sendcounts_odd.data(), displs_odd.data(), MPI_DOUBLE,
            vector_b_buf.data(), local_size_odd, MPI_DOUBLE,
            0, odd_comm);

        double local_dot_product = calculate_dot_product(vector_a_buf, vector_b_buf);
        MPI_Reduce(&local_dot_product, &global_dot_product, 1, MPI_DOUBLE, MPI_SUM, 0, odd_comm);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    int end_time = MPI_Wtime();
    if (even_rank == 0) {
        cout << std::format("Min: {}\n", global_min);
        cout << std::format("Time: {}\n", end_time - start_time);
    }
    if (odd_rank == 0) {
        cout << std::format("Scalar: {}\n", global_dot_product);
    }
    
    if (rank % 2 == 0) { // Четный процесс
        MPI_Comm_free(&even_comm);
    }
    else { // Нечетный процесс
        MPI_Comm_free(&odd_comm);
    }
    MPI_Group_free(&world_group);
    MPI_Group_free(&even_group);
    MPI_Group_free(&odd_group);
    MPI_Finalize();
    return 0;
}

int main(int argc, char** argv)
{
    //task1(argc, argv, 3);
    //task1_par(argc, argv, 3);
    //task2(argc, argv, 3);
    //task3(argc, argv, 3);
    //task4(argc, argv, 2);
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
