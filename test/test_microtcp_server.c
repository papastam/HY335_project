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
 * You can use this file to write a test microTCP server.
 * This file is already inserted at the build system.
 */


#include "../lib/microtcp.h"
#include "../utils/log.h"
#include "connections.h"

#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void server_check_main_input(int argc, char ** argv);

int main(int argc, char ** argv) {

    int sockopt;

    struct sockaddr_in addr;
    struct timeval tv;

    int64_t  ret;
    uint16_t port;
    uint8_t  buff[1500];

    microtcp_sock_t ssock;
    microtcp_header_t tcph;

    int flag = 0;
    server_check_main_input(argc, argv);
    if(argc == 3) {
        if(strcmp(argv[2], "timeout") != 0) {
            printf("Invalid argument. If you want the server to use timeout use the flag \"timeout\"");
            exit(EXIT_FAILURE);
        }
        flag = 1;
    }
    printf("initializing server...\n");

    ssock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);

    if ( ssock.sd > 0 )
        printf("socket created\n");
    else
        exit(EXIT_FAILURE);

    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_family      = AF_INET;

    check( microtcp_bind(&ssock, (struct sockaddr *)(&addr), sizeof(addr)) );
    check( microtcp_accept(&ssock, (struct sockaddr *)(&addr), sizeof(addr)) );
    memset(buff, 0, 1500UL);


    check( ret = microtcp_recv(&ssock, buff, 3000UL, 0) );
    printf("ret = %ld\n", ret);
    LOG_DEBUG("recv()ed payload [%ld] ---> %s\n", ret, buff);
    memset(buff, 0, ret);

    usleep(220000U);
    check( ret = microtcp_recv(&ssock, buff, 1500UL, 0) );
    printf("ret = %ld\n", ret);
    LOG_DEBUG("recv()ed payload [%ld] ---> %s\n", ret, buff);
    memset(buff, 0, ret);

    // [FIN, ACK]
    check( ret = microtcp_recv(&ssock, buff, 1500UL, 0) );
    printf("ret = %ld\n", ret);
    LOG_DEBUG("recv()ed payload [%ld] ---> %s\n", ret, buff);
    memset(buff, 0, ret);

    printf("Connection with host successfully closed!\n");
    return 0;
}