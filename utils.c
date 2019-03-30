#include "utils.h"
#include <stdlib.h>
#include <string.h>

void resize_array(void **arr, int *arr_size, int element_size) {
    if ((*arr_size) == 0) {
        *arr = malloc(2*element_size);
        memset(*arr,0,2*element_size);
        *arr_size = 2;
        return;
    }
    void *tmp = *arr;
    *arr = malloc(2*(*arr_size)*element_size);
    memset(*arr,0,2*(*arr_size)*element_size);
    memcpy(*arr,tmp,(*arr_size)*element_size);
    free(tmp);
    *arr_size *= 2;
}