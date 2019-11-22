ASN1 := asn1c
ASN_FLAGS := -fcompound-names -fincludes-quoted -no-gen-example
ASN_DEST := asn1-src

CFLAGS := -std=c11 -Wall -I. -I$(ASN_DEST)
CXXFLAGS := -std=c++2a -Wall -I. -I$(ASN_DEST)
LDFLAGS := -lpthread

# =======

ASN_FILES := asn1/ITS-Container.asn \
	asn1/CAM.asn \
	asn1/DENM.asn \
	asn1/ETSI_TS_103301.asn \
	asn1/ISO_TS_14816.asn \
	asn1/ISO_TS_14906_Application.asn \
	asn1/ISO_TS_14906_Generic.asn \
	asn1/ISO_TS_17419.asn \
	asn1/ISO_TS_19091.asn \
	asn1/ISO_TS_19321.asn \
	asn1/ISO_TS_24534-3.asn


APP_CXX_FILES := main.cpp \
	parser.cpp \
	proxy.cpp \
	factory.cpp

ASN_C_FILES := $(wildcard asn1-src/*.c)
ASN_H_FILES := $(wildcard asn1-src/*.h)
ASN_O_FILES := $(ASN_C_FILES:.c=.o)
APP_O_FILES := $(APP_CXX_FILES:.cpp=.o)

all: app

asn: $(ASN_FILES)
	$(ASN1) $(ASN_FLAGS) -D $(ASN_DEST) $(ASN_FILES)
	$(eval ASN_C_FILES := $(wildcard asn1-src/*.c))
	$(eval ASN_H_FILES := $(wildcard asn1-src/*.h))

app: $(APP_O_FILES) $(ASN_O_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(ASN_DEST)/*.o
	rm -rf *.o
	rm -f app
