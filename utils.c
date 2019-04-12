#include "utils.h"
#include <stdlib.h>
#include <string.h>

int resize_array(char **arr, int arr_size) {
    *arr = realloc(*arr, (arr_size + 1) * 2 * sizeof(char));
    memset(*arr + arr_size, 0, (arr_size + 2) * sizeof(char));
    return (arr_size + 1) * 2;
}