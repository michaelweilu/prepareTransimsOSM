####### Output directory

OBJECTS_DIR = ./

####### Files

TARGET		= $(notdir $(shell pwd))

USE_DEBUGTOOLS		=	1
USE_NETWORK			=	1
USE_GISSYSTEM		=	1
USE_GDAL			=	1
USE_PROJ			=	1
#USE_SPATIALINDEX	=	1
#USE_MP		= 1

SOURCES =				\
	main.cpp			\
	PrepareTransims.cpp	\

OBJECTS = ${SOURCES:.cpp=.o}

#CXXFLAGS += -DDEBUG_GLOBAL
CXXFLAGS += -DFUNCTION_TRACE

include ~/gis/src/makefiles/common.mk
include ~/gis/src/makefiles/Makefile.mk

