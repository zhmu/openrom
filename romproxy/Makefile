CXXFLAGS=	-std=c++11 -I../lib
CXXFLAGS+=	-g
LDFLAGS=

OBJS=		romproxy.o proxy.o proxiedconnection.o loginproxy.o \
		romproxiedconnection.o gameproxy.o \
		../lib/lib.a

romproxy:	$(OBJS)
		$(CXX) $(CXXFLAGS) -o romproxy $(OBJS) $(LDFLAGS)

.PHONY:		liba

../lib/lib.a:	liba

liba:
		(cd ../lib && $(MAKE))

clean:
		rm -f romproxy $(OBJS)
		(cd ../lib && $(MAKE) clean)
