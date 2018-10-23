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

SOURCES_3.14 += ulimit.c
DBDS_3.14    += ulimit.dbd

SOURCES      += epicsEnvUnset.c
DBDS_3.14    += epicsEnvUnset.dbd

ifndef BASE_3_16
SOURCES_3.14 += setMaxArrayBytes.c
SOURCES_3.14 += caFieldSize.c
DBDS_3.14    += setMaxArrayBytes.dbd
endif

ifndef BASE_3_15
SOURCES_3.14 += echo.c
DBDS_3.14    += echo.dbd
endif

ifndef BASE_7_0
SOURCES_3.14 += dbli.c
DBDS_3.14    += dbli.dbd
endif

SOURCES_3.14 += dbla.c
DBDS_3.14    += dbla.dbd

SOURCES_vxWorks += bootNotify.c
