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
 * 
 * MT-Unsafe
 */

#include "microtcp.h"
#include "../utils/crc32.h"
#include "../utils/log.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>


#define MICROTCP_HEADER_SIZE sizeof(microtcp_header_t)
#define MIN2(x, y) ( (x > y) ? y : x )
#define _ntoh_recvd_tcph(microtcp_header)  \
								{\
									microtcp_header.seq_number = ntohl(tcph.seq_number);\
									microtcp_header.ack_number = ntohl(tcph.ack_number);\
									microtcp_header.control    = ntohs(tcph.control);\
									microtcp_header.window     = ntohs(tcph.window);\
									microtcp_header.data_len   = ntohl(tcph.data_len);\
									microtcp_header.checksum   = ntohl(tcph.checksum);\
								}

#define TIOUT_DISABLE  0
#define TIOUT_ENABLE   1



/**
 * @brief Initializes the microTCP header for a packet to get send over the network. By giving FRAGMENT
 * in 'ctrlb', the packet (header) will be marked as fragmented. Putting CTRL_XXX in 'ctrlb' will not
 * set any control bits in the header
 * 
 * @param sock a valid microTCP socket handle
 * @param tcph microTCP header
 * @param ctrl control bits
 * @param paysz payload size
 * @param payld payload
 */
static void _preapre_send_tcph(microtcp_sock_t * __restrict__ sock, microtcp_header_t * __restrict__ tcph, uint16_t ctrlb,
						const void * __restrict__ payld, uint32_t paysz)
{

	#ifdef ENABLE_DEBUG_MSG
	if ( !sock ) {

		LOG_DEBUG("'sock' ---> NULL\n");
		check(-1);
	}

	if ( !tcph ) {

		LOG_DEBUG("'tcph' ---> NULL\n");
		check(-1);
	}
	#endif

	if ( (ctrlb == 3) ) {  // [SYN, FIN] together

		LOG_DEBUG("'ctrlb' ---> ");
		strctrl(ctrlb);
		check(-1);
	}

	tcph->seq_number = htonl(sock->seq_number);
	tcph->ack_number = htonl(sock->ack_number);
	tcph->control    = htons(ctrlb);
	tcph->window     = htons(MICROTCP_RECVBUF_LEN - sock->buf_fill_level);
	tcph->data_len   = htonl(paysz);
	tcph->checksum   = htonl( (paysz) ? crc32(payld, paysz) : 0U );
}

/**
 * @brief ENABLE or DISABLE the timeout socket option
 * @param sockfd A valid socket
 * @param too timeout-option (TIOUT_ENABLE, TIOUT_DISABLE)
 * @return int 
 */
static int _timeout(int sockfd, int too)
{

	struct timeval to;  // timeout

	to.tv_sec = 0L;

	if ( too == TIOUT_ENABLE )
		to.tv_usec = MICROTCP_ACK_TIMEOUT_US;
	else  // timeout disabled
		to.tv_usec = 0L;
	
	check( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) );

	return EXIT_SUCCESS;
}

static void _update_recv_buf(microtcp_sock_t *socket)
{
	
}

static void _cleanup();  /** TODO: add to at_exit() - free recvbuf() */

//////////////////////////////////////////////////////////////////////////////////////

/** TODO: [!] implement byte and packet statistics [!] */

microtcp_sock_t microtcp_socket(int domain, int type, int protocol)
{
	microtcp_sock_t sock;
	int sockfd;


	if ( type != SOCK_DGRAM )
		LOG_DEBUG("type of socket changed to 'SOCK_DGRAM'\n");

	bzero(&sock, sizeof(sock));
	sock.recvbuf = (uint8_t *) malloc(MICROTCP_RECVBUF_LEN);

	if ( !sock.recvbuf ) {

		sock.sd    = -1;
		sock.state = INVALID;
		sock.state = CLOSED;
		errno = ENOMEM;

		return sock;
	}

	check( sockfd = socket(domain, SOCK_DGRAM, protocol ));

	memset(&sock, 0, sizeof(sock));
	srand(time(NULL) + getpid());

	sock.sd         = sockfd;
	sock.seq_number = rand();
	sock.cwnd       = MICROTCP_INIT_CWND;
	sock.ssthresh   = MICROTCP_INIT_SSTHRESH;
	
	#ifdef ENABLE_DEBUG_MSG
	ackbase = sock.seq_number;
	#endif


	return sock;
}

int microtcp_bind(microtcp_sock_t * __restrict__ socket, const struct sockaddr * __restrict__ address,
               socklen_t address_len)
{
	check( bind(socket->sd, address, address_len) );
	return EXIT_SUCCESS;
}

int microtcp_connect(microtcp_sock_t * __restrict__ socket, const struct sockaddr * __restrict__ address,
                  socklen_t address_len)
{
	microtcp_header_t tcph;
	int64_t sockfd;


	if ( !socket ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	check( connect(socket->sd, address, address_len) );

	tcph.seq_number = htonl(socket->seq_number);
	tcph.window     = htons(MICROTCP_RECVBUF_LEN);
	tcph.control    = htons(CTRL_SYN);

	check( send(socket->sd, &tcph, sizeof(tcph), 0) );   // send SYN
	check( recv(socket->sd, &tcph, sizeof(tcph), 0) );   // recv SYNACK

	#ifdef ENABLE_DEBUG_MSG
	seqbase = ntohl(tcph.seq_number);  // necessary for print_tcp_header()
	print_tcp_header(socket, &tcph);
	#endif

	/** SYNACK **/
	if ( ntohs(tcph.control) != (CTRL_SYN | CTRL_ACK) ) {

		socket->state = INVALID;
		errno = ECONNABORTED;

		return -(EXIT_FAILURE);
	}

	++socket->seq_number;
	socket->ack_number = ntohl(tcph.seq_number) + 1U;
	socket->sendbuflen = ntohs(tcph.window);

	tcph.seq_number = htonl(socket->seq_number);
	tcph.ack_number = htonl(socket->ack_number);
	tcph.control    = htons(CTRL_ACK);

	check( send(socket->sd, &tcph, sizeof(tcph), 0) );  // send ACK
	socket->state     = SLOW_START;

	// _sock_enable_async(socket);

	LOG_DEBUG("INIT CCONTROL:s.state: %d, s.cwnd: %ld, s.ssthres: %ld\n",socket->state,socket->cwnd,socket->ssthresh);
	return EXIT_SUCCESS;
}

int microtcp_accept(microtcp_sock_t * __restrict__ socket, struct sockaddr * __restrict__ address,
                 socklen_t address_len)
{
	microtcp_header_t tcph;


	if ( socket->state != INVALID )
		return -(EXIT_FAILURE);

	socket->state   = LISTEN;

	check( recvfrom(socket->sd, &tcph, sizeof(tcph), 0, address, &address_len) );
	check( connect(socket->sd, address, address_len) );

	#ifdef ENABLE_DEBUG_MSG
	seqbase = ntohl(tcph.seq_number);  // necessary for print_tcp_header()
	print_tcp_header(socket, &tcph);
	#endif

	if ( ntohs(tcph.control) != CTRL_SYN ) {

		socket->state = INVALID;
		errno = ECONNABORTED;

		return -(EXIT_FAILURE);
	}

	socket->sendbuflen = ntohs(tcph.window);
	socket->ack_number = ntohl(tcph.seq_number) + 1U;

	++socket->packets_received;
	++socket->bytes_received;

	tcph.seq_number = htonl(socket->seq_number);
	tcph.ack_number = htonl(socket->ack_number);
	tcph.control    = htons(CTRL_ACK | CTRL_SYN);
	tcph.window     = htons(MICROTCP_RECVBUF_LEN);

	check(send(socket->sd, &tcph, sizeof(tcph), 0));
	check(recv(socket->sd, &tcph, sizeof(tcph), 0));

	print_tcp_header(socket, &tcph);

	if ( ( ntohs(tcph.control) ) != CTRL_ACK ) {

		socket->state = INVALID;
		errno = ECONNABORTED;

		return -(EXIT_FAILURE);
	}

	++socket->seq_number;         // ghost-byte
	socket->state = ESTABLISHED;

	// _sock_enable_async(socket);


	return EXIT_SUCCESS;
}

int microtcp_shutdown(microtcp_sock_t * socket, int how)
{
	// LOG_DEBUG("Start of SD");
	// LOG_DEBUG("how=%d",how);
	microtcp_header_t fin_ack, ack;

	if(how==SHUTDOWN_CLIENT){//sender is shutting down the connection

		fin_ack.seq_number = htonl(socket->seq_number);
		fin_ack.ack_number = htonl(socket->ack_number);
		fin_ack.control    = htons(CTRL_FIN | CTRL_ACK);

		/* Send FIN/ACK */
		check(send(socket->sd, (void*)&fin_ack, sizeof(fin_ack), 0));
		/* Receive ACK for previous FINACK */
		check(recv(socket->sd, (void*)&ack, sizeof(ack), 0));

		uint16_t recieved_ack = ntohs(ack.control);

		/* Check if the received package is an ACK (and the correct ACK)*/
		if(!(recieved_ack & CTRL_ACK)){
			return -(EXIT_FAILURE);
		}

		socket->state = CLOSING_BY_HOST;

		/* Wait for FIN ACK from the server*/
		check(recv(socket->sd, (void*)&fin_ack, sizeof(ack), 0))

		uint16_t recieved_finack = ntohs(fin_ack.control);

		if(!(recieved_finack & (CTRL_FIN | CTRL_ACK))){
			return -(EXIT_FAILURE);
		}

		/* Prepare and send back the ACK header*/
		ack.control    = htons(CTRL_ACK);
		ack.ack_number = htonl(socket->ack_number);
		ack.seq_number = htonl(socket->seq_number);

		check(send(socket->sd, (void*)&ack, sizeof(ack), 0));
		
		/** TODO: Timed wait for server FIN ACK retransmition */
		socket->state = CLOSED;
		return EXIT_SUCCESS;

	}else if(how==SHUTDOWN_SERVER){//reciever recieved a FIN packet

		socket->state=CLOSING_BY_PEER;
		// LOG_DEBUG("SD: state:cbp");
		ack.control    = htons(CTRL_ACK);
		ack.ack_number = htonl(socket->ack_number);
		check(send(socket->sd, (void*)&ack, sizeof(ack), 0));
		
		// LOG_DEBUG("SD: sent ACK\n");
		fin_ack.seq_number = htonl(socket->seq_number);
		fin_ack.ack_number = htonl(socket->ack_number);
		fin_ack.control    = htons(CTRL_FIN | CTRL_ACK);
		
		/* Send FIN/ACK */
		check(send(socket->sd, (void*)&fin_ack, sizeof(fin_ack), 0));
		// LOG_DEBUG("SD: Sent FINACK\n");
		// LOG_DEBUG("SD: Waiting for ACK\n");
		/* Receive ACK for previous FINACK */
		check(recv(socket->sd, (void*)&ack, sizeof(ack), 0));

		uint16_t recieved_ack = ntohs(ack.control);

		/* Check if the received package is an ACK */
		if((recieved_ack & CTRL_ACK)) {
			return -(EXIT_FAILURE);
		}
		// LOG_DEBUG("SD: recieved ACK");

		/* Terminate the connection */
		socket->state = CLOSED;
		// LOG_DEBUG("SD: state:closed");
		return EXIT_SUCCESS;

	}else{
		errno = EINVAL;
		return -(EXIT_FAILURE);
	}
}

ssize_t microtcp_send(microtcp_sock_t * __restrict__ socket, const void * __restrict__ buffer, size_t length,
               int flags)
{
	/** TODO: Congestion control --> update() ssthresh after each send() */

	uint8_t tbuff[MICROTCP_HEADER_SIZE + MICROTCP_MSS];  // c99 and onwards --- problem for larger MSS
	microtcp_header_t tcph;
	int64_t ret;

	int sockfd;
	int fflag;  // fragments flag
	size_t lengthcpy = length;

	uint64_t bytes_to_send;
	uint64_t chunks;
	uint64_t index;
	uint64_t dacks;
	uint64_t tmp;


	if ( !socket ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	if ( (socket->state == INVALID) || (socket->state >= CLOSING_BY_PEER) ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	sockfd = socket->sd;
	fflag  = 0;

sflag0:
	if ( length > MIN2(MICROTCP_MSS, MIN2(socket->cwnd, socket->sendbuflen)) )
		fflag = 0;
	else
		fflag = 1;

	_timeout(sockfd, TIOUT_ENABLE);

	while ( length ) {
send1:
		tmp = MIN2(socket->cwnd, socket->sendbuflen);
		bytes_to_send = MIN2(length, tmp);
		chunks = bytes_to_send / MICROTCP_MSS;  // avoid IP-Fragmentation (break into fragments)

		LOG_DEBUG("\n\e[1mlength = %lu\e[0m\n"
					" > chunks = %lu\n"
					" > bytes_to_send = %lu\n", length, chunks, bytes_to_send);

		for ( index = 0UL; index < chunks; ++index ) {

			tmp = (uint64_t)(buffer) + (index * MICROTCP_MSS);  // pointer arithmetic - c99 and onwards

			_preapre_send_tcph(socket, &tcph, ( !fflag ) ? (fflag = FRAGMENT) : CTRL_XXX, (void *)(tmp), MICROTCP_MSS);
			memcpy(tbuff, &tcph, MICROTCP_HEADER_SIZE);
			memcpy(tbuff + MICROTCP_HEADER_SIZE, (void *)(tmp), MICROTCP_MSS);

			check( send(sockfd, tbuff, MICROTCP_MSS + MICROTCP_HEADER_SIZE, 0) );
		}

		length -= bytes_to_send;
		bytes_to_send -= index * MICROTCP_MSS;

		// semi-filled chunk
		if ( bytes_to_send ) {

			tmp = (uint64_t)(buffer) + (index * MICROTCP_MSS);  // pointer arithmetic

			_preapre_send_tcph(socket, &tcph, ( !length && chunks) ? FRAGMENT : CTRL_XXX, (void *)(tmp), bytes_to_send);
			memcpy(tbuff, &tcph, MICROTCP_HEADER_SIZE);
			memcpy(tbuff + MICROTCP_HEADER_SIZE, (void *)(tmp), bytes_to_send);
			++chunks;

			check( send(sockfd, tbuff, bytes_to_send + MICROTCP_HEADER_SIZE, 0) );
		}

		for ( dacks = 0UL, index = 0UL; index < chunks; ++index ) {	

sflag1:
			ret = recv(sockfd, &tcph, MICROTCP_HEADER_SIZE, 0);

			LOG_DEBUG("s.state: %d, s.cwnd: %ld, s.ssthres: %ld\n",socket->state,socket->cwnd,socket->ssthresh);

			if ( ret < 0 ) {

				if ( errno == EAGAIN ) {

					socket->ssthresh  = socket->cwnd / 2; 
					socket->cwnd      = MICROTCP_MSS;
					socket->state     = SLOW_START;

					LOG_DEBUG("timeout-occured, retransmiting packet\n");

					length = lengthcpy;
					goto send1;
				}
				else
					check( ret );
			}
			else {

				print_tcp_header(socket, &tcph);
				_ntoh_recvd_tcph(tcph);

				if ( tcph.ack_number < socket->seq_number ) {  // retransmit

					LOG_DEBUG("!ack < seq!");
					
					/** TODO: Fast Retransmit */

					if ( ++dacks == 3UL ) {

						socket->ssthresh   = socket->cwnd / 2;
						socket->cwnd       = socket->ssthresh + 3 * MICROTCP_MSS;
						socket->seq_number = tcph.ack_number;

						// tmp = (index != chunks - 1UL) ? MICROTCP_MSS : bytes_to_send;
						// buffer += (index - 1UL) * tmp;
						length = lengthcpy;
					}
					else if ( dacks > 3UL )
						socket->cwnd = socket->cwnd + MICROTCP_MSS;

					goto send1;
				}
				else {  // everything is normal

					socket->seq_number += (index != chunks - 1UL) ? MICROTCP_MSS : bytes_to_send;
					dacks = 0UL;

					if ( socket->state == SLOW_START ) {

						socket->cwnd = socket->cwnd * 2;  // in SLOW_START increment cwnd exponentially

						if ( socket->cwnd >= socket->ssthresh )  // if SLOW_START & cwnd>=ssthresh -> CONG_AVOID
							socket->state = CONG_AVOID;
					}
					else
						socket->cwnd += MICROTCP_MSS;  // in CONG_AVOID increment cwnd additively
				}
			}
		}
	}

	_timeout(sockfd, TIOUT_DISABLE);


	return EXIT_SUCCESS;
}

ssize_t microtcp_recv(microtcp_sock_t * __restrict__ socket, void * __restrict__ buffer, size_t length, int flags)
{
	uint8_t tbuff[MICROTCP_MSS + MICROTCP_HEADER_SIZE];
	microtcp_header_t tcph;

	int64_t total_bytes_read;
	int64_t bytes_read;

	int sockfd;
	int frag;


	if ( !socket ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	if ( (socket->state == INVALID) || (socket->state >= CLOSING_BY_PEER) ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}


	total_bytes_read = 0L;
	sockfd = socket->sd;

rflag0:
	check( total_bytes_read = recv(sockfd, tbuff, length, 0) );
	memcpy(&tcph, tbuff, MICROTCP_HEADER_SIZE);
	print_tcp_header(socket,&tcph);

	_ntoh_recvd_tcph(tcph);

	// Fast Retransmit
	if ( tcph.seq_number > socket->ack_number ) {

		LOG_DEBUG("Reordering\n");  // packet that was read is actually discarded!
		_preapre_send_tcph(socket, &tcph, CTRL_ACK, NULL, 0U);
		check( send(sockfd, &tcph, MICROTCP_HEADER_SIZE, 0) );

		/** TODO: Packet reordeing could also be performed here,
		 * thus achieving better performance.
		 */

		goto rflag0;
	}
	else if ( tcph.control & CTRL_FIN ) {  // termination

		microtcp_shutdown(socket, SHUTDOWN_SERVER);
		return -1L;
	}
	else if ( tcph.seq_number < socket->ack_number )  // skip duplicate packets (during TIMEOUT)
		goto rflag0;

	if ( !tcph.data_len )  // zero length packet
		return 0L;

	memcpy(buffer, tbuff + MICROTCP_HEADER_SIZE, tcph.data_len);

	total_bytes_read -= MICROTCP_HEADER_SIZE;
	socket->ack_number += tcph.data_len;
	tcph.ack_number = socket->ack_number;
	tcph.seq_number = socket->seq_number;
	frag = tcph.control & FRAGMENT;

	_preapre_send_tcph(socket, &tcph, CTRL_ACK, NULL, 0U);
	check( send(sockfd, &tcph, MICROTCP_HEADER_SIZE, 0) );

	if ( !frag )  // no fragmentation case
		return total_bytes_read;

	// fragmentation case
	tbuff[total_bytes_read - 1L];
	bytes_read = total_bytes_read;

	do {

		tbuff[bytes_read - 1L] = 0;

		check( bytes_read = recv(sockfd, tbuff, MICROTCP_MSS + MICROTCP_HEADER_SIZE, 0) );
		memcpy(&tcph, tbuff, MICROTCP_HEADER_SIZE);
		_ntoh_recvd_tcph(tcph);
		memcpy(buffer + total_bytes_read, tbuff + MICROTCP_HEADER_SIZE, tcph.data_len);

		total_bytes_read += bytes_read - MICROTCP_HEADER_SIZE;
		socket->ack_number += tcph.data_len;
		tcph.seq_number = socket->seq_number;
		tcph.ack_number = socket->ack_number;
		frag = tcph.control & FRAGMENT;

		_preapre_send_tcph(socket, &tcph, CTRL_ACK, NULL, 0U);
		check( send(sockfd, &tcph, MICROTCP_HEADER_SIZE, 0) );

	} while ( !frag );


	return total_bytes_read;
}

