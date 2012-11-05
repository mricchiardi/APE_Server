/*
  Copyright (C) 2006, 2007, 2008, 2009, 2010  Anthony Catel <a.catel@weelya.com>

  This file is part of APE Server.
  APE is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  APE is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with APE ; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

/* servers.c */

#include "servers.h"
#include "sock.h"
#include "utils.h"
#include "config.h"
#include "http.h"
#include "handle_http.h"
#include "transports.h"
#include "parser.h"
#include "main.h"

static void ape_read(ape_socket *co, ape_buffer *buffer, size_t offset, acetables *g_ape)
{
	co->parser.parser_func(co, g_ape);
}

static void ape_sent(ape_socket *co, acetables *g_ape)
{
	subuser *sub = (subuser *)(co->attach);
	if ((sub != NULL) && (sub->burn_after_writing)) {
		if (sub->user != NULL) {
			transport_data_completly_sent(sub, sub->user->transport, g_ape);
		}
		sub->burn_after_writing = 0;
	}
}

static void ape_disconnect(ape_socket *co, acetables *g_ape)
{
	subuser *sub = (subuser *)(co->attach);
	if (sub != NULL) {
		
		if (sub->wait_for_free == 1) {
			free(sub);
			co->attach = NULL;
			return;				
		}
		
		if (co->fd == sub->client->fd) {
			sub->headers.sent = 0;
			sub->state = ADIED;
			http_headers_free(sub->headers.content);
			sub->headers.content = NULL;
			if (sub->user != NULL) {
				if (sub->user->istmp) {
					deluser(sub->user, g_ape);
					co->attach = NULL;
				}
			}
		}
		
	}
}

static void ape_onaccept(ape_socket *co, acetables *g_ape)
{
	co->parser = parser_init_http(co);
}

int servers_init(acetables *g_ape)
{
	ape_socket *main_server;
	if ((main_server = ape_listen(atoi(CONFIG_VAL(Server, port, g_ape->srv)), CONFIG_VAL(Server, ip_listen, g_ape->srv), g_ape)) == NULL) {
		return 0;
	}

	main_server->callbacks.on_read = ape_read;
	main_server->callbacks.on_disconnect = ape_disconnect;
	main_server->callbacks.on_data_completly_sent = ape_sent;
	main_server->callbacks.on_accept = ape_onaccept;
	
	return main_server->fd;
}

