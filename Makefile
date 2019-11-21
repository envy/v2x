ASN1 := asn1c
ASN_FLAGS := -fcompound-names -fincludes-quoted -no-gen-example
ASN_DEST := asn1-src

CFLAGS := -Wall -I. -I$(ASN_DEST)
CXXFLAGS := -Wall -I. -I$(ASN_DEST)
LDFLAGS := -lpthread

# =======

ASN_FILES := asn1/TS102894-2v131-CDD.asn \
	asn1/EN302637-2v141-CAM.asn \
	asn1/EN302637-3v131-DENM.asn

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

libasn.a: asn $(ASN_O_FILES)
	$(AR) r $@ $(ASN_O_FILES)

app: $(APP_O_FILES) libasn.a
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(ASN_DEST)/*.o

