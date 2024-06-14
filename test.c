#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <assert.h>

#define PREPARE_ITERS 256
#define PREPARE_LEN 65536
#define N_TESTS 128
#define TESTS_PER_LEN 100
#define MIN_LEN 15
#define MAX_LEN (1 << 24)
#define MIN_SHIFTS 1
#define MAX_SHIFTS_M 2
#define BUF_SIZE 512
#define SORT_AMT 5

#define ABORT()                    \
    printf("\nClosing tester.\n"); \
    fflush(stdout);                \
    assert(0);

long long utime() {
	struct timeval now_time;
	gettimeofday(&now_time, NULL);
	return now_time.tv_sec * 1000000LL + now_time.tv_usec;
}

typedef struct {
    int    value;
    size_t idx;
} Value;

int valCmp(Value* a, Value* b) {
    return a->value - b->value;
}

#define STABILITY_CHECK 0

#if STABILITY_CHECK
    #define VAR Value
    #define CMP(a, b) valCmp(a, b)
#else
    #define VAR int
#endif

void set(VAR* array, size_t at, int value) {
    #if STABILITY_CHECK
        (array + at)->value = value;
    #else
        array[at] = value;
    #endif
}

int get(VAR* array, size_t at) {
    #if STABILITY_CHECK
        return (array + at)->value;
    #else
        return array[at];
    #endif
}

void printArray(VAR* array, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf("%i,", get(array, i)); 
    printf("\n");
}

#include "heliumSort.h"

void heliumSortTest(VAR* array, size_t len, size_t mem) {
    heliumSort(array, 0, len, mem);
}

// other sorts
int cmp_int0(const void * a, const void * b) {   
	const int fa = *(const int *) a;
	const int fb = *(const int *) b;

	return fa - fb;
}

#include <stdbool.h>
#define SORT_TYPE VAR
#define SORT_CMP CMP

#include "other_sorts/GrailSort.h"
#include "other_sorts/SqrtSort.h"
#include "other_sorts/WikiSort.h"
#include "other_sorts/logsort.h"

size_t getMem(size_t n, size_t b) {
    size_t sqrtn = 1;
    for (; sqrtn * sqrtn < n; sqrtn <<= 1);
    size_t keySize = n / sqrtn,
           twoK    = keySize << 1;

    switch (b) {
        case 1: return n >> 1;
        case 2: return sqrtn + twoK;
        case 3: return sqrtn + keySize;
        case 4: return sqrtn;
    }

    return b;
}

void grailsortTest(VAR *a, size_t n, size_t b) {
    b = getMem(n, b);

	VAR *s = malloc(b * sizeof(VAR));
	grail_commonSort(a, n, s, b);
	free(s);
}

void sqrtsortTest(VAR *a, size_t n, size_t b) {
	SqrtSort(a, n);
}

bool wikicmp(VAR a, VAR b) {
    #if STABILITY_CHECK
        return a.value < b.value;
    #else
        return a < b;
    #endif
}

void wikisortTest(VAR *a, size_t n, size_t b) {
    b = getMem(n, b);
	WikiSort(a, n, wikicmp, b);
}

void logsortTest(VAR *a, size_t n, size_t b) {
    b = getMem(n, b);
    if (b < 24) b = 24;
    
    logSort(a, n, b);
}

// shuffling and array generation
int qsortCmp(const void* a, const void* b) {
    #if STABILITY_CHECK
        Value* x = (Value*)a;
        Value* y = (Value*)b;

        return x->value - y->value;
    #else
        int* x = (int*)a;
        int* y = (int*)b;

        return *x - *y;
    #endif
}

void randomArray(VAR* array, size_t len) {
    for (size_t i = len; i--;) set(array, i, rand() % len);
}

void noShuf(VAR* array, size_t len) {}

void sort(VAR* array, size_t len) {
    qsort(array, len, sizeof(VAR), qsortCmp);
}

size_t scrambledPart(size_t len) {
    // return rand() % (len >> 2); 
    return len >> 3; // this should be random but it's fixed for consistent benchmarks
}

void scrambledHead(VAR* array, size_t len) {
    size_t offs = scrambledPart(len);
    sort(array + offs, len - offs);
}

void scrambledTail(VAR* array, size_t len) {
    sort(array, len - scrambledPart(len));
}

void reversed(VAR* array, size_t len) {
    sort(array, len);

    size_t a = 0;
    for (--len; a < len; a++, len--) swap(array, a, len);
}

void finalMergePass(VAR* array, size_t len) {
    size_t h = len >> 2;
    sort(array, h);
    sort(array + h, len - h);
}

void genArray(VAR* array, size_t len, size_t shifts, void (*dist)(VAR*, size_t), void (*shuf)(VAR*, size_t)) {
    dist(array, len);
    shuf(array, len);

    for (size_t i = 0; i < len; i++)
        set(array, i, get(array, i) >> shifts);

    #if STABILITY_CHECK
        while (len--) (array + len)->idx = len;
    #endif
}

void check(VAR* array, VAR* reference, size_t len) {
    for (size_t i = 1; i < len; i++) {
        #if STABILITY_CHECK
            if ((array + i - 1)->value > (array + i)->value) {
                printf("\nIndices %lu and %lu are out of order.\n", i - 1, i);
                ABORT();
            }

            if ((array + i)->value != (reference + i)->value) {
                printf("\nIndex %lu does not match reference.\n", i);
                ABORT();
            }

            if ((array + i - 1)->value == (array + i)->value && (array + i - 1)->idx > (array + i)->idx) {
                printf("\nIndices %lu and %lu are unstable.\n", i - 1, i);
                ABORT();
            }
        #else
            if (array[i - 1] > array[i]) {
                printf("\nIndices %lu and %lu are out of order.\n", i - 1, i);
                ABORT();
            }

            if (array[i] != reference[i]) {
                printf("\nIndex %lu does not match reference.\n", i);
                ABORT();
            }
        #endif
    }
}

void prepare(VAR* array, void (*sorts[])(VAR*, size_t, size_t)) {
    for (size_t i = PREPARE_ITERS; i--;) {
        for (size_t j = 0; j < SORT_AMT; j++) {
            genArray(array, PREPARE_LEN, 1, randomArray, noShuf);
            sorts[j](array, PREPARE_LEN, BUF_SIZE);
        }
    }
}

long long sortTest(VAR* array, VAR* testArray, VAR* reference, size_t len, void (*sorts[])(VAR*, size_t, size_t), char* names[], size_t i, size_t mem) {
    long long start, end;

    double avg = 0;
    void (*sort)(VAR*, size_t, size_t) = sorts[i];

    printf("%s (%lu): ", names[i], getMem(len, mem));
    fflush(stdout);

    for (size_t j = TESTS_PER_LEN; j--;) {
        memcpy(testArray, array, len * sizeof(VAR));

        start = utime();
        sort(testArray, len, mem);
        end = utime();
        check(testArray, reference, len);

        avg += end - start;
    }
        
    avg /= TESTS_PER_LEN;
    printf("%lli\n", (long long)avg);
}

void test(VAR* array, VAR* testArray, VAR* reference, size_t len, void (*sorts[])(VAR*, size_t, size_t), char* names[], char* autoMem) {
    srand(utime());
    
    memcpy(reference, array, len * sizeof(VAR));
    sort(reference, len);
    
    for (size_t i = 0; i < SORT_AMT; i++) {
        if (autoMem[i]) {
            sortTest(array, testArray, reference, len, sorts, names, i, 0);
        } else {
            for (size_t mem = 0; mem <= 4; mem++)
                sortTest(array, testArray, reference, len, sorts, names, i, mem);
            sortTest(array, testArray, reference, len, sorts, names, i, BUF_SIZE);
        }
    }
}

void runTests(VAR* array, VAR* testArray, VAR* reference, void (*sorts[])(VAR*, size_t, size_t), char* names[], char* autoMem, size_t n, size_t s) {
    printf("%lu items, %lu shifts\n", n, s);

    printf("Presorted inputs.\n");
    genArray(array, n, s, randomArray, sort);
    test(array, testArray, reference, n, sorts, names, autoMem);

    printf("Reverse sorted inputs.\n");
    genArray(array, n, s, randomArray, reversed);
    test(array, testArray, reference, n, sorts, names, autoMem);

    printf("Final merge pass.\n");
    genArray(array, n, s, randomArray, finalMergePass);
    test(array, testArray, reference, n, sorts, names, autoMem);

    printf("Scrambled head.\n");
    genArray(array, n, s, randomArray, scrambledHead);
    test(array, testArray, reference, n, sorts, names, autoMem);

    printf("Scrambled tail.\n");
    genArray(array, n, s, randomArray, scrambledTail);
    test(array, testArray, reference, n, sorts, names, autoMem);

    printf("Random inputs.\n");
    genArray(array, n, s, randomArray, noShuf);
    test(array, testArray, reference, n, sorts, names, autoMem);
}

int main() {
    void (*sorts[])(VAR*, size_t, size_t) = {
        heliumSortTest,
        grailsortTest,
        sqrtsortTest,
        wikisortTest,
        logsortTest
    };

    char *names[] = {
        "Helium Sort",
        "Grail Sort",
        "Sqrt Sort",
        "Wiki Sort",
        "Log Sort"
    };

    char autoMem[] = {
        0,
        0, 
        1,
        0, 
        0,
    };

    VAR* array = (VAR*)malloc((3 * MAX_LEN) * sizeof(VAR));
    VAR* testArray = array + MAX_LEN;
    VAR* reference = testArray + MAX_LEN;

    printf("Preparing...\n");
    prepare(array, sorts);

    #if STABILITY_CHECK
        srand(utime());
        // runTests(array, testArray, reference, sorts, names, 818654, 17);
        // ABORT();

        for (size_t i = N_TESTS; i--;) {
            size_t len  = MIN_LEN + (rand() % MAX_LEN),
                   maxS = 63 - __builtin_clzll(len) - MAX_SHIFTS_M,
                   shfs = MIN_SHIFTS + (rand() % maxS);

            runTests(array, testArray, reference, sorts, names, autoMem, len, shfs);
        }
    #else
        printf("Few unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 14, 12);

        printf("sqrt(n) unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 14, 7);

        printf("Most unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 14, 0);


        printf("Few unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 21, 19);

        printf("sqrt(n) unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 21, 11);

        printf("Most unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 21, 0);


        printf("Few unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 24, 22);

        printf("sqrt(n) unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 24, 14);

        printf("Most unique | ");
        runTests(array, testArray, reference, sorts, names, autoMem, 1 << 24, 0);
    #endif

    free(array);
    return 0;
}