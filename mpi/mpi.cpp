#include <iostream>
#include <time.h>
#include <mpi.h>
#include <math.h>
#include <vector>
#include <numeric>
#include <limits.h>
#include <windows.h>


using namespace std;

#define MAX_PART_ARRAY 1000

#define MIN_PRINT_ELEMS 10
#define MIN_PRINT_SKIP_ELEMS 3

static MPI_Datatype MPI_CUSTOM;
static MPI_Group CommGroup, SortGroup, MergeGroup;
static MPI_Comm SortComm, MergeComm;

struct Msg {
	bool run;
	int size;
	int resultSize;
	int partsNum;
	int list[MAX_PART_ARRAY];
};

void registerStruct(Msg *type, MPI_Datatype *custom) {
	MPI_Aint displacements[5] = {
		offsetof(Msg, run),
		offsetof(Msg, size),
		offsetof(Msg, resultSize),
		offsetof(Msg, partsNum),
		offsetof(Msg, list),
	};

	int block_lengths[5] = { 1, 1, 1, 1, MAX_PART_ARRAY };
	MPI_Datatype types[5] = { MPI_C_BOOL, MPI_INT, MPI_INT, MPI_INT, MPI_INT };

	MPI_Type_create_struct(5, block_lengths, displacements, types, custom);
	MPI_Type_commit(custom);
}

void copyArr(int* arr1, int size1, int* arr2, int size2) {
	int resultSize = min(size1, size2);

	for (int i = 0; i < resultSize; i++)
		arr1[i] = arr2[i];
}

void printArr(int* arr, int size) {
	cout << "[ ";
	for (int i = 0; i < size - 1; i++) {
		cout << arr[i] << ", ";
		if (i == MIN_PRINT_ELEMS && i + MIN_PRINT_SKIP_ELEMS <= size - MIN_PRINT_ELEMS) {
			i = size - MIN_PRINT_ELEMS;
			cout << " ... , ";
		}
	}
	cout << arr[size - 1] << " ]" << endl;
}

struct resultArr {
	int* result;
	int size;
};

int* sortMerge(int* list, int size) {
	return list;
}

void thread() {
	int rank;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	bool run = true;
	int* list;
	MPI_Status status;

	while (run) {
		Msg msg;

		MPI_Recv(&msg, 1, MPI_CUSTOM, 0, MPI_ANY_TAG, SortComm, &status);
		if (!msg.run)
			break;

		int resultSize = msg.resultSize;
		int partsNum = msg.partsNum;
		list = new int[resultSize] {};

		int k = 0;
		for (int i = 0; i < partsNum; i++) {
			for (int j = 0; j < msg.size; j++) {
				if (k == resultSize) {
					cout << "[ERROR]" << endl;
					return;
				}
				list[k] = msg.list[j];
				k++;
			}
			/*cout << "(Sort)<-- ";
			printArr((int *)msg.list, msg.size);*/

			if (i != partsNum - 1)
				MPI_Recv(&msg, 1, MPI_CUSTOM, 0, MPI_ANY_TAG, SortComm, &status);
		}

		list = sortMerge(list, resultSize);
		cout << "sorted: ";
		printArr(list, resultSize);

		delete[] list;

		run = msg.run;
	}
	
	cout << "Process " << rank << " has been stopped" << endl;
}

resultArr mergeParts(int **parts, int partsNum, int *sizes) {
	int resultSize = 0;
	for (int i = 0; i < partsNum; i++)
		resultSize += sizes[i];

	int *result = new int[resultSize] {};
	int* indexes = new int[partsNum] {};
	bool* completed = new bool[partsNum] {};
	for (int i = 0; i < resultSize; i++) {
		int min = INT_MAX;
		int index = -1;

		for (int j = 0; j < partsNum; j++) {
			if (!completed[j] && parts[j][indexes[j]] < min) {
				min = parts[j][indexes[j]];
				index = j;
			}
		}
		result[i] = min;
		if (indexes[index] == sizes[index] - 1) {
			completed[index] = true;
			continue;
		}
		indexes[index]++;
	}

	delete[] completed;
	delete[] indexes;
	return resultArr{
		result,
		resultSize,
	};
}

void merge() {
	bool run = true;
	int** parts, rank;
	resultArr result;
	MPI_Status status;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	while (run) {
		Msg msg;
		int partsNum;

		MPI_Recv(&msg, 1, MPI_CUSTOM, 0, MPI_ANY_TAG, MergeComm, &status);
		if (!msg.run)
			break;

		partsNum = msg.partsNum;
		parts = new int* [partsNum] {};
		int* sizes = new int[partsNum] {};

		for (int i = 0;i < partsNum; i++) {
			sizes[i] = msg.size;
			parts[i] = new int[msg.size] {};
			copyArr(parts[i], msg.size, (int*) & msg.list, MAX_PART_ARRAY);

			if (i != partsNum - 1)
				MPI_Recv(&msg, 1, MPI_CUSTOM, 0, MPI_ANY_TAG, MergeComm, &status);
		}

		result = mergeParts(parts, partsNum, sizes);
		cout << "Merged: ";
		printArr(result.result, result.size);

		delete[] result.result;
		delete[] sizes;
		for (int i = 0; i < partsNum; i++)
			delete[] parts[i];
		delete[] parts;

		run = msg.run;
	}

	cout << "process" << rank << " has been stopped" << endl;
}

void killChildrens(int processesNum) {
	Msg msg;
	msg.run = false;
	for (int i = 1; i < processesNum - 1; i++) {
		cout << "stopping Sort" << i << endl;
		MPI_Send(&msg, 1, MPI_CUSTOM, i, 0, SortComm);
	}
	cout << "stopping Merge" << 1 << endl;
	MPI_Send(&msg, 1, MPI_CUSTOM, 1, 0, MergeComm);
}

struct processPart {
	vector<vector<int>> parts;
};

int random(int min, int max) {
	return min + rand() % (max - min);
}

int* generateList(int size, int min, int max) {
	int* list = new int[size] {};

	for (int i = 0; i < size; i++)
		list[i] = random(min, max);

	return list;
}

struct Splited {
	vector<vector<vector<int>>> splited;
	vector<int> resultSize;
	int allPartsNum;
};

Splited splitList(int* list, int size, int maxChunk, int processesNum) {
	int chunkNum = size / maxChunk + (size % maxChunk != 0);

	vector<vector<int>> parts(chunkNum);
	vector<int> sizes(chunkNum);

	int k = 0;
	for (int i = 0; i < chunkNum; i++) {
		if (size - k == 0) {
			chunkNum--;
			goto process;
		}
		sizes[i] = min(maxChunk, size - k);
		parts[i] = vector<int>(sizes[i]);
		for (int j = 0; j < sizes[i]; j++) {
			parts[i][j] = list[k];
			k++;
		}
	}
process:
	int processChunk = chunkNum / processesNum + 1;

	vector<vector<vector<int>>> processParts(processesNum);
	vector<int> resultSize(processesNum);
	int allPartsNum = 0;

	k = 0;
	for (int i = 0; i < processesNum; i++) {
		if (chunkNum - k == 0)
			goto exit;

		int chunk = min(processChunk, chunkNum - k);
		processParts[i] = vector<vector<int>>(chunk);
		resultSize[i] = 0;

		cout << "Process" << i << ": " << endl;
		for (int j = 0; j < chunk; j++) {
			cout << "\t" << j << ": ";
			processParts[i][j] = parts[k];
			resultSize[i] += parts[k].size();
			printArr(&parts[k].front(), sizes[k]);
			k++;
		}
	}
exit:

	return Splited{
		processParts,
		resultSize,
		(int)parts.size(),
	};
}

void printMsg(Msg msg) {
	cout << "run: \t\t" << (msg.run ? "true" : "false") << endl;
	cout << "partsNum: \t" << msg.partsNum << endl;
	cout << "partSize: \t" << msg.size << endl;
	cout << "resultSize: \t" << msg.resultSize << endl;
	cout << "part: ";
	printArr((int*)&msg.list, msg.size);
}


int main(int argc, char *argv[])
{
	int rank = 0, processesNum = 0;
	Msg msgType;
	srand(1);

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &processesNum);

	int lastProcess = processesNum - 1;
	vector<int> ranks(processesNum - 1);
	iota(ranks.begin(), ranks.end(), 0);
	vector<int> mergeRanks = {0, lastProcess};

	MPI_Comm_group(MPI_COMM_WORLD, &CommGroup);
	MPI_Group_incl(CommGroup, processesNum - 1, &ranks.front(), &SortGroup);
	MPI_Group_incl(CommGroup, 2, &mergeRanks.front(), &MergeGroup);

	MPI_Comm_create(MPI_COMM_WORLD, SortGroup, &SortComm);
	MPI_Comm_create(MPI_COMM_WORLD, MergeGroup, &MergeComm);

	registerStruct(&msgType, &MPI_CUSTOM);

#define numparts 2
	if (!rank) {
		cout << "sort group: ";
		printArr(&ranks.front(), processesNum - 1);
		cout << "merge group: ";
		printArr(&mergeRanks.front(), 2);

		Msg msg;
		int* list = generateList(10000, 0, 1);
		Splited splited = splitList(list, 10000, MAX_PART_ARRAY, processesNum - 2);
		vector<vector<vector<int>>> slist = splited.splited;
		vector<int> resultSize = splited.resultSize;

		delete[] list;

		int** parts = new int*[numparts] {};
		int sizes[numparts] = { 5, 4 };
		int partsNums[numparts] = { 1, 1 };
		parts[0] = new int[5]{ 3, 27, 38, 43, 55 };
		parts[1] = new int[4] { 9, 10, 82, 105 };
		
		cout << "Sorting..." << endl;
		for (int i = 0; i < slist.size(); i++) {
			int to = min(i % processesNum + 1, lastProcess - 1);
			cout << "-->Sending to " << to << endl;
			for (int j = 0; j < slist[i].size(); j++) {
				cout << j << "/" << slist[i].size() - 1 << ": ";
				printArr(&slist[i][j].front(), slist[i][j].size());
				copyArr((int*)&msg.list, MAX_PART_ARRAY, &slist[i][j].front(), slist[i][j].size());
				msg.partsNum = slist[i].size();
				msg.run = true;
				msg.size = slist[i][j].size();
				msg.resultSize = resultSize[i];

				MPI_Send(&msg, 1, MPI_CUSTOM, to, 0, SortComm);
			}
		}

		cout << "Merging..." << endl;
		for (int i = 0; i < slist.size(); i++) {
			for (int j = 0; j < slist[i].size(); j++) {
				copyArr((int*)&msg.list, MAX_PART_ARRAY, &slist[i][j].front(), slist[i][j].size());
				msg.partsNum = splited.allPartsNum;
				msg.run = true;
				msg.size = slist[i][j].size();

				MPI_Send(&msg, 1, MPI_CUSTOM, 1, 0, MergeComm);
			}
		}

		delete[] parts[0];
		delete[] parts[1];
		delete[] parts;

		killChildrens(processesNum);
	}
	else if (rank != lastProcess) {
		thread();
	}
	else {
		merge();
	}

	MPI_Finalize();
}