size_t LEFT_RIGHT_FN(binarySearch)(VAR* array, size_t a, size_t b, VAR* value) {
    while (a < b) {
        size_t m = a + ((b - a) >> 1);

        if (LGT(array + m, value)) b = m;
        else                       a = m + 1;
    }

    return a;
}

void LEFT_RIGHT_FN(mergeInPlaceFW)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    size_t s = a,
           l = m;

    void (*rotate)(HeliumData*, VAR*, size_t, size_t, size_t);
    rotate = self->rotate;

    while (s < l && l < b) {
        if (RGT(array + s, array + l)) {
            size_t p = LEFT_RIGHT_FN(binarySearch)(array, l, b, array + s);
            rotate(self, array, s, l, p);
            s += p - l;
            l = p;
        } else s++;
    }
}

void LEFT_RIGHT_FN(mergeInPlaceBW)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    size_t s = b - 1,
           l = m;

    void (*rotate)(HeliumData*, VAR*, size_t, size_t, size_t);
    rotate = self->rotate;

    while (s >= l && l > a) {
        if (RGT(array + l - 1, array + s)) {
            size_t p = LEFT_RIGHT_FN_INV(binarySearch)(array, a, l - 1, array + s);
            rotate(self, array, p, l, s + 1);
            s -= l - p;
            l = p;
        } else s--;
    }
}

void LEFT_RIGHT_FN(mergeInPlace)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    if (m - a > b - m) LEFT_RIGHT_FN(mergeInPlaceBW)(self, array, a, m, b);
    else               LEFT_RIGHT_FN(mergeInPlaceFW)(self, array, a, m, b);
}

void LEFT_RIGHT_FN(mergeWithBufferFW)(VAR* array, size_t a, size_t m, size_t b, size_t buf) {
    size_t ll = m - a;
    blockSwapFW(array, a, buf, ll);

    size_t l = buf,
           r = m,
           o = a,
           e = buf + ll;

    for (; l < e && r < b; o++) {
        if (LLT(array + l, array + r)) swap(array, o, l++);
        else                           swap(array, o, r++);
    }

    while (l < e) swap(array, o++, l++);
}

void LEFT_RIGHT_FN(mergeWithBufferBW)(VAR* array, size_t a, size_t m, size_t b, size_t buf) {
    size_t rl = b - m;
    blockSwapBW(array, m, buf, rl);

    size_t l = m,
           r = buf + rl,
           o = b - 1;

    for (; l > a && r > buf; o--) {
        if (LGT(array + r - 1, array + l - 1)) swap(array, o, --r);
        else                                   swap(array, o, --l);
    }

    while (r > buf) swap(array, o--, --r);
}

void LEFT_RIGHT_FN(mergeOOPFW)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    size_t ll = m - a;
    VAR* extBuf = self->extBuf;
    VAR* o = array + a;
    memcpy(extBuf, o, ll * sizeof(VAR));

    size_t l = 0,
           r = m,
           e = ll;

    for (; l < e && r < b; o++) {
        if (LLT(extBuf + l, array + r)) *o = extBuf[l++];
        else                            *o = array[r++];
    }

    memcpy(o, extBuf + l, (e - l) * sizeof(VAR));
}

void LEFT_RIGHT_FN(mergeOOPBW)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    size_t rl = b - m;
    VAR* extBuf = self->extBuf;
    memcpy(extBuf, array + m, rl * sizeof(VAR));

    size_t l = m,
           r = rl;

    VAR* o = array + b - 1;

    for (; l > a && r > 0; o--) {
        if (LGT(extBuf + r - 1, array + l - 1)) *o = extBuf[--r];
        else                                    *o = array[--l];
    }
    
    while (r) *(o--) = extBuf[--r];
}

char LEFT_RIGHT_FN(optiSmartMerge)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    size_t ll = m - a,
           rl = b - m;

    if (ll > rl) {
        if (rl <= SMALL_MERGE) {
            LEFT_RIGHT_FN(mergeInPlaceBW)(self, array, a, m, b);
            return 1;
        }

        if (self->hasExtBuf && rl <= self->extBufLen)
            LEFT_RIGHT_FN(mergeOOPBW)(self, array, a, m, b);
        else if (self->hasIntBuf && rl <= self->bufLen)
            LEFT_RIGHT_FN(mergeWithBufferBW)(array, a, m, b, self->bufPos);
        else return 0;
    } else {
        if (ll <= SMALL_MERGE) {
            LEFT_RIGHT_FN(mergeInPlaceFW)(self, array, a, m, b);
            return 1;
        }

        if (self->hasExtBuf && ll <= self->extBufLen)
            LEFT_RIGHT_FN(mergeOOPFW)(self, array, a, m, b);
        else if (self->hasIntBuf && ll <= self->bufLen)
            LEFT_RIGHT_FN(mergeWithBufferFW)(array, a, m, b, self->bufPos);
        else return 0;
    }

    return 1;
}

void LEFT_RIGHT_FN(smartMerge)(HeliumData* self, VAR* array, size_t a, size_t m, size_t b) {
    if (LEFT_RIGHT_FN(optiSmartMerge)(self, array, a, m, b)) return;
    LEFT_RIGHT_FN(mergeInPlace)(self, array, a, m, b);
}