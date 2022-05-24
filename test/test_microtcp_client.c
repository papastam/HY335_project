/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * You can use this file to write a test microTCP client.
 * This file is already inserted at the build system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "connections.h"
#include "../lib/microtcp.h"
#include "../utils/log.h"

#define TEST_BYTES 2805

void send_file(FILE *fp, microtcp_sock_t sockfp);


int main(int argc, char **argv) {
    microtcp_sock_t csock;
    struct sockaddr_in addr;
    uint16_t port=atoi(argv[2]);
    uint32_t iaddr;
    char buff[32];
    void *frag_test;


    client_check_main_input(argc, argv);
    int flag = atoi(argv[3]);
    csock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);

    inet_pton(AF_INET, argv[1], &iaddr);
    addr.sin_addr.s_addr = iaddr;
    addr.sin_port        = htons(port);
    addr.sin_family      = AF_INET;

    microtcp_connect(&csock,(struct sockaddr*)&addr,sizeof(addr));
    FILE* fp;
    switch(flag) {
        case 1 :
            fp = fopen(argv[4], "r");
        
            // if ( fd  < 0 ) {

            //     perror("open() failed");
            //     exit(EXIT_FAILURE);
            // }

            // if ( !(frag_test = malloc(TEST_BYTES + 1)) ) {

            //     perror("malloc() failed");
            //     exit(EXIT_FAILURE);
            // }

            // read(fd, frag_test, TEST_BYTES);
            // *(char *)(frag_test + TEST_BYTES) = 0;
        
            send_file(fp, csock);

            break;
        case 2 :

            break;
        case 3 :

            break;
        default :

            break;
    }

    LOG_DEBUG("Shutting down the connection.\n");
    microtcp_shutdown(&csock,SHUTDOWN_CLIENT);

    LOG_DEBUG("Connection has been shut down successfully!\n");

    return 0;
}


void send_file(FILE *fp, microtcp_sock_t sockfp) {
    
    struct stat finfo;
    fstat(fileno(fp), &finfo);  
    
    char data[finfo.st_size];
    
    fread(data, finfo.st_size, 1, fp);
    microtcp_send(&sockfp, data, finfo.st_size, 0);
    
    bzero(data, finfo.st_size);

    data[0] = '6';
    data[1] = '9';
    data[2] = 0;

    microtcp_send(&sockfp, data, 3UL, 0);
}