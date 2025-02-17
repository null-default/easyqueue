# Easyqueue
Easyqueue is simple queue implementation in C intended to support flexible applicability and a simple interface. It encapsulates a fixed-size buffer in which items are stored that is augmented by a dynamically allocated singly-linked list when the fixed-size buffer is filled.

Other features include:

- Compile-time configurable fixed-size buffer length
- Optional maximum capacity, configurable per-queue
- Supports custom memory allocators
- No external dependencies

## Usage

After installation, simply include the `easyqueue.h` header file in your project to use the definitions. The Easyqueue API is straightforward and only consists of a few symbols:

|         **Symbol**          |        **Symbol Type**        | **Description**                                                                                                                                                      |
|:---------------------------:|:-------------------------:|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `EZQ_FIXED_BUFFER_CAPACITY` |  Preprocessor definition  | The number of items an `ezq_queue` will be able to store without necessitating dynamic allocation. See the [Configuration](#configuration) section for more details. |
|         `ezq_init`          |         Function          | Initializes an `ezq_queue` structure ("the queue").                                                                                                                  |
|         `ezq_push`          |         Function          | Returns the number of items in the queue.                                                                                                                            |
|          `ezq_pop`          |         Function          | Places a new item at the tail end of the queue.                                                                                                                      |
|         `ezq_count`         |         Function          | Retrieves an item from the front of the queue.                                                                                                                       |
|        `ezq_destroy`        |         Function          | Clears the queue and performs any necessary teardown.                                                                                                                |

Each of the API functions (except `ezq_count()`) returns an integer-based `ezq_status` value. `ezq_count()` instead returns the number of items in the queue and may optionally be passed an `ezq_status *` in which the resulting `ezq_status` value will be stored. If the return value of one of these functions is not `EZQ_STATUS_SUCCESS`, which indicates that the call was entirely successful, an error-specific code will be returned instead.

## Building

Easyqueue currently supports the following build systems, whose relevant files are included in this repository:

- [CMake](https://cmake.org/)
  - This project will be built with the target name `easyqueue`.

### Configuration

Easyqueue supports some configuration options (beyond what your build system may include by default), depending on your build system.

|    Option Type     |            Option/Flag            | Description                                                                                   | Default Value |
|:------------------:|:---------------------------------:|-----------------------------------------------------------------------------------------------|:-------------:|
|  Compilation Flag  |    `EZQ_FIXED_BUFFER_CAPACITY`    | Sets the number of items an `ezq_queue` will support before dynamically allocating new nodes. |     `32`      |
|   CMake Variable   | `EASYQUEUE_FIXED_BUFFER_CAPACITY` | CMake variable equivalent to the `EZQ_FIXED_BUFFER_CAPACITY` compilation flag.                |     `32`      |
|   CMake Variable   | `EASYQUEUE_BUILD_EXAMPLES`   | If set/defined, any example programs in the `examples/` directory will be built. |    _unset_    |

## Example

A more thorough example can be found in [`examples/example.c`](examples/example.c), but below is a brief example demonstrating usage of the Easyqueue API. Note that this example largely ignores the `ezq_status` value returned by the Easyqueue API functions.

```c
#include <stdio.h>
#include <stdlib.h>
#include "easyqueue.h"

int main(int argc, char **argv) {
    ezq_status estat;
    ezq_queue queue;
    int items[EZQ_FIXED_BUFFER_CAPACITY + 10]; // 10 will be heap-allocated
    int *popped_val;
    
    // initialize queue and set dynamic allocation funcs it will use if needed
    ezq_init(&queue, 0, malloc, free);
    
    // push items onto and pop items off of the queue
    for (int i = 0; i < sizeof(items)/sizeof(*items); ++i) {
        items[i] = i + 1;
        ezq_push(&queue, &items[i]);
    }
    printf("The queue contains %u items\n", ezq_count(&queue, NULL));
    for (int i = 0; i < sizeof(items)/sizeof(*items); ++i) {
        ezq_pop(&queue, (void **)&popped_val);
    }
    
    // tear down the queue, clearing it if it still contains any items
    estat = ezq_destroy(&queue, NULL, NULL);

    return EZQ_STATUS_SUCCESS == estat ? 0 : estat;
}
```
