/* Copyright 2011 Nick Mathewson, George Kadianakis
   See LICENSE for other credits and copying information
*/

#include "dummy.h"
#include "../protocol.h"
#include "../util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <event2/buffer.h>

static void usage(void);
static int parse_and_set_options(int n_options,
                                 const char *const *options,
                                 struct protocol_params_t *params);

/**
   This function populates 'params' according to 'options' and sets up
   the protocol vtable.

   'options' is an array like this:
   {"dummy","socks","127.0.0.1:6666"}
*/
static struct protocol_params_t *
dummy_init(int n_options, const char *const *options)
{
  struct protocol_params_t *params
    = calloc(1, sizeof(struct protocol_params_t));
  if (!params)
    return NULL;

  if (parse_and_set_options(n_options, options, params) < 0) {
    free(params);
    usage();
    return NULL;
  }

  return params;
}

/**
   Helper: Parses 'options' and fills 'params'.
*/
static int
parse_and_set_options(int n_options, const char *const *options,
                      struct protocol_params_t *params)
{
  const char* defport;

  if (n_options != 3)
    return -1;

  assert(!strcmp(options[0],"dummy"));

  if (!strcmp(options[1], "client")) {
    defport = "48988"; /* bf5c */
    params->mode = LSN_SIMPLE_CLIENT;
  } else if (!strcmp(options[1], "socks")) {
    defport = "23548"; /* 5bf5 */
    params->mode = LSN_SOCKS_CLIENT;
  } else if (!strcmp(options[1], "server")) {
    defport = "11253"; /* 2bf5 */
    params->mode = LSN_SIMPLE_SERVER;
  } else
    return -1;

  if (resolve_address_port(options[2], 1, 1,
                           &params->listen_address,
                           &params->listen_address_len, defport) < 0) {
    log_warn("addr");
    return -1;
  }

  params->vtable = &dummy_vtable;
  return 0;
}

/**
   Prints dummy protocol usage information.
*/
static void
usage(void)
{
  log_warn("Great... You can't even form a dummy protocol line:\n"
           "dummy syntax:\n"
           "\tdummy dummy_opts\n"
           "\t'dummy_opts':\n"
           "\t\tmode ~ server|client|socks\n"
           "\t\tlisten address ~ host:port\n"
           "Example:\n"
           "\tobfsproxy dummy socks 127.0.0.1:5000");
}

/*
  This is called everytime we get a connection for the dummy
  protocol.
*/

static struct protocol_t *
dummy_create(struct protocol_params_t *params)
{
  /* Dummy needs no per-connection protocol-specific state. */
  struct protocol_t *proto = calloc(1, sizeof(struct protocol_t));
  proto->vtable = &dummy_vtable;
  return proto;
}

static void
dummy_destroy(struct protocol_t *proto)
{
  free(proto);
}

/**
   Responsible for sending data according to the dummy protocol.

   The dummy protocol just puts the data of 'source' in 'dest'.
*/
static int
dummy_handshake(struct protocol_t *proto __attribute__((unused)),
                struct evbuffer *buf __attribute__((unused)))
{
  return 0;
}

static int
dummy_send(struct protocol_t *proto __attribute__((unused)),
           struct evbuffer *source, struct evbuffer *dest)
{
  return evbuffer_add_buffer(dest,source);
}

/*
  Responsible for receiving data according to the dummy protocol.

  The dummy protocol just puts the data of 'source' into 'dest'.
*/
static enum recv_ret
dummy_recv(struct protocol_t *proto __attribute__((unused)),
           struct evbuffer *source, struct evbuffer *dest)
{
  if (evbuffer_add_buffer(dest,source)<0)
    return RECV_BAD;
  else
    return RECV_GOOD;
}

DEFINE_PROTOCOL_VTABLE(dummy);
