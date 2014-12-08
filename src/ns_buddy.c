/*
 * @file ns_buddy.c
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

/*
  ns_watch_buddy
  Add the given buddy to the netsoul watchlist
*/

void	ns_watch_buddy(PurpleConnection *gc, PurpleBuddy *gb)
{
  NetsoulData	*ns = gc->proto_data;
  NetsoulBuddy	*nb;
  char	**login;

  login = g_strsplit(gb->name, "@", 2);
  purple_debug_info("netsoul", "ns_watch_buddy: %s\n", *login);
  nb = g_new0(NetsoulBuddy, 1);
  gb->proto_data = nb;
  nb->login = g_strdup(*login);
  if (!g_list_find(ns->watchlist, nb->login))
    ns->watchlist = g_list_append(ns->watchlist, nb->login);
  g_strfreev(login);
}

int	ns_text_to_state(char *state)
{
  if (!strcmp(state, "connection"))
    return NS_STATE_CONNECTION;
  if (!strcmp(state, "actif"))
    return NS_STATE_ACTIF;
  if (!strcmp(state, "away"))
    return NS_STATE_AWAY;
  if (!strcmp(state, "idle"))
    return NS_STATE_IDLE;
  if (!strcmp(state, "server"))
    return NS_STATE_SERVER;
  if (!strcmp(state, "lock"))
    return NS_STATE_LOCK;
  return NS_STATE_OTHER;
}

char	*ns_state_to_text(int state)
{
  if (state == NS_STATE_CONNECTION)
    return g_strdup("Connection");
  if (state == NS_STATE_ACTIF)
    return g_strdup("Actif");
  if (state == NS_STATE_AWAY)
    return g_strdup("Away");
  if (state == NS_STATE_IDLE)
    return g_strdup("Idle");
  if (state == NS_STATE_SERVER)
    return g_strdup("Server");
  if (state == NS_STATE_LOCK)
    return g_strdup("Lock");
  if (state == NS_STATE_ACTIF_MORE)
    return g_strdup("Actif+");
  if (state == NS_STATE_SEVERAL_ACTIF)
    return g_strdup("Several Actif");
  if (state == NS_STATE_SEVERAL_INACTIF)
    return g_strdup("Several Inactif");
  return g_strdup("Unknown");
}

void
inform_conv (PurpleConnection *gc, PurpleBuddy *gb, gboolean idchanged, gboolean waschatty)
{
  PurpleConversation	*conv;
  NetsoulBuddy	*nb = gb->proto_data;
  gchar		*tmp;

  purple_debug_info ("netsoul", "inform_conv %s idchanged:%d\n",
		   gb->name, idchanged);
  if (!(conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_ANY, gb->name, purple_connection_get_account(gc))))
    return;
  if (!nb->nblocations)
    return;
  if ((nb->state == NS_STATE_SEVERAL_ACTIF) || (nb->state == NS_STATE_SEVERAL_INACTIF)) {
    if (!waschatty) {
      const char	*message;
      if (nb->state == NS_STATE_SEVERAL_ACTIF)
        message = "%s is active at several locations. Now sending messages to all locations.";
      else
        message = "%s is inactive at several locations. Now sending messages to all locations.";
      tmp = g_strdup_printf (message, (gb->alias) ? gb->alias : gb->name );
      purple_conversation_write(conv, NULL, tmp, PURPLE_MESSAGE_SYSTEM, time(NULL));
      g_free (tmp);
    }
  } else {
    if (waschatty) {
      NetsoulConn	*nc;
      if (!(nc = find_conn_id(nb, nb->defaultid)))
	return;
      tmp = g_strdup_printf ("Messages to %s are now only sent to one location [%s]@%s",
			     (gb->alias) ? gb->alias : gb->name,
			     nc->location, nc->ip);
      purple_conversation_write(conv, NULL, tmp, PURPLE_MESSAGE_SYSTEM, time(NULL));
      g_free (tmp);
    }
  }

  /* If it is talking to everybody
   *   If it Was not talking to everybody before
   *     Inform that it is now
   * Else
   *   If it was talking to several people before
   *     Inform that it isn't anymore
   */
}

void	ns_compute_update_state(PurpleConnection *gc, PurpleBuddy *gb)
{
  GList	*tmp;
  NetsoulConn	*nc;
  gboolean	waschatty;
  int		nbc, defid, oldid;
  NetsoulBuddy	*nb = gb->proto_data;
  PurpleAccount	*account = purple_connection_get_account (gc);

  purple_debug_info("netsoul", "compute_update_state : %s\n", gb->name);
  oldid = nb->defaultid;
  waschatty = ((nb->state == NS_STATE_SEVERAL_ACTIF) || (nb->state == NS_STATE_SEVERAL_INACTIF));
  /* if buddy not logged, state = OFFLINE */
  if (!(nb->nblocations)) {
    purple_debug_info("netsoul", "compute : nb0\n");
    nb->state = NS_STATE_NOT_CONNECTED;
    nb->signon = 0;
    nb->laststate = 0;
    nb->defaultid = 0;
  } else if (nb->nblocations == 1) {
    /* else if buddy logged once, copy only conn data */
    purple_debug_info("netsoul", "compute : nb1\n");
    nc = nb->locationlist->data;
    nb->state = nc->state;
    nb->signon = nc->logintime;
    nb->laststate = nc->statetime;
    nb->defaultid = nc->id;
  } else {  /* else if logged several times compute mixed status */
    purple_debug_info("netsoul", "compute : nb+\n");
    /* parse through connections */
    for (tmp = nb->locationlist, nbc = defid = 0; tmp; tmp = tmp->next) {
      nc = tmp->data;
      /*   get the earliest logintime */
      if (nb->signon > nc->logintime)
	nb->signon = nc->logintime;
      /*   get the latest statetime */
      if (nb->laststate < nc->statetime)
	nb->laststate = nc->statetime;
      /*   count the number of active connections */
      if (nc->state == NS_STATE_ACTIF) {
	nbc++;
	defid = nc->id;
      }
    }
    if (nbc == 1) {   /* one actif */
      nb->state = NS_STATE_ACTIF_MORE;
      nb->defaultid = defid;
    } else if (nbc > 1) { /* several actif */
      nb->state = NS_STATE_SEVERAL_ACTIF;
      nb->defaultid = 0;
    } else { /* several inactif */
      nb->state = NS_STATE_SEVERAL_INACTIF;
      nb->defaultid = 0;
    }
  }

  /* Inform Purple that status changed */
  if (nb->state == NS_STATE_NOT_CONNECTED)
    purple_prpl_got_user_status(account, gb->name, "offline", NULL);
  else if ((nb->state == NS_STATE_ACTIF)
           || (nb->state == NS_STATE_ACTIF_MORE)
           || (nb->state == NS_STATE_SEVERAL_ACTIF)
           || (nb->state == NS_STATE_CONNECTION)) {
    purple_prpl_got_user_status(account, gb->name, "available", NULL);
    purple_prpl_got_user_idle(account, gb->name, FALSE, 0);
  }
  else {
    purple_prpl_got_user_status(account, gb->name, "away", NULL);
    purple_prpl_got_user_idle(account, gb->name, 1, -1);
  }
  inform_conv(gc, gb, oldid == nb->defaultid, waschatty);
}

/*
** Ajoute des champs au menu contextuel de la buddy list
*/
GList *ns_buddy_menu(PurpleBuddy *gb)
{
  GList *m = NULL;
  /* PurpleBlistNodeAction *act; */

  purple_debug_info("netsoul", "ns_buddy_menu on buddy: %s\n", gb->name);
  /*  act = purple_blist_node_action_new(_("Initiate _Chat"), */
  /*				   ns_chat_send_start, gb); */
  /* m = g_list_append(m, act); */
  return m;
}

