#pragma once

#include <stdlib.h>

#define IPA_ALLOC(obj) malloc(sizeof(obj));
#define IPA_ALLOC_N(n) malloc(n);
#define IPA_FREE(obj) free(obj)
#define IPA_REALLOC(obj, n) realloc(obj, n)
