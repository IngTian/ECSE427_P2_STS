## How to run tests?
Follow the following examples.
```console
# Test 1
gcc sut.c test1.c -pthread -o test
./test

# Test 2
gcc sut.c test2.c -pthread -o test
./test

# Test 3
gcc sut.c test3.c -pthread -o test
./test

# Test 4
gcc sut.c test4.c -pthread -o test
./test

# Test 5
gcc sut.c test5.c -pthread -o test
./test
```
---
## File Structure

The Simple User Threadign Library, `sut.c`, is designed to work with the file structure as shown below.
```text
├── queue
│   ├── queue_example.c
│   └── queue.h
├── README.md
├── sut.c
├── sut.h
├── test1.c
├── test2.c
├── test3.c
├── test4.c
├── test5.c
```
The only local dependency we need is `queue.h`. If `gcc` is unable to compile due to `#include "queue/queue.h"`, please check the file structure.

---
## Control the Number of C_EXE

To toggle the number of `C_EXE`, simply change the global constant `NUM_OF_C_EXEC`.