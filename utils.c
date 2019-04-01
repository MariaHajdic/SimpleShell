#include "utils.h"
#include <stdlib.h>
#include <string.h>

void resize_array(void **arr, int *arr_size, int element_size, int num_elements) {
    *arr = realloc(*arr, (*arr_size + 1) * 2 * element_size);
    memset(*arr + num_elements, 0, (*arr_size + 2) * element_size);
    *arr_size = (*arr_size + 1) * 2;
}