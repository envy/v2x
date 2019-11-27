ASN1 := asn1c
ASN_FLAGS := -fcompound-names -fincludes-quoted -findirect-choice -fno-include-deps -no-gen-example
ASN_DEST := asn1-src

CFLAGS := -std=c11 -Wall -g -I. -I$(ASN_DEST)
CXXFLAGS := -std=c++2a -Wall -g -I. -I$(ASN_DEST)
LDFLAGS := -lpthread

# =======

ASN_FILES := asn1/ITS_Container.asn \
	asn1/CAM.asn \
	asn1/DENM.asn \
	asn1/mod/DSRC.asn \
	asn1/mod/REGION.asn \
	asn1/mod/AddGrpC.asn \
	asn1/ElectronicRegistrationIdentificationVehicleDataModule.asn \
	asn1/MAPEM_PDU_Descriptions.asn \
	asn1/SPATEM_PDU_Descriptions.asn

ASN_ORIG_FILES := asn1/ITS_Container.asn \
	asn1/CAM.asn \
	asn1/DENM.asn \
	asn1/original/DSRC.asn \
	asn1/original/REGION.asn \
	asn1/original/AddGrpC.asn \
	asn1/ElectronicRegistrationIdentificationVehicleDataModule.asn \
	asn1/MAPEM_PDU_Descriptions.asn \
	asn1/SPATEM_PDU_Descriptions.asn



APP_CXX_FILES := main.cpp \
	parser.cpp \
	proxy.cpp \
	factory.cpp \
	MessageSink.cpp \
	Formatter.cpp

ASN_C_FILES := $(wildcard asn1-src/*.c)
ASN_H_FILES := $(wildcard asn1-src/*.h)
ASN_O_FILES := $(ASN_C_FILES:.c=.o)
APP_O_FILES := $(APP_CXX_FILES:.cpp=.o)
APP_H_FILES := $(wildcard *.h)

all: app

asn: $(ASN_FILES)
	rm -f asn1-src/*
	$(ASN1) $(ASN_FLAGS) -D $(ASN_DEST) $(ASN_FILES)
	$(eval ASN_C_FILES := $(wildcard asn1-src/*.c))
	$(eval ASN_H_FILES := $(wildcard asn1-src/*.h))

asn_orig: $(ASN_ORIG_FILES)
	rm -f asn1-src/*
	$(ASN1) $(ASN_FLAGS) -D $(ASN_DEST) $(ASN_ORIG_FILES)
	$(eval ASN_C_FILES := $(wildcard asn1-src/*.c))
	$(eval ASN_H_FILES := $(wildcard asn1-src/*.h))

app: $(APP_CXX_FILES) $(APP_H_FILES) $(ASN_C_FILES) $(ASN_H_FILES)
	mkdir -p build
	cd build && cmake .. && make
	rm -f app
	ln -s build/app .
