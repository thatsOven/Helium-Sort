# Helium Sort
A block merge sorting algorithm inspired by [Grail Sort](https://github.com/Mrrl/GrailSort/) and focused on adaptivity.

Time complexity:
- Best case: O(n)
- Average case: O(n log n)
- Worst case: O(n log n)

Space complexity is variable, but the algorithm can efficiently run using O(1) memory.

The algorithm extends the concept of adaptivity to memory, by using different strategies based on the amount of memory given to it. 

To see Helium Sort's practical performance, jump to [Benchmarks](#benchmarks).

## Usage
- Include `heliumSort.h` in your project and define VAR element type and CMP comparison function.
- Call `heliumSort(array, start, end, memory)` to sort `array`. Memory can be set to 0, 1, 2, 3, and 4 to make Helium Sort run in its default modes (respectively, Strategies 3C or 4B, 1, 2, 3A, 3B or 4A). 

# Algorithm
The algorithm proceeds as follows:
1) Reverse runs and scan for sortedness: 
    - Helium Sort starts of by scanning the array for reverse-sorted runs, and if it finds any which can be safely reversed without exchaning the order of equal elements, it does, so it can later exploit the presence of sorted segments to skip portions of the sorting procedure. The array is then scanned backwards for sortedness. This is used to reduce the range in which the initial run sorting is performed, or to significantly improve the performance of the key finding routine. If the array is already sorted at this stage, the algorithm simply returns.
2) Strategy selection: based on the amounts of memory the algorithm is allowed to use and the contents of the original array, Helium Sort will pick a different strategy:
    - **Memory >= n / 2**: [Uranium Mode (Strategy 1)](#uranium-mode)
    - **Memory >= 2 ^ ceil(log2(sqrt(n))) + 2n / (2 ^ ceil(log2(sqrt(n))))**: [Hydrogen Mode (Strategy 2)](#hydrogen-mode)
    - **Memory >= 0**: [Helium Mode (Macrostrategies 3 and 4)](#helium-mode)

# Uranium Mode
Since we have enough memory to do so, Uranium Mode simply consists in an optimized adaptive mergesort:
1) Runs of a given size are sorted using an optimized binary insertion sort that is able to avoid searching on pre-sorted segments.
2) External buffered merges are performed until just one segment is left. 

# Hydrogen Mode
Hydrogen mode consists in a block merge sorting algorithm that efficiently uses its available external space:
1) Like in [Uranium Mode](#uranium-mode), runs of a given size are sorted using optimized binary insertion sort.
2) External buffered merges are performed until the subarrays fit in the external buffer.
3) Block merging is performed according to the [Grail Sort scheme](#the-grail-sort-block-merging-scheme) until just one segment is left:
    - Blocks are sorted using fast block sorting, referred to as "block cycle sort": this is the asymptotically optimal way to sort blocks, as this process is done in O(sqrt n) comparisons and O(n) moves, while taking advantage of the external buffer to move the blocks. To find the blocks' order, a blockwise merging algorithm is performed, and indices are written to external memory, marking the final positions of the blocks. The indices are then copied over to a secondary array which will later be used for block merging, then block sorting is performed using the indices we generated before;
    - Sorted blocks are then merged using the external buffer.

# Helium Mode
Depending on the amount of memory the algorithm is allowed to work with, Helium Mode further splits into different sub-strategies.
- **Memory >= 2 ^ ceil(log2(sqrt(n))) + n / (2 ^ ceil(log2(sqrt(n))))**: Both an external buffer and external keys are allocated - Strategy 3A;
- **Memory >= 2 ^ ceil(log2(sqrt(n)))**: An external buffer is allocated, the algorithm will try to find `n / (2 ^ ceil(log2(sqrt(n))))` keys inside the array - Strategy 3B or 4A;
- **Memory >= 0**: The algorithm will try to find `2 ^ ceil(log2(sqrt(n))) + n / (2 ^ ceil(log2(sqrt(n))))` items inside the array to form internal keys and an internal buffer - Strategy 3C or 4B.

Strategies 4A and 4B are fallbacks for when the array does not contain enough items to form the ideal internal buffer:
- **Strategy 4A**: The algorithm has enough memory to allocate an external buffer without needing an internal one, but not enough unique items are found to form a fully-sized key buffer. In these cases, it will try to perform block merging normally until its keys are enough to do so, then it will increase the size of the blocks in order to reduce the number of keys needed.
- **Strategy 4B**: The algorithm has a fully internal buffer. To better take advantage of it, instead of having `2 ^ ceil(log2(sqrt(n)))` sized blocks like usual, the algorithm will try to resize its blocks to `2 ^ ceil(log2(sqrt(2 * r)))` where `r` is the current size of the subarrays. This allows the algorithm to temporarily split the internal buffer and use it to perform merges, even if it's undersized for normal operation. When having smaller blocks is not possible anymore (because the amount of keys we have is limited), blocks are resized like in strategy 4A, increasing their size until the amount of keys available is enough.
- In both of these cases, when blocks can't fit in neither the internal or external buffer, the algorithm resorts to naive in-place rotation merges.

The algorithm operates as follows:
1) If needed, an internal buffer is formed by finding unique items in O(n log n) time worst case.
2) If the unique items that have been found are less or equal than 8, the algorithm switches to [Strategy 5](#strategy-5---adaptive-lazy-stable-sort).
2) Runs of a given size are sorted.
3) If an external buffer is available, external buffered merges are performed until the subarrays fit in the external buffer.
4) If an internal buffer is available, the same is done with the internal buffer.
5) Block merging is performed according to the [Grail Sort scheme](#the-grail-sort-block-merging-scheme) until just one segment is left:
    - Depending on the current strategy, block merging will be performed differently:
        - If external keys are available, ordered keys are generated on the fly based on the quantity of blocks we have to sort and merge. This only happens in strategy 3A.
        - The external buffer will be used for block merging whenever it's possible to do so.
    - Blocks are sorted using an heavily optimized blockwise selection sort, then are merged using the available buffer.
6) If an internal buffer was formed, it gets sorted using optimized binary insertion sort, and then "redistributed" into the array using a naive in-place rotation merge routine, or the external buffer if possible.

# Strategy 5 - Adaptive Lazy Stable Sort
Strategy 5 is called when very few unique items are found in the array, or the array size is less or equal than 256. Since when very few unique items are found in the array, the internal buffer formed will in turn be very small, it's more efficient to use an algorithm which's complexity lowers with lower amounts of unique items. Adaptive Lazy Stable Sort consists in a merge sort algorithm which uses naive in-place rotation merges, which are faster when smaller subarrays are used or very few unique items are present. This strategy also benefits of the [adaptive optimizations](#adaptive-optimizations) from the main algorithm.

# The Grail Sort block merging scheme
Different block merge sorting algorithms use different block merging schemes depending on their designs. Helium Sort is designed to operate using Grail Sort's scheme, which can be roughly summarized into "sort blocks, then merge them". Different orders or different techniques to perform these operations are possible, each with their individual benefits and downsides. The Grail Sort scheme has been chosen for this algorithm, despite it being fairly complicated, because it easily allows to apply a great number of optimizations, especially for adaptivity. It operates as follows:
1) Blocks are sorted: in Grail Sort, this is done using a blockwise selection sort (selection sort uses O(n^2) comparisons and O(n) moves to sort, but since it's used on O(sqrt n) blocks, `sqrt(n)^2 = n`, giving us linear complexity). Every movement performed by block sorting is also performed on the keys (which have been sorted before starting to sort our blocks). This will allow the algorithm to track the subarray of origin of each block, in order to keep it stable.
2) Once blocks are in their correct order, they have to be merged to produce a sorted subarray: each block is merged with the next one, and the original order of equal items is restored by prioritizing either left or right elements depending on the subarray of origin of each block. The origin can be determined by comparing the key relative to the current block to the "mid key", which is the key at position `number_of_left_blocks` before block sorting. If the current key is less than the middle key, the origin of our current block was the left subarray, so we should prioritize equal items coming from the left block. Otherwise, the origin of the current block was the right subarray, so equal elements from the right block are prioritized.

# Adaptive optimizations
Helium Sort uses a number of different tricks to improve its adaptivity, but most notably:
- Every merge is simplified to a naive in-place merge routine in case of small subarrays (<= 16 items) to avoid copying data to a buffer, which usually results in accesses beyond
cache bounds, hence creating cache misses.
- Each merge routine also uses two techniques that allow it to skip merging parts of the subarray, or skipping merging altogether:
    - Bounds checking: consists in checking the upper and lower bounds of the subarrays to merge to figure out if they are already in the correct order, or if they might need to be exchanged;
    - Bounds reduction: the first item of the right subarray is binary searched into the left subarray, and the last item of the left subarray is searched into the right subarray. These searches will return the index from which the merge process will actually need to start and finish. Doing this allows us to skip copying and comparing big portions of the subarrays in case of beneficial patterns. On random data, this is usually not very beneficial and can be wasteful.
- When an internal buffer has to be formed, unlike Grail Sort, Helium Sort will look for keys backwards and form its buffer in the right end of the array. This sounds like a minimal change, but having the buffer in the front of the array shifts the actual sorting routine, and makes some types of patterns unexploitable. Having the buffer on the right end allows the algorithm to treat the last merge, like normally, as a special case, but without actually shifting the bounds of the procedure.
- Unlike Grail Sort, Helium Sort will re-compute and increase its block size if it has more memory available than what it needs to work. This allows for much more efficient usage of memory.
- Helium Sort's rotation algorithm is a modified version of the original Gries-Mills algorithm used in Grail Sort. This modification allows the algorithm to have better access patterns, reduce overall writes, and actually use external and internal buffers to reduce the number of operations needed to complete the rotation.

# Benchmarks
In the following benchmarks, Helium Sort has been ran against other similar algorithms on various types of inputs and various amounts of memory. The other algorithms shown are:
- [**Grail Sort**](https://github.com/Mrrl/GrailSort/): The original algorithm that inspired Helium Sort;
- [**Sqrt Sort**](https://github.com/Mrrl/SqrtSort/): An algorithm similar to Grail Sort by the same author. Uses O(sqrt n) external memory;
- [**Wiki Sort**](https://github.com/BonzaiThePenguin/WikiSort): Another kind of block merge sorting algorithm;
- [**Log Sort**](https://github.com/aphitorite/Logsort): A novel in-place stable O(n log n) quicksort. Needs at least O(log n) external memory.

The tester and the relative sorts are compiled using `gcc -O3` using GCC 11.4.0 and ran on Ubuntu 22.04.2 LTS using WSL on Windows 10, on an AMD Ryzen 5 5500 CPU with 16 GB of RAM available. The algorithms sort a random distribution of 32-bit integers. The average time among 100 trials is recorded.

![](tables/16777216_Few%20unique.svg)
![](tables/16777216_sqrt(n)%20unique.svg)
![](tables/16777216_Most%20unique.svg)

More graphs and all relative data tables are found in `tables`.

You can run these tests by yourself by compiling `test.c` (using gcc: `gcc -O3 test.c -lm`) and running the result. The program's output can be written inside a `result.txt` file and you can run `csvgen.py` to generate `csv` files that can be easily imported into spreadsheet software.

# Conclusions
As the data shows, Helium Sort manages to consistently outperform Grail Sort, even on random data, where Helium Sort actually wastes comparisons that are usually useful for adaptivity purposes, and manages to perform quite closely to Sqrt Sort, despite it having an advantage since it always uses an amount of external memory. 

Wiki Sort performs significantly worse when running with no memory allocated, due to its design that makes it perform an higher amount of operations. This changes when memory is allocated to it, somehow. My guess as to why is mostly Wiki Sort's usage of the "ping pong merging" technique that allows it to skip a great number of data copies, which Helium Sort does not implement for adaptivity reasons, as ping pong merges cannot be optimized the same way. 

Helium Sort takes a big hit in the sqrt(n) unique distribution, however this is expected: in this case, when memory is not enough, Helium Sort will need to form an internal buffer, but this buffer will be undersized for normal operation, making it fall back to strategies 4A and 4B. These strategies are particularly slow compared to the normal algorithm operation. Despite this, Helium Sort still manages to outperform Grail Sort, and when enough memory is given, the performance is comparable to the other cases. 

As you can also see, Helium Sort manages to achieve its goal of adaptivity very well, also managing to outperform Log Sort on favourable patterns, despite it having much better access patterns due to it being a quicksort, and using branchless operations. 

Branchless operations have been also considered for Helium Sort, and they have shown a significant increase in speed on random data, however this lead to a loss in favourable patterns, since branches in those cases are predictable.

# Acknowledgements
Special thanks to members of the "The Studio" Discord server (https://discord.gg/thestudio) for sustaining me through the extremely long development and design process of this algorithm, and especially:
- **@aphitorite** ([github](https://github.com/aphitorite)): For creating the "smarter block selection" algorithm, as well as inspiring me to create this algorithm and helping me to understand some of the workings of block merge sorting algorithms throughout the whole process;
- **@MusicTheorist** ([github](https://github.com/MusicTheorist)): For creating very helpful animations and visualizations of Grail Sort, which made the learning process much easier, as well as taking any opportunity to explain every detail about the algorithm;
- All members of [The Holy Grail Sort Project](https://github.com/HolyGrailSortProject): For creating [Rewritten Grailsort](https://github.com/HolyGrailSortProject/Rewritten-Grailsort), which has been a great reference throughout the development of Helium Sort.
