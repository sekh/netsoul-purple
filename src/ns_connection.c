/*
 * @file ns_connection.c
 *
 * gaim-netsoul Protocol Plugin
 *
 * Copyright (C) 2004, 2007, Edward Hervey <bilboed@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <string.h>
#include "netsoul.h"

int	netsoul_send_passwd(PurpleConnection *gc)
{
  char	*seed;
  char	*res;
  char	*loc;
  char	*com;
  NetsoulData	*ns = gc->proto_data;
  PurpleAccount	*account;

  account = purple_connection_get_account(gc);
  seed = g_strdup_printf("%s-%s/%i%s", ns->challenge, ns->host,
			 ns->port, purple_account_get_password(account));
  res = crypt_pass(seed);
  g_free(seed);
  loc = strdup((char *) purple_account_get_string(account,
					   "location",
					   NETSOUL_DEFAULT_LOCATION));
  if (purple_account_get_bool(account, "locdisc", NETSOUL_DEFAULT_DISCOVERY))
    loc = get_location(loc);
  com = (char *) purple_account_get_string(account,
					   "comment",
					   NETSOUL_DEFAULT_COMMENT);
  seed = g_strdup_printf("ext_user_log %s %s %s %s\n",
			 purple_account_get_username(account), res,
			 url_encode(loc), url_encode(com));
  free(loc);
  return (netsoul_write(ns, seed));
}

static void auth_response (gpointer data, gint source, PurpleInputCondition cond)
{
  PurpleConnection *gc = data;
  NetsoulData *ns = gc->proto_data;
  char	buf[1024];
  char	**tab;

  if ((read(source, buf, 1024)) < 0) {
    purple_connection_error(gc, _("Connection Error\n"));
    return;
  }
  purple_debug_info("netsoul", "auth_response got : %s\n", buf);
  tab = g_strsplit(buf, " ", 6);
  if (strcmp(*tab, "rep")) {
    purple_connection_error(gc, _("Wrong Answer from server\n"));
    return;
  }
  g_strfreev(tab);
  if (netsoul_send_passwd(gc) < 0) {
    purple_connection_error(gc, _("Error while sending password\n"));
    return;
  }
  purple_input_remove(gc->inpa);
  ns->state = NS_STATE_SENT_EXTUSERLOG;
  purple_debug_info("netsoul", "password sent\n");
  gc->inpa = purple_input_add(source, PURPLE_INPUT_READ, ns_listen, gc);
  purple_connection_update_progress(gc, _("Password sent\n"), 2, 3);
  netsoul_get_buddies (gc);
}

static void netsoul_login_greeting (gpointer data, gint source, PurpleInputCondition cond)
{
  PurpleConnection *gc = data;
  NetsoulData *ns = gc->proto_data;
  char	buf[1024];
  char	**tab;
  memset(&buf, 0, 1024);

  if ((read(source, buf, 1024)) < 0)
  {
    purple_debug_info ("netsoul", "!!! %s %u\n", strerror (errno), PURPLE_INPUT_READ & cond);
    purple_connection_error(gc, _("Couldn't read from server"));
    return;
  }
  purple_debug_info("netsoul", "netsoul_login_connect received: %s\n", buf);
  tab = g_strsplit(buf, " ", 6);
  if ((g_strv_length(tab) != 6) || strncmp(tab[0], "salut", 5))
  {
    purple_connection_error(gc, _("Wrong greetings from server\n"));
    purple_debug_info("netsoul", "Error on str : %s\n", buf);
    g_strfreev(tab);
    return;
  }
  ns->id = atoi(tab[1]);
  purple_debug_info("netsoul", "netsoul_login_connect tab[2]: %p\n", tab[2]);
  ns->challenge = strdup(tab[2]);
  ns->host = strdup(tab[3]);
  ns->port = atoi(tab[4]);
  ns->fd = source;
  ns->read_len = 0;
  ns->pos = 0;
  ns->len = 0;
  if (netsoul_write(ns, "auth_ag ext_user none none\n") < 0)
  {
    purple_connection_error(gc, _("Connection Error"));
    g_strfreev(tab);
    return;
  }

  purple_input_remove (gc->inpa);
  gc->inpa = purple_input_add(source, PURPLE_INPUT_READ, auth_response, gc);
  purple_debug_info("netsoul", "auth_ag sent, waiting for response\n");
  ns->state = NS_STATE_SENT_AUTH;
  purple_connection_update_progress(gc, _("auth_ag sent, waiting for response\n"), 1, 3);
  g_strfreev(tab);
  return;
}

static void netsoul_login_connect (gpointer data, gint source, const gchar *error)
{
  PurpleConnection *gc = data;

  purple_debug_info ("netsoul", "netsoul_login_connect starts %u\n", source);
  if (source < 0 || !g_list_find(purple_connections_get_all(), gc)) {
    purple_connection_error(gc, _("Couldn't connect to host"));
    close(source);
    purple_debug_info("netsoul", "error \"%s\"", error);
    return;
  }
  gc->inpa = purple_input_add(source, PURPLE_INPUT_READ, netsoul_login_greeting, gc);
}

void netsoul_login (PurpleAccount *account)
{
  PurpleConnection *gc = purple_account_get_connection(account);
  NetsoulData *ns = gc->proto_data = g_new0(NetsoulData, 1);
  PurpleProxyConnectData *err;

  purple_debug_info("netsoul", "netsoul_login\n");

  ns->account = account;
  purple_debug_info ("netsoul", "Connection on %s on port %u\n",
		   purple_account_get_string(account, "server", NETSOUL_DEFAULT_SERVER),
		   purple_account_get_int(account, "port", NETSOUL_DEFAULT_PORT));
  err = purple_proxy_connect(NULL, account, purple_account_get_string(account, "server", NETSOUL_DEFAULT_SERVER),
			   purple_account_get_int(account, "port", NETSOUL_DEFAULT_PORT),
			   netsoul_login_connect, gc);
  if (err == NULL || !purple_account_get_connection (account)) {
    purple_connection_error(gc, _("Couldn't create connection"));
    return;
  }

  purple_connection_update_progress(gc, _("Connecting"), 0, 3);
  ns->state = NS_STATE_CONNECTING;
}
