#include "contlive.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>



struct asocket_con* contlive_connect(const char *address, int port, int dbglev)
{
	struct sockaddr * skaddr;
	struct asocket_con* sk_con;
	if(!address)
		address = "127.0.0.1";

	if(!port)
		port = DEFAULT_PORT;

	skaddr = asocket_addr_create_in( address, port);

    sk_con = asocket_clt_connect(skaddr);

    free(skaddr);

    if(!sk_con){
        fprintf(stderr, "Socket connect failed: %s (%d)\n", strerror(errno), errno);
    }

	sk_con->dbglev = dbglev;

    return sk_con;
}

int contlive_disconnect(struct asocket_con* sk_con)
{
	return asocket_clt_disconnect(sk_con);
}




int contlive_get_updlst(struct asocket_con* sk_con, const char *flags, const char *ename)
{
	struct skcmd* tx_msg = asocket_cmd_create("ReadsUpdate");
    struct skcmd* rx_msg = NULL;

    if(!flags)
		flags = "show";
    
    if(!ename)
		ename = "*.*";
    
	asocket_cmd_param_add(tx_msg, flags);  
    asocket_cmd_param_add(tx_msg, ename);  

    asocket_con_trancive(sk_con, tx_msg, &rx_msg);

    asocket_cmd_delete(tx_msg);
    
    if(!rx_msg)
        return -EFAULT;

    asocket_cmd_delete(rx_msg);
    
    return 0;

}

int contlive_get_next(struct asocket_con* sk_con, char *ename,  char *value, char *hname, char *unit)
{
	struct skcmd* tx_msg = asocket_cmd_create("ReadGet");
    struct skcmd* rx_msg = NULL;

	const char *str = NULL;

	asocket_con_trancive(sk_con, tx_msg, &rx_msg);

	if(!rx_msg)
	    return -1;
	
	if(!rx_msg->param)
	    return -1;
	
	if(ename){
		str = asocket_cmd_param_get(rx_msg, 0);
		if(str)
			strcpy(ename, str);
		else 
			strcpy(hname, "");
	}

	if(hname){
		str = asocket_cmd_param_get(rx_msg, 1);	

		if(str)
			strcpy(hname, str);
		else 
			strcpy(hname, "");
		
	}


	if(value){
		str = asocket_cmd_param_get(rx_msg, 2);
		if(str)
			strcpy(value, str);
		else
			strcpy(value, "");
	}

	if(unit){
		str = asocket_cmd_param_get(rx_msg, 3);
		if(str)
			strcpy(unit, str);
		else 
			strcpy(unit, "");
	}

	asocket_cmd_delete(rx_msg);
	asocket_cmd_delete(tx_msg);
	 
    return 1;
}

int contlive_get_single(struct asocket_con* sk_con, const char *ename, char *value, char *hname, char *unit)
{
	int retval;

	retval = contlive_get_updlst(sk_con, "", ename);

	if(retval<0)
		return retval;

	return contlive_get_next(sk_con, NULL , value, hname, unit);

}



int contlive_set_single(struct asocket_con* sk_con, const char *ename , const char *value)
{
    struct skcmd* tx_msg = asocket_cmd_create("Write");
    struct skcmd* rx_msg = NULL;
    char tmpbuf[256];
	int retval = 0;
    
    asocket_cmd_param_add(tx_msg, ename);

    asocket_cmd_param_add(tx_msg, value);

    asocket_con_trancive(sk_con, tx_msg, &rx_msg);
    
    if(!rx_msg){
		printf("Error: no responce\n");
		retval = -1;
		goto out;
    }
	

	asocket_cmd_delete(rx_msg);    
  out:
    asocket_cmd_delete(tx_msg);
    
    return 0;
}
