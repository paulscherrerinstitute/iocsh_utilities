#include "db_access.h"

long caFieldSize(int type, long nelem) {
    /* We don't know which type a client will request.
       Let's prepare for the worst. */
    return sizeof(struct dbr_ctrl_double) - sizeof(dbr_double_t) + nelem * sizeof(dbr_string_t);
}
