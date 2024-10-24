include /ioc/tools/driver.makefile

BUILDCLASSES += vxWorks Linux WIN32

# cannot build required 'utilities' for vxWorks 6.6
EXCLUDE_ARCHS += V66

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

SOURCES      += exec.c
DBDS_3.14    += exec.dbd

SOURCES      += ifEnvSet.c
DBDS_3.14    += ifEnvSet.dbd

SOURCES_3.14 += mlock.c
DBDS_3.14    += mlock.dbd

SOURCES_3.14 += ulimit.c
DBDS_3.14    += ulimit.dbd

ifndef BASE_7_0
SOURCES      += epicsEnvUnset.c
DBDS_3.14    += epicsEnvUnset.dbd
HEADERS      += epicsEnvUnset.h
endif

ifndef BASE_3_16
SOURCES_3.14 += setMaxArrayBytes.c
SOURCES_3.14 += caFieldSize.c
DBDS_3.14    += setMaxArrayBytes.dbd
endif

SOURCES      += echo.c
DBDS_3.14    += echo.dbd

ifndef BASE_7_0
SOURCES_3.14 += dbli.c
DBDS_3.14    += dbli.dbd
endif

SOURCES_3.14 += dbla.c
DBDS_3.14    += dbla.dbd

#SOURCES_3.14 += termSig.c
#DBDS_3.14 += termSig.dbd

SOURCES_3.14 += threads.c
DBDS_3.14 += threads.dbd

SOURCES_3.14 += eval.c
DBDS_3.14    += eval.dbd

SOURCES      += afterInit.c
DBDS_3.14    += afterInit.dbd

SOURCES_vxWorks += bootNotify.c
