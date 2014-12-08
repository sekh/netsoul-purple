/*
 * @file ns_output.c
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

int	netsoul_write(NetsoulData *ns, char *data)
{
  int ret;

  if (ns->fd < 0)
    return -1;

  purple_debug_info("netsoul", "netsoul_write [%s]\n",data);
  if ((ret = write(ns->fd, data, strlen(data))) < 0)
    purple_connection_error(purple_account_get_connection(ns->account),
			  _("Server has disconnected"));
  return ret;
}

void	ns_list_users(PurpleConnection *gc, GList *list)
{
  NetsoulData	*ns = gc->proto_data;
  GList	*tmp;
  int	i,len;
  char	**tab;
  char	*resp;
  char	*ensemble;

  if ((len = g_list_length(list)) < 1)
    return;
  tab = g_new0(char *, len + 1);
  for (tmp = list, i = 0; i < len; tmp = tmp->next, i++)
    tab[i] = tmp->data;
  ensemble = g_strjoinv(",", tab);
  resp = g_strdup_printf("list_users {%s}\n", ensemble);
  netsoul_write(ns, resp);
  g_free(resp);
  g_free(ensemble);
  g_free(tab);
}

void	ns_list_users_login(PurpleConnection *gc, char *login)
{
  char	*resp;
  NetsoulData	*ns = gc->proto_data;

  resp = g_strdup_printf("list_users {%s}\n", login);
  netsoul_write(ns, resp);
  g_free(resp);
}

void	ns_list_users_id(PurpleConnection *gc, int id)
{
  char	*resp;
  NetsoulData	*ns = gc->proto_data;

  resp = g_strdup_printf("list_users {:%d}\n", id);
  netsoul_write(ns, resp);
  g_free(resp);
}

void	ns_watch_log_user(PurpleConnection *gc)
{
  NetsoulData	*ns = gc->proto_data;
  GList	*tmp;
  char	**tab;
  int	len, i;
  char	*buf, *res;

  if ((len = g_list_length(ns->watchlist)) < 1)
    return;
  tab = g_new0(char *, len + 1);
  for (tmp = ns->watchlist, i=0; i < len; tmp = tmp->next, i++)
    tab[i] = tmp->data;
  buf = g_strjoinv(",", tab);
  res = g_strdup_printf("user_cmd watch_log_user {%s}\n", buf);
  if (netsoul_write(ns, res) < 0) {
    purple_debug_warning("netsoul", "Error sending state\n");
  }
  g_free(res);
  g_free(buf);
  g_free(tab);
}

void	ns_send_state(PurpleConnection *gc, int state, long int sincewhen)
{
  NetsoulData	*ns = gc->proto_data;
  char		*buf;

  if ((state == NS_STATE_ACTIF) || (state == NS_STATE_CONNECTION))
    buf = g_strdup_printf("state actif:%ld\n", sincewhen);
  else if ((state == NS_STATE_IDLE) || (state == NS_STATE_AWAY))
    buf = g_strdup_printf("state away:%ld\n", sincewhen);
  else
    buf = g_strdup_printf("state lock:%ld\n", sincewhen);
  if (netsoul_write(ns, buf) < 0) {
    purple_debug_warning("netsoul", "Error sending state\n");
  }
}

char	*get_good_msg_user(PurpleConnection *gc, const char *who)
{
  PurpleBuddy	*gb;
  NetsoulBuddy	*nb;
  char		**tab;
  char		*resp;

  tab = g_strsplit(who, "@", 2);
  resp = g_strdup(*tab);
  g_strfreev(tab);

  /* look for a buddy */
  if (!(gb = purple_find_buddy(purple_connection_get_account(gc), who)))
    if (!(gb = purple_find_buddy(purple_connection_get_account(gc), resp))) {
      /* if we don't have a buddy, return the login */
      return resp;
    }
  /* if we have a buddy for who, find the good place to send to */
  nb = gb->proto_data;
  if (nb && (nb->state != NS_STATE_SEVERAL_ACTIF) &&
      (nb->state != NS_STATE_SEVERAL_INACTIF)) {
    g_free(resp);
    resp = g_strdup_printf(":%d", nb->defaultid);
  }
  return resp;
}

void	ns_msg_user(PurpleConnection *gc, const char *who, const char *what) {
  NetsoulData	*ns = gc->proto_data;
  char		*towho;
  char		*resp;
  char		*msg2;

  msg2 = url_encode((char *) what);
  towho = get_good_msg_user(gc, who);
  resp = g_strdup_printf("user_cmd msg_user %s msg %s\n", towho, msg2);
  netsoul_write(ns, resp);
  g_free(towho);
  g_free(resp);
  g_free(msg2);
}

void	ns_send_typing(PurpleConnection *gc, const char *who, PurpleTypingState typing)
{
  NetsoulData	*ns = gc->proto_data;
  char		*towho;
  char		*resp;

  towho = get_good_msg_user(gc, who);
  if (typing == PURPLE_TYPING)
    resp = g_strdup_printf("user_cmd msg_user %s dotnetSoul_UserTyping null\n", towho);
  else
    resp = g_strdup_printf("user_cmd msg_user %s dotnetSoul_UserCancelledTyping null\n", towho);
  netsoul_write(ns, resp);
  g_free(towho);
  g_free(resp);
}

