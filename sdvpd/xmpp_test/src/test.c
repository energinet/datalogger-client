#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sdvp.h>
#include <stdbool.h>

#include "define.h"

void reverse (char* s) {
    int length = strlen(s);
    int c, i, j;

    for (i = 0, j = length - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
/**
 * The Callback routine called from the SDVP lib when a RCP command is received
 */
sdvp_reply_params_t* _SdvpCallback (sdvp_method_t methodType, sdvp_from_t* from,
        sdvp_params_t* params) {
    sdvp_reply_params_t* replyParams = NULL;

    printf("Got call from %s: %s (%s)\n", from->name, from->jid, from->group);

    switch (methodType) {
    case SDVP_METHOD_UNDEFINED:
        printf("Undefined method called!\n");
        break;
    case SDVP_METHOD_61850_READ:
        sdvp_InitiateReplyParameters(&replyParams, 2);
        replyParams->params[0].value = strdup("Reply param 1\n");
        replyParams->params[1].value = strdup("Reply param 2\n");
        break;
    case SDVP_METHOD_61850_WRITE:
        if (params->numberOfParams == 2) {
            printf("Wants to write '%s' to '%s'\n", params->params[1].value,
                    params->params[0].value);
        }
        break;
    case SDVP_METHOD_DIRECT_READ:
        // ContdaemClient_Get(NULL, param0, value_str, sizeof(value_str));
        if (params->numberOfParams == 1) {
            sdvp_InitiateReplyParameters(&replyParams, 1);
            replyParams->params[0].value = strdup(params->params[0].value);
            reverse(replyParams->params[0].value);
        }
        break;
    case SDVP_METHOD_DIRECT_WRITE:
        if (params->numberOfParams == 2) {
            // ContdaemClient_Set(NULL, params->params[0].value,
            //      params->params[1].value, value_str, sizeof(value_str));
            printf("Wants to write '%s' to '%s'\n", params->params[1].value,
                    params->params[0].value);
        }
        break;
    }

    return replyParams;
}

/**
 * Create the configuration for SDVP control from the
 * @param[in] jid The XMPP jid to connect to the SDVP hub with
 * @param[in] pass The password to login with
 * @param[in] host The host to connect to
 * @return A SDVP configuration set according to the input
 */
sdvp_config_t* _CreateSdvpConfig (char* jid, char* pass, char* host) {
    sdvp_config_t* sdvpConfig;
    sdvpConfig = sdvp_ConfigNew();
    sdvp_ConfigSetUsername(sdvpConfig, jid);
    sdvp_ConfigSetPassword(sdvpConfig, pass);
    sdvp_ConfigSetHost(sdvpConfig, host);
    sdvp_ConfigSetCallback(sdvpConfig, _SdvpCallback);
    sdvp_ConfigSetDebug(sdvpConfig, SDVP_DEBUG_ENABLED);
    return sdvpConfig;
}

int main (int argc, char **argv) {
    sdvp_config_t* sdvpConfig;
    if (argc < 4) {
        fprintf(stderr, "Usage: <jid> <pass> <host>\n\n");
        return 1;
    }
    sdvpConfig = _CreateSdvpConfig(argv[1], argv[2], argv[3]);

    //Note: This connects and blocks!
    sdvp_ConnectionCreate(sdvpConfig);
    sdvp_Shutdown();

    return 0;
}
