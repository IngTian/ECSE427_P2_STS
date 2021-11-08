## How to run tests?
Follow the example.
```console
gcc sut.c test1.c -pthread -o test
./test
```

## File Structure

The Simple User Threadign Library, `sut.c`, is designed to work with the following file structure.
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

The only local dependency we need is `queue.h`. If `gcc` is unable to compile with `#include "queue/queue.h";`, please check the file structure.

## Control the Number of C_EXE

To toggle the number of `C_EXE`, simply change the global constant `NUM_OF_C_EXEC`.