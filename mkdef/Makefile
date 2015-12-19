CXXFLAGS=	-std=c++11 -I/usr/include/libxml2 -I../lib
CXXFLAGS+=	-g
LDFLAGS=	-lxml2 -g

OBJS=		mkdef.o ../lib/lib.a

mkdef:		$(OBJS)
		$(CXX) $(CXXFLAGS) -o mkdef $(OBJS) $(LDFLAGS)

.PHONY:		liba

../lib/lib.a:	liba

liba:
		(cd ../lib && $(MAKE))

clean:
		rm -f mkdef $(OBJS)
		(cd ../lib && $(MAKE) clean)
