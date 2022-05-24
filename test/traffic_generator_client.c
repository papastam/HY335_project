/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2017  Manolis Surligas <surligas@gmail.com>
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#include "../lib/microtcp.h"
#include "../utils/log.h"

static char running = 1;

static void
sig_handler(int signal)
{
  if(signal == SIGINT) {
    LOG_INFO("Stopping traffic generator client...");
    running = 0;
  }
}

int
main(int argc, char **argv) {
  /*
   * Register a signal handler so we can terminate the client with
   * Ctrl+C
   */
  signal(SIGINT, sig_handler);
  check_args(argc, argv);
  microtcp_sock_t socket;
  struct sockaddr_in addr;
  uint16_t port=atoi(argv[2]);
  uint32_t iaddr;
  uint8_t  buff[1500];


  socket = microtcp_socket(AF_INET, SOCK_DGRAM, 0);
  inet_pton(AF_INET, argv[1], &iaddr);
  addr.sin_addr.s_addr = iaddr;
  addr.sin_port        = htons(port);
  addr.sin_family      = AF_INET;

  microtcp_connect(&socket, (struct sockaddr*)&addr, sizeof(addr));


  LOG_INFO("Start receiving traffic from port %u", port);
  /*TODO: Connect using microtcp_connect() */
  while(running) {
    /* TODO: Measure time */
    time_t now = time(0);
    /* TODO: Receive using microtcp_recv()*/
    microtcp_recv(&socket, buff, 1500UL, 0);
    /* TODO: Measure time */
    time_t later = time(0);
    double elapsed_time = difftime(later, now);
    /* TODO: Do other stuff... */
  }

  /* Ctrl+C pressed! Store properly time measurements for plotting */
}

check_args(int argc, char** argv) {
  if(argc < 4 || argc >=5) {
    printf("Invalid Syntax: ./traffic_generator_client <server-ip> <port>");
    exit(0);
  }
}
