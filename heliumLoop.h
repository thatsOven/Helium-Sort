void FN_NAME(HeliumData* self, VAR* array, size_t a, size_t b) {
    size_t r = RUN_SIZE, 
           l = b - a, bl;
           
    #if EXT_BUF
        bl = self->extBufLen;

        while (r <= bl) {
            size_t twoR = r << 1, i;
            for (i = a; i + twoR < b; i += twoR)
                mergeOOP(self, array, i, i + r, i + twoR);

            if (i + r < b) mergeOOP(self, array, i, i + r, b);

            r = twoR;
        }
    #endif
    
    #if INT_BUF
        bl = self->bufLen;

        while (r <= bl) {
            size_t twoR = r << 1, i;
            for (i = a; i + twoR < b; i += twoR)
                mergeWithBuffer(self, array, i, i + r, i + twoR);

            if (i + r < b) mergeWithBuffer(self, array, i, i + r, b);

            r = twoR;
        }
    #endif
    
    #if STRAT4
        size_t kLen = self->keyLen,
               kPos = self->keyPos;
    #endif

    while (r < l) {
        size_t twoR = r << 1, i;

        #if STRAT4
            #if EXT_BUF && !INT_BUF
                size_t bLen = self->blockLen;
                
                if (twoR / bLen + 1 > kLen) {
                    do bLen <<= 1; while (twoR / bLen + 1 > kLen);
                    self->blockLen = bLen;
                }                
            #else
                size_t sqrtTwoR = self->blockLen;
                for (; sqrtTwoR * sqrtTwoR < twoR; sqrtTwoR <<= 1);

                size_t kCnt = twoR / sqrtTwoR + 1;
                if (kCnt < kLen) {
                    self->bufLen = kLen - kCnt;
                    self->bufPos = kPos + kCnt;
                    self->hasIntBuf = 1;
                } else {
                    if (kCnt > kLen) {
                        do sqrtTwoR <<= 1; while (twoR / sqrtTwoR + 1 > kLen);
                    }

                    self->bufLen    = 0;
                    self->hasIntBuf = 0;
                }

                self->blockLen = sqrtTwoR;
            #endif
        #endif

        for (i = a; i + twoR < b; i += twoR) {
            #if EXT_KEYS
                heliumCombineOOP(self, array, i, i + r, i + twoR);
            #else
                heliumCombineInPlace(self, array, i, i + r, i + twoR);
            #endif
        }

        if (i + r < b) {
            #if STRAT4
                if (b - i - r <= kLen) {
                    self->bufPos = kPos;
                    self->bufLen = kLen;
                }
            #endif

            #if EXT_KEYS
                heliumCombineOOP(self, array, i, i + r, b);
            #else
                heliumCombineInPlace(self, array, i, i + r, b);
            #endif
        }

        r = twoR;
    }

    #if !(EXT_KEYS && EXT_BUF)
        size_t s = self->keyPos, e;

        #if STRAT4
            e = s + kLen;
        #else
            e = s + self->keyLen + self->bufLen;
        #endif

        self->bufLen = 0;
        self->hasIntBuf = 0;

        insertSort(array, s, e);

        REDUCE_BOUNDS(array, a, s, e);
        if (!optiSmartMergeLeft(self, array, a, s, e))
            mergeInPlaceLeft(self, array, a, s, e);
    #endif
}