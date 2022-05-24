#ifndef OUR_CONNECTIONS
#define OUR_CONNECTIONS

#include <stdio.h>
#include <stdlib.h>

#define STR_ERROR "\033[1;31mERROR\033[93m:\033[0m"

void server_check_main_input(int argc, char ** argv) {

    if ( argc < 2 ) {

        printf(STR_ERROR " unspecified port number\n");
        printf("./test_microtcp_server <port-number>\n");
        exit(EXIT_FAILURE);
    }
}

void client_check_main_input(int argc, char ** argv) {

    if ( argc < 4 ) {

        printf(STR_ERROR " invalid command syntax\n");
        printf("./test_microtcp_client <server-ip> <port-number> <flag> <file_name>\n");
        printf("flag options:\n1 Transmit a big file\n2 Transmit in wrong order\n");
        exit(EXIT_FAILURE);
    }
}


#endif