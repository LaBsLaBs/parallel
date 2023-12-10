#include <iostream>
#include <time.h>
#include <mpi.h>
#include <math.h>
#include <vector>
#include <numeric>


using namespace std;

#define MAX_PART_ARRAY 1000

static MPI_Datatype MPI_CUSTOM;
struct Msg {
	bool run;
	bool isEnd;
	int list[MAX_PART_ARRAY];
};

void registerStruct(Msg *type, MPI_Datatype *custom) {
	MPI_Aint displacements[3] = {
		offsetof(type, run),
		offsetof(type, isEnd),
		offsetof(type, list),
	};

	int block_lengths[3] = { 1, 1, MAX_PART_ARRAY };
	MPI_Datatype types[3] = { MPI_C_BOOL, MPI_C_BOOL, MPI_INT };

	MPI_Type_create_struct(3, block_lengths, displacements, types, custom);
	MPI_Type_commit(custom);
}

void thread() {

}


int main(int argc, char *argv[])
{
	int rank, processesNum;
	MPI_Group CommGroup, SortGroup, MergeGroup;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &processesNum);

	vector<int> ranks(processesNum - 1);
	iota(ranks.begin(), ranks.end(), 1);

	MPI_Comm_group(MPI_COMM_WORLD, &CommGroup);
	MPI_Group_incl(CommGroup, processesNum - 1, &ranks.front(), &SortGroup);
	MPI_Group_excl(CommGroup, processesNum - 1, &ranks.front(), &MergeGroup);

    if (!rank) {

	}
	else {
		thread();
	}

	MPI_Finalize();
}