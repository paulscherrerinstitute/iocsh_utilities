#include "db_access.h"
#include "db_convert.h"

long caFieldSize(int type, long nelem) {
    /* add 4 to native type as DBR_CTRL size to work with new caget */
    return dbr_size_n(dbf_type_to_DBR_CTRL(dbDBRnewToDBRold[type]), nelem)+4;
}
