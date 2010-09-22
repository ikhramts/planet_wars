//Available under GPLv3.
//Author: Iouri Khramtsov.

#include "utils.h"

//General-purpose utilities.
void forceCrash() {
    int* bad;
    bad = NULL;
    *bad = 2;

    int x = 2;
}
