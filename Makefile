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

ASN_C_FILES := $(wildcard asn1-src/*.c)
ASN_H_FILES := $(wildcard asn1-src/*.h)
ASN_O_FILES := $(ASN_C_FILES:.c=.o)

APP_H_FILES := $(wildcard *.h)
APP_CXX_FILES := $(wildcard *.cpp)
APP_O_FILES := $(APP_CXX_FILES:.cpp=.o)

all: app

asn_mod: $(ASN_FILES)
	rm -f asn1-src/*
	$(ASN1) $(ASN_FLAGS) -D $(ASN_DEST) $(ASN_FILES)
	$(eval ASN_C_FILES := $(wildcard asn1-src/*.c))
	$(eval ASN_H_FILES := $(wildcard asn1-src/*.h))
	cd asn1-src && find . -type f \( -iname "*.h" -not -iname "asn_headers.h" \) | cut -c3- | awk '{ print "#include \"" $$1 "\"" }' > asn_headers.h

asn: $(ASN_ORIG_FILES)
	rm -f asn1-src/*
	$(ASN1) $(ASN_FLAGS) -D $(ASN_DEST) $(ASN_ORIG_FILES)
	$(eval ASN_C_FILES := $(wildcard asn1-src/*.c))
	$(eval ASN_H_FILES := $(wildcard asn1-src/*.h))
	cd asn1-src && find . -type f \( -iname "*.h" -not -iname "asn_headers.h" \) | cut -c3- | awk '{ print "#include \"" $$1 "\"" }' > asn_headers.h

app: $(APP_CXX_FILES) $(APP_H_FILES) $(ASN_C_FILES) $(ASN_H_FILES)
	mkdir -p build
	cd build && cmake .. && make
	rm -f app
	ln -s build/app .
