#include "db_access.h"
#include "db_convert.h"

long caFieldSize(int type, long nelem) {
    return dbr_size_n(dbf_type_to_DBR_CTRL(dbDBRnewToDBRold[type]), nelem);
}
