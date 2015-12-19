CXXFLAGS=	-std=c++11 -I/usr/include/libxml2

OBJS=		rompack.o protocoldefinition.o protocoldecode.o \
		protocoldisplay.o protocolcodegenerator.o \
		loggingsystem.o logger.o buffer.o \
		address.o socket.o client.o server.o \
		romconnection.o rompacketlogger.o

lib.a:		$(OBJS)
		$(AR) src lib.a $(OBJS)

clean:
		rm -f lib.a $(OBJS)
