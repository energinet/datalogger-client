cgibin_PROGRAMS = rpfchart.cgi csv_export.cgi

rpfchart_cgi_SOURCES = rpfchart.c soapC.c soapServer.c
#rpfchart_cgi_LDFLAGS = -g -lm -lgsoap -L../lib
rpfchart_cgi_LDFLAGS = -g -lm -lgsoap 

csv_export_cgi_SOURCES = csv_export.c
#csv_export_cgi_LDFLAGS = -g -lm -L../lib
csv_export_cgi_LDFLAGS = -g -lm 

#AM_CFLAGS = -Wall  -I../include
AM_CFLAGS = -Wall 
#AM_LDFLAGS = -L../lib


html_DATA = \
    rpfchart.wsdl \
    rpfchart_flash.wsdl \
    rpc_server_flash.wsdl

