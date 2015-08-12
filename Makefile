include /ioc/tools/driver.makefile

BUILDCLASSES += Linux

SOURCES += listRecords.c
DBDS    += listRecords.dbd

SOURCES += updateMenuConvert.c
DBDS    += updateMenuConvert.dbd

SOURCES += addScan.c
DBDS    += addScan.dbd

SOURCES_3.14 += disctools.c
DBDS_3.14    += disctools.dbd

SOURCES_3.14 += exec.c
DBDS_3.14    += exec.dbd

SOURCES_3.14 += mlock.c
DBDS_3.14    += mlock.dbd

SOURCES_vxWorks += bootNotify.c

HEADERS_3.13 += epicsEndian.h
HEADERS_3.14.8 += epicsEndian.h
