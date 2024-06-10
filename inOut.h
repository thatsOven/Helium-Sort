void IN_OUT_FN(rotate)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    size_t rl = b - m,
           ll = m - a;

    #if EXT
        size_t bl  = self->extBufLen;
        VAR*   buf = self->extBuf;
    #else
        size_t bl  = self->bufLen,
               buf = self->bufPos;
    #endif

    size_t min = (rl != ll && (bl < ll ? (bl < rl ? bl : rl) : (ll < rl ? ll : rl)) > SMALL_MERGE) ? bl : 1; 

    while ((rl > min && ll > min) || (rl < SMALL_MERGE && rl > 1 && ll < SMALL_MERGE && ll > 1)) {
        if (rl < ll) {
            blockSwapFW(array, a, m, rl);
            a  += rl;
            ll -= rl;
        } else {
            b  -= ll;
            rl -= ll;
            blockSwapBW(array, a, b, ll);
        }
    }

    if      (rl == 1) insertToLeft( array, m, a);
    else if (ll == 1) insertToRight(array, a, b - 1);
    if (min == 1 || rl <= 1 || ll <= 1) return;

    #if EXT
        if (rl < ll) {
            memcpy(buf           , array + m, rl * sizeof(VAR));
            memcpy(array + b - ll, array + a, ll * sizeof(VAR));
            memcpy(array + a     , buf      , rl * sizeof(VAR));
        } else {
            memcpy(buf           , array + a, ll * sizeof(VAR));
            memcpy(array + a     , array + m, rl * sizeof(VAR));
            memcpy(array + b - ll,       buf, ll * sizeof(VAR));
        }
    #else
        if (rl < ll) {
            blockSwapBW(array, m, buf, rl);

            for (size_t i = m + rl; i-- > a + rl;)
                swap(array, i, i - rl);
                
            blockSwapBW(array, buf, a, rl);
        } else {
            blockSwapFW(array, a, buf, ll);

            for (size_t i = a; i + ll < b; i++)
                swap(array, i, i + ll);

            blockSwapFW(array, buf, b - ll, ll);
        }
    #endif
}

inline void IN_OUT_FN(blockSelect)(HeliumData* self, VAR* array, size_t a, size_t leftBlocks, size_t rightBlocks, size_t blockLen) {
    #if EXT
        size_t stKey = 0,
               i1    = 0,
               k     = 0;

        size_t* keys = self->keys;
    #else
        size_t stKey = self->keyPos,
               i1    = stKey,
               k     = stKey;
    #endif

    size_t tm = stKey + leftBlocks,
           j1 = tm,
           tb = tm + rightBlocks;

    while (k < j1 && j1 < tb) {
        if (CMP(
            array + a + (i1 - stKey + 1) * blockLen - 1, 
            array + a + (j1 - stKey + 1) * blockLen - 1  
        ) <= 0) {
            if (i1 > k) blockSwapFW(array, a + (k - stKey) * blockLen, a + (i1 - stKey) * blockLen, blockLen);
            
            #if EXT
                indexSwap(keys, k++, i1);
            #else
                swap(array, k++, i1);
            #endif

            i1 = k;
            for (size_t i = (k + 1 > tm ? k + 1 : tm); i < j1; i++) {
                #if EXT
                    if (keys[i] < keys[i1]) i1 = i;
                #else
                    if (CMP(array + i, array + i1) < 0) i1 = i;
                #endif
            }
                
        } else {
            blockSwapFW(array, a + (k - stKey) * blockLen, a + (j1 - stKey) * blockLen, blockLen);

            #if EXT
                indexSwap(keys, k, j1++);
            #else
                swap(array, k, j1++);
            #endif

            if (i1 == k++) i1 = j1 - 1;
        }
    } 

    while (k < j1 - 1) {
        if (i1 > k) blockSwapFW(array, a + (k - stKey) * blockLen, a + (i1 - stKey) * blockLen, blockLen);

        #if EXT
            indexSwap(keys, k++, i1);
        #else
            swap(array, k++, i1);
        #endif

        i1 = k;
        for (size_t i = k + 1; i < j1; i++) {
            #if EXT
                if (keys[i] < keys[i1]) i1 = i;
            #else
                if (CMP(array + i, array + i1) < 0) i1 = i;
            #endif
        }
    }
}

#if EXT
    #define COMPARE_KEYS(keys, key, midKey) (keys[key] < (midKey))
    #define K_TYPE size_t
#else
    #define COMPARE_KEYS(array, key, midKey) (CMP(array + (key), &(midKey)) < 0)
    #define K_TYPE VAR
#endif

inline void IN_OUT_FN(mergeBlocks)(HeliumData* self, VAR* array, size_t a, K_TYPE midKey, size_t blockQty, size_t blockLen, size_t lastLen) {
    #if EXT
        size_t* keys = self->keys;
        size_t stKey = 0;
    #else
        VAR* keys = array;
        size_t stKey = self->keyPos;
    #endif

    size_t f = a;
    char left = COMPARE_KEYS(keys, stKey, midKey);

    for (size_t i = 1; i < blockQty; i++) {
        if (left ^ COMPARE_KEYS(keys, stKey + i, midKey)) {
            size_t next = a + i * blockLen, nextEnd;

            if (left) {
                nextEnd = binarySearchLeft( array, next, next + blockLen, array + next - 1);
                smartMergeLeft(self, array, f, next, nextEnd);
            } else {
                nextEnd = binarySearchRight(array, next, next + blockLen, array + next - 1);
                smartMergeRight(self, array, f, next, nextEnd);
            }

            f    = nextEnd;
            left = !left;
        }
    }

    if (left && lastLen != 0) {
        size_t lastFrag = a + blockQty * blockLen;
        smartMergeLeft(self, array, f, lastFrag, lastFrag + lastLen);
    }
}

#undef COMPARE_KEYS
#undef K_TYPE

void IN_OUT_FN(heliumCombine)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    CHECK_BOUNDS(self, array, a, m, b);
    if (optiMerge(self, array, a, m, b)) return;

    size_t blockLen    = self->blockLen,
           leftBlocks  = (m - a) / blockLen,
           rightBlocks = (b - m) / blockLen,
           blockQty    = leftBlocks + rightBlocks,
           frag        = (b - a) - blockQty * blockLen;

    #if EXT
        size_t* keys = self->keys;
        for (size_t i = blockQty; i--;) keys[i] = i;
        size_t midKey = leftBlocks;
    #else
        size_t keyPos = self->keyPos;
        insertSort(array, keyPos, keyPos + blockQty + 1);
        VAR midKey = array[keyPos + leftBlocks];
    #endif

    IN_OUT_FN(blockSelect)(self, array, a, leftBlocks, rightBlocks, blockLen);
    IN_OUT_FN(mergeBlocks)(self, array, a, midKey, blockQty, blockLen, frag);
}