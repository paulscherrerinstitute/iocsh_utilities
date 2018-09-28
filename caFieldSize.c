#include "db_access.h"
#include "db_convert.h"

long caFieldSize(int type, long nelem) {
    /* round up native type as DBR_CTRL size to next multiple of 4 */
    return (dbr_size_n(dbf_type_to_DBR_CTRL(dbDBRnewToDBRold[type]), nelem)+3)%4;
}
