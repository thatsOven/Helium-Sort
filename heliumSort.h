/*
Copyright (c) 2020 thatsOven

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * Helium Sort
 * 
 * A block merge sorting algorithm inspired by Grail Sort and focused on adaptivity.
 * 
 * Time complexity:
 *  - Best case: O(n)
 *  - Average case: O(n log n)
 *  - Worst case: O(n log n)
 * Space complexity is variable.
 * 
 * The algorithm extends the concept of adaptivity to memory,
 * by using different strategies based on the amount
 * of memory given to it. 
 * 
 * Major strategies are:
 * "Uranium": merge sort, requires n / 2 memory.
 *            The code refers to it as "Strategy 1".
 * 
 * "Hydrogen": block merge sort, requires "x" memory with sqrt(n) + 2n / sqrt(n) <= x < n / 2.
 *             Hydrogen mode can be modified to run with sqrt(n) + n / sqrt(n) memory using 
 *             an internal key buffer, but benchmarking has shown that this variant, in practice, 
 *             is slower than strategy 3A. This mode is referred to as "Strategy 2".
 * 
 * "Helium": block merge sort, requires "x" memory with 0 <= x < sqrt(n) + 2n / sqrt(n).
 *           Helium mode uses five strategies, referred to as: "3A", "3B", "3C", "4A", 
 *           and "4B". Optimal amounts of memory are:
 *              - sqrt(n) + n / sqrt(n): will use strategy 3A;
 *              - sqrt(n): will use strategy 3B or 4A;
 *              - 0: will use strategy 3C or 4B.
 * 
 * When a very low amount of distinct values is found or the array size is less or equal than 256, 
 * the sort uses an adaptive in-place merge sort referred to as "Strategy 5".
 * 
 * Special thanks to the members of The Holy Grail Sort Project, for the creation of Rewritten GrailSort,
 * which has been a great reference during the development of this algorithm,
 * and thanks to aphitorite, a great sorting mind which inspired the creation of this algorithm,
 * alongside being very helpful for me to understand some of the workings of block merge sorting algorithms,
 * and for part of the code used in this algorithm itself: "smarter block selection", 
 * the algorithm used in the "blockSelectInPlace" and "blockSelectOOP" routines, and the 
 * code used in the "mergeBlocks" routine.
 */

#include <stdlib.h>

#ifndef VAR
    #define VAR int
#endif
#ifndef CMP
    #define CMP(a, b) ((*(a)) - (*(b)))
#endif

#define RUN_SIZE          32
#define SMALL_SORT        256
#define MIN_SORTED_UNIQUE 8
#define MAX_STRAT5_UNIQUE 8
#define MIN_REV_RUN_SIZE  8
#define SMALL_MERGE       4
#define SQRT_TWOR         8 // 2^ceil(log2(sqrt(2 * RUN_SIZE)))

typedef struct HeliumData HeliumData;

typedef struct HeliumData {
    char hasExtBuf;
    char hasIntBuf;

    VAR*   extBuf;
    size_t extBufLen;

    size_t* indices;
    size_t* keys;
    size_t  extKeyLen;
    
    size_t blockLen;
    size_t bufPos;
    size_t bufLen;
    size_t keyPos;
    size_t keyLen;

    void (*rotate)(HeliumData*, VAR*, size_t, size_t, size_t);
} HeliumData;

void swap(VAR* array, size_t a, size_t b) {
    VAR* x = array + a;
    VAR* y = array + b;

    VAR t = *x;
    *x    = *y;
    *y    = t;
}

void indexSwap(size_t* array, size_t a, size_t b) {
    size_t* x = array + a;
    size_t* y = array + b;

    size_t t = *x;
    *x       = *y;
    *y       = t;
}

void blockSwapFW(VAR* array, size_t a, size_t b, size_t len) {
    while (len--) swap(array, a++, b++);
}

void blockSwapBW(VAR* array, size_t a, size_t b, size_t len) {
    while (len--) swap(array, a + len, b + len);
}

void insertToLeft(VAR* array, size_t from, size_t to) {
    VAR tmp = array[from];
    while (from-- > to) array[from + 1] = array[from];
    array[to] = tmp;
}

void insertToRight(VAR* array, size_t from, size_t to) {
    VAR tmp = array[from];
    for (; from < to; from++) array[from] = array[from + 1];
    array[to] = tmp;
}

size_t binarySearchRight(VAR* array, size_t a, size_t b, VAR* value);

#define LGT(a, b) CMP((a), (b)) >= 0
#define LLT(a, b) CMP((a), (b)) <= 0
#define RGT(a, b) CMP((a), (b)) > 0
#define LEFT_RIGHT_FN(NAME) NAME##Left
#define LEFT_RIGHT_FN_INV(NAME) NAME##Right

#include "leftRight.h"

#undef LGT
#undef LLT
#undef RGT
#undef LEFT_RIGHT_FN
#undef LEFT_RIGHT_FN_INV

#define LGT(a, b) CMP((a), (b)) > 0
#define LLT(a, b) CMP((a), (b)) < 0
#define RGT(a, b) CMP((a), (b)) >= 0
#define LEFT_RIGHT_FN(NAME) NAME##Right
#define LEFT_RIGHT_FN_INV(NAME) NAME##Left

#include "leftRight.h"

#undef LGT
#undef LLT
#undef RGT
#undef LEFT_RIGHT_FN
#undef LEFT_RIGHT_FN_INV

#define CHECK_BOUNDS(self, array, a, m, b)          \
    if (CMP(array + m - 1, array + m) <= 0) return; \
    if (CMP(array + a, array + b - 1) >  0) {       \
        self->rotate(self, array, a, m, b);         \
        return;                                     \
    }

#define REDUCE_BOUNDS(array, a, m, b)                      \
    a = binarySearchRight(array, a, m - 1, array + m    ); \
    b = binarySearchLeft( array, m, b    , array + m - 1); 

void insertSort(VAR* array, size_t a, size_t b) {
    for (size_t i = a + 1; i < b; i++)
        if (CMP(array + i, array + i - 1) < 0)
            insertToLeft(array, i, binarySearchRight(array, a, i, array + i));
}

void sortRuns(VAR* array, size_t a, size_t b, size_t p) {
    size_t bn = a + ((p - a) / RUN_SIZE + 1) * RUN_SIZE;
    if (p != b && bn < b) b = bn;

    size_t i;
    for (i = a; i + RUN_SIZE < b; i += RUN_SIZE)
        insertSort(array, i, i + RUN_SIZE);

    if (i < b) insertSort(array, i, b);
}

inline void mergeInPlaceWithCheck(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    CHECK_BOUNDS(self, array, a, m, b);
    REDUCE_BOUNDS(array, a, m, b);

    mergeInPlaceLeft(self, array, a, m, b);
}

inline void mergeWithBuffer(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    CHECK_BOUNDS(self, array, a, m, b);
    REDUCE_BOUNDS(array, a, m, b);

    size_t ll = m - a,
           rl = b - m;

    if (ll > rl) {
        if (rl <= SMALL_MERGE) mergeInPlaceBWLeft(self, array, a, m, b);
        else                   mergeWithBufferBWLeft(array, a, m, b, self->bufPos);
    } else {
        if (ll <= SMALL_MERGE) mergeInPlaceFWLeft(self, array, a, m, b);
        else                   mergeWithBufferFWLeft(array, a, m, b, self->bufPos);
    }
}

inline void mergeOOP(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    CHECK_BOUNDS(self, array, a, m, b);
    REDUCE_BOUNDS(array, a, m, b);

    size_t ll = m - a,
           rl = b - m;

    if (ll > rl) {
        if (rl <= SMALL_MERGE) mergeInPlaceBWLeft(self, array, a, m, b);
        else                   mergeOOPBWLeft(self, array, a, m, b);
    } else {
        if (ll <= SMALL_MERGE) mergeInPlaceFWLeft(self, array, a, m, b);
        else                   mergeOOPFWLeft(self, array, a, m, b);
    }
}

inline char optiMerge(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    REDUCE_BOUNDS(array, a, m, b);
    return optiSmartMergeLeft(self, array, a, m, b);
}

inline void getBlocksIndices(size_t* indices, VAR* array, size_t a, size_t leftBlocks, size_t rightBlocks, size_t blockLen) {
    size_t l = 0,
           m = leftBlocks,
           r = m,
           b = m + rightBlocks;
           
    size_t* o = indices;

    for (; l < m && r < b; o++) {
        if (CMP(
            array + a + (l + 1) * blockLen - 1, 
            array + a + (r + 1) * blockLen - 1
        ) <= 0) *o = l++;
        else    *o = r++;
    }

    for (; l < m; o++, l++) *o = l;
    for (; r < b; o++, r++) *o = r;
}

#define EXT 0
#define IN_OUT_FN(NAME) NAME##InPlace 

#include "inOut.h"

#undef EXT
#undef IN_OUT_FN

#define EXT 1
#define IN_OUT_FN(NAME) NAME##OOP 

#include "inOut.h"

#undef EXT
#undef IN_OUT_FN

inline void blockCycle(HeliumData* self, VAR* array, size_t* indices, size_t a, size_t leftBlocks, size_t rightBlocks, size_t blockLen) {
    size_t total = leftBlocks + rightBlocks;

    VAR* extBuf = self->extBuf;

    for (size_t i = 0; i < total; i++) {
        if (i != indices[i]) {
            memcpy(extBuf, array + a + i * blockLen, blockLen * sizeof(VAR));
            size_t j = i,
                   n = indices[i];

            do {
                memcpy(array + a + j * blockLen, array + a + n * blockLen, blockLen * sizeof(VAR));
                indices[j] = j;

                j = n;
                n = indices[n];
            } while (n != i);

            memcpy(array + a + j * blockLen, extBuf, blockLen * sizeof(VAR));
            indices[j] = j;
        }
    }
}

void hydrogenCombine(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    CHECK_BOUNDS(self, array, a, m, b);
    if (optiMerge(self, array, a, m, b)) return;

    size_t blockLen    = self->blockLen,
           leftBlocks  = (m - a) / blockLen,
           rightBlocks = (b - m) / blockLen,
           blockQty    = leftBlocks + rightBlocks,
           frag        = (b - a) - blockQty * blockLen;

    size_t* indices = self->indices;
    getBlocksIndices(indices, array, a, leftBlocks, rightBlocks, blockLen);
    memcpy(self->keys, indices, blockQty * sizeof(size_t));

    blockCycle(self, array, indices, a, leftBlocks, rightBlocks, blockLen);
    mergeBlocksOOP(self, array, a, leftBlocks, blockQty, blockLen, frag);
} 

void hydrogenLoop(HeliumData* self, VAR* array, size_t a, size_t b) {
    size_t r  = RUN_SIZE,
           l  = b - a,
           bl = self->extBufLen;

    while (r <= bl) {
        size_t twoR = r << 1, i;
        for (i = a; i + twoR < b; i += twoR)
            mergeOOP(self, array, i, i + r, i + twoR);

        if (i + r < b) mergeOOP(self, array, i, i + r, b);

        r = twoR;
    }

    while (r < l) {
        size_t twoR = r << 1, i;
        for (i = a; i + twoR < b; i += twoR)
            hydrogenCombine(self, array, i, i + r, i + twoR);

        if (i + r < b) hydrogenCombine(self, array, i, i + r, b);

        r = twoR;
    }
}

// helium mode loop

#define STRAT4 0
#define EXT_KEYS 0
#define EXT_BUF 0
#define INT_BUF 1
#define FN_NAME strat3CLoop

#include "heliumLoop.h"

#undef STRAT4
#undef EXT_KEYS
#undef EXT_BUF
#undef INT_BUF
#undef FN_NAME

#define STRAT4 0
#define EXT_KEYS 0
#define EXT_BUF 1
#define INT_BUF 1
#define FN_NAME strat3CLoopExtraBuf

#include "heliumLoop.h"

#undef STRAT4
#undef EXT_KEYS
#undef EXT_BUF
#undef INT_BUF
#undef FN_NAME

#define STRAT4 0
#define EXT_KEYS 0
#define EXT_BUF 1
#define INT_BUF 0
#define FN_NAME strat3BLoop

#include "heliumLoop.h"

#undef STRAT4
#undef EXT_KEYS
#undef EXT_BUF
#undef INT_BUF
#undef FN_NAME

#define STRAT4 0
#define EXT_KEYS 1
#define EXT_BUF 1
#define INT_BUF 0
#define FN_NAME strat3ALoop

#include "heliumLoop.h"

#undef STRAT4
#undef EXT_KEYS
#undef EXT_BUF
#undef INT_BUF
#undef FN_NAME

// strat 4

#define STRAT4 1
#define EXT_KEYS 0
#define EXT_BUF 0
#define INT_BUF 1
#define FN_NAME strat4BLoop

#include "heliumLoop.h"

#undef STRAT4
#undef EXT_KEYS
#undef EXT_BUF
#undef INT_BUF
#undef FN_NAME

#define STRAT4 1
#define EXT_KEYS 0
#define EXT_BUF 1
#define INT_BUF 1
#define FN_NAME strat4BLoopExtraBuf

#include "heliumLoop.h"

#undef STRAT4
#undef EXT_KEYS
#undef EXT_BUF
#undef INT_BUF
#undef FN_NAME

#define STRAT4 1
#define EXT_KEYS 0
#define EXT_BUF 1
#define INT_BUF 0
#define FN_NAME strat4ALoop

#include "heliumLoop.h"

#undef STRAT4
#undef EXT_KEYS
#undef EXT_BUF
#undef INT_BUF
#undef FN_NAME

inline void reverse(VAR* array, size_t a, size_t b) {
    for (--b; a < b; a++, b--) swap(array, a, b);
}

void reverseRuns(VAR* array, size_t a, size_t b) {
    size_t l = a;
    while (l < b) {
        size_t i = l;
        for (; i < b - 1; i++)
            if (CMP(array + i, array + i + 1) <= 0)
                break;

        if (i - l >= MIN_REV_RUN_SIZE) reverse(array, l, i);
        l = i + 1;
    }
}

size_t checkSortedIdx(VAR* array, size_t a, size_t b) {
    reverseRuns(array, a, b);

    while (--b > a) 
        if (CMP(array + b, array + b - 1) < 0)
            return b;

    return a;
}

void uraniumLoop(HeliumData* self, VAR* array, size_t a, size_t b) {
    size_t r  = RUN_SIZE,
           l  = b - a;

    while (r < l) {
        size_t twoR = r << 1, i;
        for (i = a; i + twoR < b; i += twoR)
            mergeOOP(self, array, i, i + r, i + twoR);

        if (i + r < b) mergeOOP(self, array, i, i + r, b);

        r = twoR;
    }
}

size_t findKeysUnsorted(HeliumData* self, VAR* array, size_t a, size_t p, size_t b, size_t q, size_t to) {
    size_t n = b - p;

    for (size_t i = p; i > a && n < q; i--) {
        size_t l = binarySearchLeft(array, p, p + n, array + i - 1) - p;
        if (l == n || CMP(array + i - 1, array + p + l) < 0) {
            rotateOOP(self, array, i, p, p + n++);
            p = i - 1;
            insertToRight(array, i - 1, p + l);
        }
    }

    rotateOOP(self, array, p, p + n, to);
    return n;
}

size_t findKeysSorted(HeliumData* self, VAR* array, size_t a, size_t b, size_t q) {
    size_t n = 1,
           p = b - 1;

    for (size_t i = p; i > a && n < q; i--) {
        if (CMP(array + i - 1, array + i) != 0) {
            rotateOOP(self, array, i, p, p + n++);
            p = i - 1;
        }
    }

    if (n == q) rotateOOP(self, array, p, p + n,     b);
    else        rotateOOP(self, array, a,     p, p + n);

    return n;
}

inline char findKeys(size_t* found, HeliumData* self, VAR* array, size_t a, size_t b, size_t q) {
    size_t p = checkSortedIdx(array, a, b);
    if (p == a) return 1;

    if (b - p < MIN_SORTED_UNIQUE) {
        *found = findKeysUnsorted(self, array, a, b - 1, b, q, b);
        return 0;
    } 

    size_t n = findKeysSorted(self, array, p, b, q);
    if (n == q) {
        *found = n;
        return 0;
    }
    
    *found = findKeysUnsorted(self, array, a, p, p + n, q, b);
    return 0;
}
    
// strategy 5
void inPlaceMergeSort(HeliumData* self, VAR* array, size_t a, size_t b, size_t p) {
    sortRuns(array, a, b, p);
    
    size_t r = RUN_SIZE,
           l = b - a;

    while (r < l) {
        size_t twoR = r << 1, i;                                     
        for (i = a; i + twoR < b; i += twoR)
            mergeInPlaceWithCheck(self, array, i, i + r, i + twoR);

        if (i + r < b) mergeInPlaceWithCheck(self, array, i, i + r, b);

        r = twoR;
    } 
}

inline void inPlaceMergeSortWithCheck(HeliumData* self, VAR* array, size_t a, size_t b) {
    size_t p = checkSortedIdx(array, a, b);
    if (p == a) return;
    inPlaceMergeSort(self, array, a, b, p);
}

void heliumSort(VAR* array, size_t a, size_t b, size_t mem) {
    size_t n = b - a;
    if (n <= SMALL_SORT) {
        HeliumData self;
        self.extBufLen = 0;
        self.rotate    = rotateOOP;

        inPlaceMergeSortWithCheck(&self, array, a, b);
        return;
    }

    size_t h = n >> 1;
    if (mem == 1 || mem >= h) {
        if (mem == 1) mem = h;

        size_t p = checkSortedIdx(array, a, b);
        if (p == a) return;
        sortRuns(array, a, b, p);

        HeliumData self;
        self.extBuf    = (VAR*)malloc(mem * sizeof(VAR));
        self.extBufLen = mem; 
        self.rotate    = rotateOOP;

        uraniumLoop(&self, array, a, b);

        free(self.extBuf);
        return;
    }

    size_t sqrtn = 1;
    for (; sqrtn * sqrtn < n; sqrtn <<= 1);
    size_t keySize = n / sqrtn,
           twoK    = keySize << 1,
           ideal   = sqrtn + twoK;

    if (mem == 2 || mem >= ideal) {
        if (mem == 2) mem = ideal;
        else if (mem != ideal) {
            for (; sqrtn + ((n / sqrtn) << 1) <= mem; sqrtn <<= 1);
            sqrtn >>= 1;
            keySize = n / sqrtn;
            twoK = keySize << 1;
        }

        size_t p = checkSortedIdx(array, a, b);
        if (p == a) return;
        sortRuns(array, a, b, p);

        size_t* indices = (size_t*)malloc(twoK * sizeof(size_t));

        mem -= twoK;

        HeliumData self;
        self.indices   = indices;
        self.keys      = indices + keySize;
        self.extKeyLen = keySize;
        self.extBuf    = (VAR*)malloc(mem * sizeof(VAR));
        self.extBufLen = mem;

        self.hasExtBuf = 1;
        self.hasIntBuf = 0;
        self.blockLen = sqrtn;

        self.rotate = rotateOOP;

        hydrogenLoop(&self, array, a, b);

        free(indices);
        free(self.extBuf);
        return;
    }

    ideal = sqrtn + keySize;
    if (mem == 3 || mem >= ideal) {
        if (mem == 3) mem = ideal;
        else if (mem != ideal) {
            for (; sqrtn + ((n / sqrtn) << 1) <= mem; sqrtn <<= 1);
            sqrtn >>= 1;
            keySize = n / sqrtn;
        }

        size_t p = checkSortedIdx(array, a, b);
        if (p == a) return;
        sortRuns(array, a, b, p);

        mem -= keySize;

        HeliumData self;
        self.extBuf    = (VAR*)malloc(mem * sizeof(VAR));
        self.extBufLen = mem;
        self.keys      = (size_t*)malloc(keySize * sizeof(size_t));
        self.extKeyLen = keySize;
        self.rotate    = rotateOOP;

        self.blockLen  = sqrtn;
        self.hasExtBuf = 1;
        self.hasIntBuf = 0;

        strat3ALoop(&self, array, a, b);

        free(self.keys);
        free(self.extBuf);
        return;
    }

    if (mem == 4 || mem >= sqrtn) {
        if (mem == 4) mem = sqrtn;
        else if (mem != sqrtn) {
            for (; sqrtn <= mem; sqrtn <<= 1);
            sqrtn >>= 1;
            keySize = n / sqrtn;
        }

        HeliumData self;
        self.extBuf    = (VAR*)malloc(mem * sizeof(VAR));
        self.extBufLen = mem;
        self.rotate    = rotateOOP;

        size_t keysFound;
        if (findKeys(&keysFound, &self, array, a, b, keySize)) return;
        
        char ideal = keysFound == keySize;
        if ((!ideal) && keysFound <= MAX_STRAT5_UNIQUE) {
            inPlaceMergeSort(&self, array, a, b, b);
            free(self.extBuf);
            return;
        }

        size_t e = b - keysFound;
        sortRuns(array, a, e, e);

        self.keyLen    = keysFound;
        self.keyPos    = e;
        self.bufLen    = 0;
        self.hasExtBuf = 1;
        self.hasIntBuf = 0;
        self.blockLen  = sqrtn;

        if (ideal) strat3BLoop(&self, array, a, e);
        else       strat4ALoop(&self, array, a, e);
            
        free(self.extBuf);
        return;
    }
    
    HeliumData self;
    self.rotate = rotateOOP;

    char hasMem = mem > 0;
    if (hasMem) {
        self.hasExtBuf = 1;
        self.extBuf    = (VAR*)malloc(mem * sizeof(VAR));
        self.extBufLen = mem;
    } else self.extBufLen = 0;

    ideal = sqrtn + keySize;
    size_t keysFound;
    if (findKeys(&keysFound, &self, array, a, b, ideal)) return;

    if (keysFound <= MAX_STRAT5_UNIQUE) {
        inPlaceMergeSort(&self, array, a, b, b);
        if (hasMem) free(self.extBuf);
        return;
    }

    self.hasIntBuf = 1;
    size_t e = b - keysFound;
    sortRuns(array, a, e, e);

    if (keysFound == ideal) {
        self.blockLen = sqrtn;
        self.bufLen   = sqrtn;
        self.bufPos   = b - sqrtn;
        self.keyLen   = keySize;
        self.keyPos   = e;
        
        if (hasMem) {
            if (sqrtn > (mem << 1)) self.rotate = rotateInPlace;
            strat3CLoopExtraBuf(&self, array, a, e);
            free(self.extBuf);
        } else {
            self.rotate = rotateInPlace;
            strat3CLoop(&self, array, a, e);
        }
    } else {
        self.blockLen = SQRT_TWOR;
        self.bufLen   = keysFound;
        self.keyLen   = keysFound;
        self.bufPos   = e;
        self.keyPos   = e;

        if (hasMem) {
            if (keysFound > (mem << 1)) self.rotate = rotateInPlace;
            strat4BLoopExtraBuf(&self, array, a, e);
            free(self.extBuf);
        } else {
            self.rotate = rotateInPlace;
            strat4BLoop(&self, array, a, e);
        }
    }
}