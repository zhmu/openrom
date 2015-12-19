CXXFLAGS=	-std=c++11 -I/usr/include/libxml2 -I../lib
CXXFLAGS+=	-g
LDFLAGS=	-lxml2

OBJS=		romdump.o tcpflowparser.o types.o romstate.o flow.o \
		csvsysparser.o romlogparser.o ../lib/lib.a

romdump:	$(OBJS)
		$(CXX) $(CXXFLAGS) -o romdump $(OBJS) $(LDFLAGS)

../lib/lib.a:
		(cd ../lib && ${MAKE})

clean:
		rm -f romdump $(OBJS)
