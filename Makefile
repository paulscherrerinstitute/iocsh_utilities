include /ioc/tools/driver.makefile

HEADERS += require.h
BUILDCLASSES += Linux

SOURCES += require.c
SOURCES += listRecords.c
SOURCES += updateMenuConvert.c
SOURCES_vxWorks += bootNotify.c
SOURCES_vxWorks += iocCheck.c
