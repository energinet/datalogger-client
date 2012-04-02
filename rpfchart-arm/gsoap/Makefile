cfiles = soapC.c soapH.h soapServer.c soapServerLib.c soapStub.h rpfchart.wsdl rpfchart.nsmap rpfchart_flash.wsdl rpc_server_flash.wsdl

all: rpfchart.wsdl rpfchart_flash.wsdl rpc_server_flash.wsdl copy_file

install: all


rpfchart.wsdl: ../src/rpfchart.h
	soapcpp2 -c  ../src/rpfchart.h


PHONY: rpfchart_flash.wsdl
rpfchart_flash.wsdl:  rpfchart.wsdl
	sed 's/schemaLocation="\([a-zA-Z0-9:/.-]*\)"//g' rpfchart.wsdl > rpfchart_flash.wsdl

PHONY: rpc_server_flash.wsdl
rpc_server_flash.wsdl: rpfchart_flash.wsdl
	cp rpfchart_flash.wsdl rpc_server_flash.wsdl

copy_file:
	$(foreach var,$(cfiles), cp $(var) ../src;)

clean:
	-rm ns.xsd
	-rm rpfchart*
	-rm soap*
	-cd ../src ; /bin/rm $(cfiles)

