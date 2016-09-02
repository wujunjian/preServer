PSRC = BaseLog.cpp Config.cpp CRC.cpp common.cpp DbPool.cpp SigMask.cpp main.cpp preServer.cpp BroadCast.cpp DbHelper.cpp Utils.cpp RSAHelper.cpp Redis.cpp
BSRC = UdpRecv.cpp BroadCast.cpp BaseLog.cpp Redis.cpp DbHelper.cpp DbPool.cpp Config.cpp

POBJ := $(patsubst %.cpp,%.o,$(PSRC))
BOBJ:= $(patsubst %.cpp,%.o,$(BSRC))

INCS = -I/usr/include/oracle/11.2/client64  -I/usr/local/include/hiredis 

SHLIBS = /usr/local/lib/libcastle.so.1.0.0 -L/usr/local/lib -L/home/oracle/app/oracle/product/11.2.0/client_1/lib -L/usr/lib/oracle/11.2/client64/lib 

Target = preServer UdpRecv

all: $(Target)
preServer: $(POBJ)
	g++ -g -rdynamic $(INCS) $^  $(SHLIBS) -o $@ -locci -lociei -lssl -lpthread -lhiredis

UdpRecv: $(BOBJ)
	g++ -g -rdynamic $(INCS) $^  $(SHLIBS) -o $@ -locci -lociei -lssl -lpthread -lhiredis

%.o : %.cpp
	g++ -g -rdynamic $(INCS) -c $< -o $@
clean:
	rm -f $(Target) *.o 
