/*
Copyright (c) 2017 Darren Smith

ssl_examples is free software; you can redistribute it and/or modify
it under the terms of the MIT license. See LICENSE for details.
*/

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void handle_error(const char *file, int lineno, const char *msg);
void die(const char *msg); 
void print_unencrypted_data(char *buf, size_t len); 
/* An instance of this object is created each time a client connection is
 * accepted. It stores the client file descriptor, the SSL objects, and data
 * which is waiting to be either written to socket or encrypted. */
struct ssl_client
{
  int fd;

  SSL *ssl;

  BIO *rbio; /* SSL reads from, we write to. */
  BIO *wbio; /* SSL writes to, we read from. */

  /* Bytes waiting to be written to socket. This is data that has been generated
   * by the SSL object, either due to encryption of user input, or, writes
   * requires due to peer-requested SSL renegotiation. */
  char* write_buf;
  size_t write_len;

  /* Bytes waiting to be encrypted by the SSL object. */
  char* encrypt_buf;
  size_t encrypt_len;

  /* Method to invoke when unencrypted bytes are available. */
  void (*io_on_read)(char *bufin, size_t lenin,char *bufout, size_t &lenout);
};

/* This enum contols whether the SSL connection needs to initiate the SSL
 * handshake. */
enum ssl_mode { SSLMODE_SERVER, SSLMODE_CLIENT };

void ssl_client_init(struct ssl_client *p,
                     int fd,
                     enum ssl_mode mode);

void ssl_client_cleanup(struct ssl_client *p);
int ssl_client_want_write(struct ssl_client *cp); 

/* Obtain the return value of an SSL operation and convert into a simplified
 * error code, which is easier to examine for failure. */
enum sslstatus { SSLSTATUS_OK, SSLSTATUS_WANT_IO, SSLSTATUS_FAIL};

static enum sslstatus get_sslstatus(SSL* ssl, int n);

/* Handle request to send unencrypted data to the SSL.  All we do here is just
 * queue the data into the encrypt_buf for later processing by the SSL
 * object. */
void send_unencrypted_bytes(const char *buf, size_t len, struct ssl_client &client);

/* Queue encrypted bytes. Should only be used when the SSL object has requested a
 * write operation. */
void queue_encrypted_bytes(const char *buf, size_t len, struct ssl_client &client);

enum sslstatus do_ssl_handshake(struct ssl_client &client);

/* Process SSL bytes received from the peer. The data needs to be fed into the
   SSL object to be unencrypted.  On success, returns 0, on SSL error -1. */
int on_read_cb(char* src, size_t len,char *bufout,size_t &lenout,struct ssl_client &client);

/* Process outbound unencrypted data that is waiting to be encrypted.  The
 * waiting data resides in encrypt_buf.  It needs to be passed into the SSL
 * object for encryption, which in turn generates the encrypted bytes that then
 * will be queued for later socket write. */
int do_encrypt(struct ssl_client &client);

/* Read bytes from stdin and queue for later encryption. */
void do_stdin_read(struct ssl_client &client);

/* Read encrypted bytes from socket. */
int do_sock_read(struct ssl_client &client);

/* Write encrypted bytes to the socket. */
int do_sock_write(struct ssl_client &client);

void ssl_init(const char * certfile, const char* keyfile);

