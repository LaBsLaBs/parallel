#include <iostream>
#include <time.h>
#include <mpi.h>
#include <math.h>


using namespace std;


#define RANDOM_MAX 100
#define RANDOM_MIN 1

#define MAX_CHUNK 10
#define MAX_POLYNOM 1000

static MPI_Datatype mpi_custom_dt;

struct indata_type {
    int chunk;
    unsigned int sizes[MAX_CHUNK];
    int inputSize;
    int input[MAX_POLYNOM];
    bool run;
};

class Poly
{
private:
    int* poly;
    unsigned int size;

    int getRandom(int min, int max);

public:
    Poly();
    Poly(unsigned int size);
    Poly(int* poly, unsigned int size);
    ~Poly();
    int* getPoly();
    unsigned int getSize();
    void fillRandom();
    void fillRandom(int min, int max);
    void printPoly();

    static Poly copy(Poly poly) {
        unsigned int size = poly.getSize();
        int* newPoly = new int[size] {};

        for (int i = 0; i < size; i++)
            newPoly[i] = poly.getPoly()[i];

        return Poly(newPoly, size);
    }
};

Poly::Poly()
{
    this->size = 0;
}

Poly::Poly(unsigned int size)
{
    this->size = size + 1;
    this->poly = new int[this->size];
}

Poly::Poly(int* poly, unsigned int size)
{
    this->size = size;
    this->poly = new int[this->size];

    for (int i = 0; i < size; i++)
        this->poly[i] = poly[i];
}

Poly::~Poly()
{
    // delete[] this->poly;
}

int* Poly::getPoly()
{
    return this->poly;
}

unsigned int Poly::getSize()
{
    return this->size;
}

int Poly::getRandom(int min, int max)
{
    return min + rand() % (max - min);
}

void Poly::fillRandom()
{
    for (int i = 0; i < this->size; i++)
        this->poly[i] = getRandom(1, 100);
}

void Poly::fillRandom(int min, int max)
{
    for (int i = 0; i < this->size; i++)
        this->poly[i] = getRandom(min, max);
}

void Poly::printPoly()
{
    if (this->size == 0)
        return;

    cout << this->poly[0] << " ";
    for (int i = 1; i < this->size; i++)
        if (this->poly[i] != 0)
            cout << "+ " << this->poly[i] << "x**" << i << " ";
}

class PackPolynoms;

class Polynoms
{
private:
    Poly* polynoms;
    unsigned int size;
    unsigned int reserveSize;

    unsigned int calcNextSize(unsigned int size);

public:
    bool debug;

    Polynoms();
    Polynoms(unsigned int size);
    Polynoms(unsigned int size, bool withoutReserve);
    ~Polynoms();
    unsigned int getSize();
    Poly* getPolynoms();
    void addPoly(Poly);
    Poly dot2(Poly poly1, Poly poly2);
    Poly dotN(Polynoms polynoms);
    Poly dotAll(unsigned int size);
    Polynoms arr2Polynoms(int* polynoms, unsigned int* sizes, unsigned int size);
    unsigned int getAllSize();
    PackPolynoms getArrPolynoms(unsigned int size);
    unsigned int* getGroupedSizes(unsigned int size);
    unsigned int calcGroupSize(unsigned int size);
    unsigned int calcAllGroupSize(unsigned int size);
    int* group2Polynoms(Poly poly1, Poly poly2);
};

class PackPolynoms
{
public:
    Polynoms polynoms;
    unsigned int** sizes;
    unsigned int size;
};

unsigned int Polynoms::calcNextSize(unsigned int size)
{
    return size + 10;
}

Polynoms::Polynoms()
{
    unsigned int calcSize = calcNextSize(0);

    this->reserveSize = calcSize;
    this->polynoms = new Poly[calcSize];
    this->size = calcSize;
}

Polynoms::Polynoms(unsigned int size)
{
    unsigned int calcSize = calcNextSize(size);

    this->reserveSize = calcSize;
    this->polynoms = new Poly[calcSize];
    this->size = calcSize;
}

Polynoms::Polynoms(unsigned int size, bool withoutReserve)
{
    if (withoutReserve) {
        this->reserveSize = 0;
        this->polynoms = new Poly[size];
        this->size = size;
    }
    else {
        unsigned int calcSize = calcNextSize(size);

        this->reserveSize = calcSize - this->size;
        this->polynoms = new Poly[calcSize];
        this->size = calcSize;
    }
}

Polynoms::~Polynoms()
{
    // delete[] this->polynoms;
}

unsigned int Polynoms::getSize()
{
    return this->size - this->reserveSize;
}

Poly* Polynoms::getPolynoms()
{
    return this->polynoms;
}

void Polynoms::addPoly(Poly poly)
{
    if (this->reserveSize == 0) {
        unsigned int calcSize = calcNextSize(this->size);

        unsigned int reserveSize = calcSize - this->size;
        Poly* polynoms = new Poly[calcSize];
        unsigned int size = calcSize;

        for (int i = 0; i < this->size; i++)
            polynoms[i] = Poly(this->polynoms[i].getPoly(), this->polynoms[i].getSize());

        delete[] this->polynoms;

        this->reserveSize = reserveSize;
        this->polynoms = polynoms;
        this->size = size;
    }
    
    this->polynoms[this->size - this->reserveSize] = Poly(poly.getPoly(), poly.getSize());
    this->reserveSize--;
}

Poly Polynoms::dot2(Poly poly1, Poly poly2)
{
    unsigned int size1 = poly1.getSize();
    unsigned int size2 = poly2.getSize();
    int* resultPoly;

    resultPoly = new int[size1 + size2] {};

    for (int i = 0; i < size1; i++) {
        for (int j = 0; j < size2; j++) {
            resultPoly[i + j] += poly1.getPoly()[i] * poly2.getPoly()[j];
        }
    }

    Poly result = Poly(resultPoly, size1 + size2);

    return result;
}

Poly Polynoms::dotN(Polynoms polynoms)
{
    if (polynoms.getSize() == 0)
        return Poly();

    Poly* polys = polynoms.getPolynoms();
    Poly result = polys[0];

    for (int i = 1; i < polynoms.getSize(); i++)
        result = Poly::copy(polynoms.dot2(result, polys[i]));

    return result;
}

Polynoms Polynoms::arr2Polynoms(int* polynoms, unsigned int* sizes, unsigned int size)
{
    Polynoms polys = Polynoms(size);

    for (int i = 0; i < size; i++) {
        int* buf = new int[sizes[i]] {};

        for (int j = 0; j < sizes[i]; j++) {
            buf[j] = polynoms[j];
        }

        polys.addPoly(Poly(buf, sizes[i]));
    }

    return polys;
}

int* Polynoms::group2Polynoms(Poly poly1, Poly poly2)
{
    unsigned int resultSize = poly1.getSize() + poly2.getSize();

    int* result = new int[resultSize] {};
    int* poly1Arr = poly1.getPoly();
    int* poly2Arr = poly2.getPoly();

    for (int i = 0; i < poly1.getSize(); i++)
        result[i] = poly1Arr[i];

    for (int i = poly1.getSize(); i < resultSize; i++)
        result[i] = poly2Arr[i - poly1.getSize()];

    return result;
}

PackPolynoms Polynoms::getArrPolynoms(unsigned int size)
{
    unsigned int grSizes = calcGroupSize(size);
    unsigned int** sizes = new unsigned int* [grSizes] {};

    Polynoms result = Polynoms(0);
    Poly buf = Poly();
    int polyIndex = 0;

    for (unsigned int i = 0; i < grSizes; i++) {
        buf = Poly();
        sizes[i] = new unsigned int[size] {};
        if (this->debug) 
            cout << "{ (";

        for (unsigned int j = 0; j < size; j++) {
            if (this->debug) {
                this->polynoms[polyIndex].printPoly();
                cout << ")";
                if (j != size - 1)
                    cout << ", (";
            }

            sizes[i][j] = this->polynoms[polyIndex].getSize();

            buf = Poly(group2Polynoms(buf, this->polynoms[polyIndex]),
                buf.getSize() + this->polynoms[polyIndex].getSize());
            polyIndex++;
            if (polyIndex >= getSize())
                break;
        }
        if (this->debug)
            cout << " }" << endl;

        result.addPoly(Poly::copy(buf));
    }

    PackPolynoms pack;

    pack.sizes = sizes;
    pack.polynoms = result;
    pack.size = grSizes;

    return pack;
}

unsigned int Polynoms::calcAllGroupSize(unsigned int size)
{
    return getAllSize() / size + 1;
}

unsigned int Polynoms::calcGroupSize(unsigned int size)
{
    return getSize() / size;
}

unsigned int* Polynoms::getGroupedSizes(unsigned int size)
{
    unsigned int resultSize = calcAllGroupSize(size);
    unsigned int* sizes = new unsigned int[resultSize];

    for (int i = 0; i < resultSize; i++) {
        for (int j = i * size; (j < (i + 1) * size && j < getSize()); j++) {
            sizes[i] += this->polynoms[j].getSize();
        }
    }

    return sizes;
}

unsigned int Polynoms::getAllSize()
{
    unsigned int size = 0;

    for (int i = 0; i < getSize(); i++) {
        size += this->polynoms[i].getSize();
    }

    return size;
}

int calcOperations(int elements, int chunk)
{
    int result = 0, buf = elements;

    while (buf > 1) {
        buf /= 2;
        result++;
    }

    return result;
}

void killChildrens(int processesNum)
{
    indata_type data;
    data.run = false;
    for (int i = 1; i < processesNum; i++) {
        MPI_Send(&data, 1, mpi_custom_dt, i, 0, MPI_COMM_WORLD);
    }
}

void printResult(Polynoms polynoms, Poly result)
{
    for (int i = 0; i < polynoms.getSize(); i++) {
        cout << "(";
        polynoms.getPolynoms()[i].printPoly();
        if (i != polynoms.getSize() - 1)
            cout << ") * ";
        else
            cout << ") = ";
    }
    result.printPoly();
    cout << endl;
}

Polynoms decodePolyninoms(int *array, unsigned int size, int chunk, unsigned int *sizes)
{
    int sum = 0;
    Polynoms polynoms = Polynoms();

    for (int i = 0; i < chunk; i++) {
        if (!sizes[i])
            continue;

        int* bufPoly = new int[sizes[i]] {};

        for (int j = 0; j < sizes[i]; j++)
            bufPoly[j] = array[sum + j];
        sum += sizes[i];

        polynoms.addPoly(Poly::copy(Poly(bufPoly, sizes[i])));

        delete[] bufPoly;
    }

    return polynoms;
}

void buildDerivedType(indata_type* indata, MPI_Datatype* message_type_ptr)
{
    MPI_Aint displacements[5] = { 
        offsetof(indata_type, chunk),
        offsetof(indata_type, sizes),
        offsetof(indata_type, inputSize),
        offsetof(indata_type, input),
        offsetof(indata_type, run),
    };

    int block_lengths[5] = { 1, MAX_CHUNK, 1, MAX_POLYNOM, 1 };
    MPI_Datatype types[5] = { MPI_INT, MPI_UNSIGNED, MPI_INT, MPI_INT, MPI_C_BOOL };

    MPI_Type_create_struct(5, block_lengths, displacements, types, message_type_ptr);
    MPI_Type_commit(message_type_ptr);
}

void copyArr(int* arr1, int sarr1, int* arr2, int sarr2) {
    int resultSize = min(sarr1, sarr2);

    for (int i = 0; i < resultSize; i++)
        arr1[i] = arr2[i];
}

void thread()
{
    bool run = true;
    MPI_Status status;

    Polynoms polynoms = Polynoms();
    Poly result;

    int* input = {};
    unsigned int* sizes = {};
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int sum;

    while (true) {
        sum = 0;
        indata_type data;
        indata_type resultData;
        result = Poly();

        MPI_Recv(&data, 1, mpi_custom_dt, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (!data.run)
            break;

        sizes = new unsigned int[data.chunk] {};
        input = new int[data.inputSize] {};

        copyArr((int*)sizes, data.chunk, (int*)&data.sizes, MAX_CHUNK);
        copyArr((int*)input, data.inputSize, (int*)&data.input, MAX_POLYNOM);

        polynoms = decodePolyninoms(input, data.inputSize, data.chunk, sizes);
        result = polynoms.dotN(polynoms);

        cout << "child result" << rank << ": ";
        result.printPoly();
        cout << endl;

        int resultSize = (int)result.getSize();

        copyArr((int*)&resultData.input, MAX_POLYNOM, result.getPoly(), resultSize);

        resultData.inputSize = min(resultSize, MAX_POLYNOM);
        MPI_Send(&resultData, 1, mpi_custom_dt, 0, 0, MPI_COMM_WORLD);

        delete[] input;
        delete[] sizes;
    }
}

Polynoms fillPolynoms()
{
    Polynoms polynoms = Polynoms();

    for (int i = 0; i < 2; i++) {
        Poly poly = Poly(1);
        poly.fillRandom(1, 2);
        polynoms.addPoly(Poly::copy(poly));
    }
    Poly poly = Poly(1);
    poly.fillRandom(2, 5);
    polynoms.addPoly(Poly::copy(poly));
    Poly poly1 = Poly(4);
    poly1.fillRandom(2, 5);
    polynoms.addPoly(Poly::copy(poly1));


    return polynoms;
}

int main(int argc, char *argv[])
{
	int rank, processesNum;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &processesNum);

    indata_type type;

    buildDerivedType(&type, &mpi_custom_dt);

	if (rank == 0) {
        Polynoms polynoms = fillPolynoms();
        Polynoms buf = Polynoms();
        Poly RESULT;
        PackPolynoms groupedPolynoms;

        MPI_Status status;

        bool run = true;
        int chunk = 2;
        if (chunk > polynoms.getSize())
            chunk = polynoms.getSize();

        int numOperations = calcOperations(polynoms.getSize(), chunk);
        cout << "processes num: " << processesNum << endl;
        cout << "num operations: " << numOperations << endl;

        groupedPolynoms = polynoms.getArrPolynoms(chunk);


        for (int oper = 0; oper < numOperations; oper++) {
            int j = 0;
            Polynoms buf = Polynoms();

            while (j < groupedPolynoms.size) {
                for (int i = 1; i < processesNum; i++) {
                    indata_type data;
                    indata_type recvData;
                    Poly bufResult;
                    int sizeOfArr = groupedPolynoms.polynoms.getPolynoms()[j].getSize();

                    unsigned int sizesStatic[MAX_CHUNK] = {};
                    int inputStatic[MAX_POLYNOM] = {};

                    copyArr((int *) &data.sizes, MAX_CHUNK, (int *)groupedPolynoms.sizes[j], chunk);
                    copyArr((int*) &data.input, MAX_POLYNOM, groupedPolynoms.polynoms.getPolynoms()[j].getPoly(), sizeOfArr);

                    data.chunk = chunk;
                    data.inputSize = sizeOfArr;
                    data.run = run;

                    MPI_Send(&data, 1, mpi_custom_dt, i, 0, MPI_COMM_WORLD);
                    MPI_Recv(&recvData, 1, mpi_custom_dt, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                    int* result = new int[recvData.inputSize] {};
                    bufResult = Poly::copy(Poly((int*)&recvData.input, recvData.inputSize));

                    buf.addPoly(bufResult);
                    if (recvData.inputSize)
                        RESULT = bufResult;

                    cout << "recv from " << i << ": ";
                    bufResult.printPoly();
                    cout << endl;

                    j++;
                    if (j == groupedPolynoms.size) {
                        break;
                    }

                    //delete[] result;
                }
            }

            groupedPolynoms = buf.getArrPolynoms(chunk);
        }

        cout << "stopping..." << endl;
        killChildrens(processesNum);
        printResult(polynoms, RESULT);
	}
	else {
		thread();
	}

	MPI_Finalize();
}