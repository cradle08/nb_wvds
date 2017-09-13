CONTIKI = ../../../contiki

APPS += at-cmd wvds

PROJECT_SOURCEFILES += app.c VehicleDetection.c

CFLAGS += -I..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

all: device

include $(CONTIKI)/Makefile.include
