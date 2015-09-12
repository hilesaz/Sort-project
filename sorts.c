#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const unsigned int FIXED_SEED = 47;
const unsigned int DATA_SIZE = 1<<24;
const unsigned int CACHE_LINE_SIZE = 64;

void printArray(unsigned int* start, unsigned int* end)
{
	unsigned int i;
	for(i = 0; i < end-start; ++i)
	{
		printf("%d ", start[i]);
		if(!((i+1)%8) && i != end-start-1)
			printf("\n");
	}
	printf("\n");
}
int checkSorted(unsigned int* start, unsigned int* end)
{
	while(start < end-1)
	{
		if(start[0] > start[1])
			return 0;
		++start;
	}
	return 1;
}

//Separate copies of the copy function are provided to ease performance analysis
void copyDownCopy(unsigned int* to, unsigned int* from, unsigned int* fromEnd)
{
	while(from < fromEnd)
		*to++ = *from++;
}
void copyUpCopy(unsigned int* to, unsigned int* from, unsigned int* fromEnd)
{
	while(from < fromEnd)
		*to++ = *from++;
}
void swap(unsigned int* a, unsigned int* b)
{
	unsigned int tmp = *a;
	*a = *b;
	*b = tmp;
}

//Separate copies of the merge function are provided to ease performance analysis
void mergeDownCopy(unsigned int* out, unsigned int* start1, unsigned int* end1, unsigned int* end2)
{
	unsigned int* start2 = end1;
	while(start1 < end1 && start2 < end2)
	{
		if(*start1 < *start2)
		{
			*out++ = *start1++;
		}
		else
		{
			*out++ = *start2++;
		}
	}
	copyDownCopy(out, start2, end2);
	copyDownCopy(out, start1, end1);
}
void mergeUpCopy(unsigned int* out, unsigned int* start1, unsigned int* end1, unsigned int* end2)
{
	unsigned int* start2 = end1;
	while(start1 < end1 && start2 < end2)
	{
		if(*start1 < *start2)
		{
			*out++ = *start1++;
		}
		else
		{
			*out++ = *start2++;
		}
	}
	copyUpCopy(out, start2, end2);
	copyUpCopy(out, start1, end1);
}
void downMergeSort(unsigned int* scratch, unsigned int* start, unsigned int* end)
{
	unsigned int* mid;
	if(end-start == 1)
	{
		return;
	}
	mid = start + (end - start)/2;
	downMergeSort(scratch, start, mid);
	downMergeSort(scratch, mid, end);  
	mergeDownCopy(scratch, start, mid, end);
	copyDownCopy(start, scratch, scratch + (end-start));
}


unsigned int* pmin(unsigned int* p1, unsigned int* p2)
{
	return p1<p2?p1:p2;
}
unsigned int umin(unsigned int u1, unsigned int u2)
{
	return u1<u2?u1:u2;
}

//requires start to be 64byte aligned for speed
void upMergeSort(unsigned int* scratch, unsigned int* start, unsigned int* end)
{
	unsigned int size = 1;
	unsigned int* itr;
	
	while(size < end-start)
	{
		for(itr = start; itr < end; itr += size * 2)
		{
			if(end-itr > size)
			{
				mergeUpCopy(scratch, itr, itr + size, pmin(end, itr + size*2)); 
				copyUpCopy(itr, scratch, scratch + umin(end - itr, size*2));
			}
		}
		size *= 2;
	}
}

typedef void (*Sorter)(unsigned int*, unsigned int*, unsigned int*);

//A negative return value for failure
clock_t timeSort(Sorter sorter, unsigned int size)
{
	unsigned int* data;
	unsigned int* scratch; 
	if(posix_memalign( (void**)&data, CACHE_LINE_SIZE, sizeof(*data) * size) != 0)
		return -1.0;
	if(posix_memalign( (void**)&scratch, CACHE_LINE_SIZE, sizeof(*scratch) * size) != 0)
	{
		free(data);
		return -1.0;
	}
		
	unsigned int* itr;
	clock_t t;
	
	srand(FIXED_SEED);

	itr = data;

	while(itr < data + size) *itr++ = rand()%(size * 4);

	t = clock();
	sorter(scratch, data, data + size);
	t = clock() - t;

	if(!checkSorted(data, data + size))
		t = -1.0;
	free(data);
	free(scratch);
	return t;
}
void timePrintSort(const char* name, Sorter sorter, int verbose)
{
	unsigned int* data = malloc(sizeof(*data) * DATA_SIZE);
	unsigned int* scratch = malloc(sizeof(*scratch) * DATA_SIZE);
	unsigned int* itr;
	clock_t t;
	
	srand(FIXED_SEED);

	itr = data;

	while(itr < data + DATA_SIZE) *itr++ = rand()%(DATA_SIZE * 4);
	if(verbose)
		printArray(data, data + DATA_SIZE);

	t = clock();
	sorter(scratch, data, data + DATA_SIZE);
	t = clock() - t;

	if(verbose)
		printArray(data, data + DATA_SIZE);

	printf("Sorted using %s\n", name);
	if(checkSorted(data, data + DATA_SIZE))
	{
		printf("Array is sorted properly ");
	}
	else
	{
		printf("Array is not sorted properly ");
	}
	printf("in %f seconds\n", ((float)t)/CLOCKS_PER_SEC);
	free(data);
	free(scratch);
}

void benchSort(const char* names[2], Sorter sorters[2], unsigned int maxSize)
{
	unsigned int size;
	clock_t t1, t2;
	printf(",%s,%s\n", names[0], names[1]);
	for(size = 2;size < maxSize; size<<=1)
	{
		t1 = timeSort(sorters[0], size);
		t2 = timeSort(sorters[1], size);
		if(t1 < 0 || t2 < 0)
			return;
		printf("%d,%f,%f\n", size, ((float)t1)*1000000/CLOCKS_PER_SEC, ((float)t2)*1000000/CLOCKS_PER_SEC);
	}
}

int main(int argc, char** argv)
{
	int verbose = 0;
	const int maxSize = 1<<24;
	
	const char* names[2];
	Sorter sorters[2];

	names[0] = "UpMerge";
	sorters[0] = upMergeSort;
	names[1] = "DownMerge";
	sorters[1] = downMergeSort;

	if(argc > 1 && !strcmp(argv[1], "verbose"))
		verbose = 1;

	benchSort(names, sorters, maxSize);
}
