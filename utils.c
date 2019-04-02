#include "utils.h"
#include <stdlib.h>
#include <string.h>

void resize_array(char **arr, int *arr_size,  int num_elements) {
    *arr = realloc(*arr, (*arr_size + 1) * 2 * sizeof(char));
    memset(*arr + num_elements, 0, (*arr_size + 2) * sizeof(char));
    *arr_size = (*arr_size + 1) * 2;
}