#include <iostream>
#include <time.h>
#include <mpi.h>
#include <math.h>
#include <vector>
#include <numeric>
#include <windows.h>


using namespace std;


#define dec_chunk 4


MPI_Comm MPI_Comm_dec, MPI_Comm_star;
MPI_Comm buf;


void thread()
{
    bool run = true;
    MPI_Status status;

    int rank;

    MPI_Comm_rank(MPI_Comm_star, &rank);

    int sum;
    int nneighbors;
    int* neighbors;

    MPI_Graph_neighbors_count(MPI_Comm_star, rank, &nneighbors);
    neighbors = new int[nneighbors] {};
    MPI_Graph_neighbors(MPI_Comm_star, rank, nneighbors, neighbors);

    bool isMain = nneighbors != 1;

    while (true) {

        bool run = isMain, recvRun;

        for (int i = 0; i < nneighbors; i++) {
            MPI_Sendrecv(&run, 1, MPI_C_BOOL, neighbors[i], 1, &recvRun, 1, MPI_C_BOOL, neighbors[i],
            1, MPI_Comm_star, &status);
            cout << "(" << rank << ")" << " <- " << (recvRun ? "run" : "stop") << endl;
        }
        break;
    }

end:
    // cout << "stopping" << endl;
    delete[] neighbors;
}

void starG() {
    int src, dst, rank, data;
    MPI_Status status;
    MPI_Comm_rank(buf, &rank);

    data = rank;
    cout << "(" << rank << ")current data: " << data << endl;
    MPI_Cart_shift(MPI_Comm_dec, 0, 1, &src, &dst);
    MPI_Sendrecv_replace(&data, 1, MPI_INT, dst, 0, src, 0, MPI_Comm_dec, &status);
    cout << "(" << rank << ")recv data: " << data << endl;
}

void printArr(int* arr, int size) {
    cout << "[ ";
    for (int i = 0; i < size-1; i++)
        cout << arr[i] << ", ";
    cout << arr[size - 1] << " ]" << endl;
}

bool checkIn(int* arr, int size, int value) {
    for (int i = 0; i < size; i++)
        if (arr[i] == value) return true;
    return false;
}

vector<int> exludeVector(vector<int> v, int max) {
    vector<int> result(max - v.size());

    int k = 0;
    for (int i = 0; i < max; i++) {
        if (!checkIn((int*)&v.front(), v.size(), i))
            result[k++] = i;
    }

    return result;
}


int main(int argc, char *argv[])
{
	int rank, processesNum;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &processesNum);

    if (processesNum < 4)
        return 0;

    MPI_Group GroupComm, GraphGroup, DecGroup;
    vector<int> dec(processesNum / 2);
    iota(dec.begin(), dec.end(), dec.size());
    vector<int> test = exludeVector(dec, processesNum);

    if (!checkIn((int *)&dec.front(), dec.size(), rank)){

        MPI_Comm_group(MPI_COMM_WORLD, &GroupComm);
        MPI_Group_incl(GroupComm, test.size(), test.data(), &GraphGroup);
        MPI_Comm_create(MPI_COMM_WORLD, GraphGroup, &buf);
        MPI_Comm_rank(buf, &rank);

        vector<int> edges((test.size() - 1) * 2);

        for (int i = 1; i < test.size(); i++)
            edges[i - 1] = i;

        vector<int> indexes(test.size());
        int k = test.size() - 1;
        for (int i = 0; i < test.size(); i++)
            indexes[i] = k++;
        int processesDecNum = (processesNum + 1) / 2;
        int processesStarNum = processesNum - processesDecNum + 1;

        if (rank == 0) {
            cout << "graph processes: ";
            printArr((int*)&test.front(), test.size());

            cout << "edges: ";
            printArr((int*)&edges.front(), edges.size());

            cout << "indexes: ";
            printArr((int*)&indexes.front(), indexes.size());
        }

        MPI_Graph_create(buf, processesDecNum, indexes.data(), edges.data(), 1, &MPI_Comm_star);

		thread();
    }
    else {
        // cout << "thread" << rank << " in star" << endl;

        MPI_Comm_group(MPI_COMM_WORLD, &GroupComm);
        MPI_Group_incl(GroupComm, dec.size(), dec.data(), &DecGroup);
        MPI_Comm_create(MPI_COMM_WORLD, DecGroup, &buf);

        int ndims = 1;
        int periods[] = { 1 };

        MPI_Cart_create(buf, ndims, dec.data(), periods, 1, &MPI_Comm_dec);

        // starG();
    }


	MPI_Finalize();
}