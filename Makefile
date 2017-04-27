include /ioc/tools/driver.makefile

BUILDCLASSES += Linux

DBDS = -none-

SOURCES      += listRecords.c
DBDS_3.14    += listRecords.dbd

SOURCES      += updateMenuConvert.c
DBDS_3.14    += updateMenuConvert.dbd

SOURCES      += addScan.c
DBDS_3.14    += addScan.dbd

SOURCES      += dbll.c
DBDS_3.14    += dbll.dbd

SOURCES      += cal.c
DBDS_3.14    += cal.dbd

SOURCES_3.13 += glob.c

SOURCES_3.14 += disctools.c
DBDS_3.14    += disctools.dbd

SOURCES_3.14 += exec.c
DBDS_3.14    += exec.dbd

SOURCES_3.14 += mlock.c
DBDS_3.14    += mlock.dbd

SOURCES_3.14 += setMaxArrayBytes.c
DBDS_3.14    += setMaxArrayBytes.dbd

SOURCES_3.14 += dbli.c
DBDS_3.14    += dbli.dbd

SOURCES_vxWorks += bootNotify.c
