/*
 * @file netsoul.h
 *
 * gaim-netsoul Protocol Plugin
 *
 * Copyright (C) 2004, Edward Hervey <bilboed@gmail.com>
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

#ifndef _NETSOUL_H_
#define _NETSOUL_H_

#ifdef HAVE_CONFIG_H
# include "../netsoul_config.h"
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "time.h"
#include "plugin.h"
#include "accountopt.h"
#include "prpl.h"
#include "conversation.h"
#include "notify.h"
#include "debug.h"
#include "blist.h"
#include "util.h"
#include "version.h"
#include "cipher.h"
#include "imgstore.h"

#define NETSOUL_WEBSITE "http://projects.irq6.net/projects/show/netsoul-purple"

#define NETSOUL_DEFAULT_SERVER "ns-server.epita.fr"
#define NETSOUL_DEFAULT_PORT 4242
#define NETSOUL_DEFAULT_LOCATION "Home"
#define NETSOUL_DEFAULT_DISCOVERY 0
#define NETSOUL_DEFAULT_COMMENT "netsoul-purple"

#define NETSOUL_PHOTO_URL "http://cdn.local.epitech.eu/userprofil/profilview/"

#define NS_BUF_LEN 4096

typedef enum {
  NS_STATE_NOT_CONNECTED,
  NS_STATE_CONNECTING,
  NS_STATE_SENT_AUTH,
  NS_STATE_SENT_EXTUSERLOG,
  NS_STATE_CONNECTION,
  NS_STATE_ACTIF,
  NS_STATE_AWAY,
  NS_STATE_IDLE,
  NS_STATE_SERVER,
  NS_STATE_LOCK,
  NS_STATE_ACTIF_MORE,	// logged several times, but only one active
  NS_STATE_SEVERAL_ACTIF,	// active at several locations
  NS_STATE_SEVERAL_INACTIF,	// logged several times, none active
  NS_STATE_OTHER		// undefined state !?!
} NetsoulState;

typedef struct		_NetsoulData {
  /*
    donnees propres au protocole Netsoul
   */
  NetsoulState		state;
  PurpleAccount		*account;
  int			id;		// Netsoul ID
  char			*challenge;	// Challenge for auth
  char			*host;		// hostname seen by server
  int			port;		// local port used for connection
  int			fd;
  PurpleConvChat		conv;
  GList			*watchlist;
  char			read_buf[NS_BUF_LEN];
  int			read_len;
  int			pos;
  char			buf[NS_BUF_LEN];
  int			len;
}			NetsoulData;

typedef struct  _NetsoulBuddy {
  char		*login;		// login
  char		*group;		// group
  int		state;		// Buddy global state
  long int	signon;		// global signon time
  long int	laststate;	// global last state time
  int		defaultid;	// default communication id
  int		nblocations;	// number of locations
  GList		*locationlist;	// List of NetsoulConn
}		NetsoulBuddy;

typedef struct	_NetsoulConn {
  int		id;		// Connection ID
  long int	logintime;	// connection login time
  long int	statetime;	// connection last state time
  char		*ip;		// connection ip
  char		*location;	// connection location
  char		*comment;	// connection comment
  int		state;		// connection netsoul state
}		NetsoulConn;

/*
  netsoul.c
*/
void netsoul_get_buddies (PurpleConnection* gc);

/*
  ns_buddy.c
*/

void	ns_watch_buddy(PurpleConnection *gc, PurpleBuddy *gb);
int	ns_text_to_state(char *state);
char	*ns_state_to_text(int state);
void	ns_compute_update_state(PurpleConnection *gc, PurpleBuddy *gb);
GList	*ns_buddy_menu(PurpleBuddy *gb);

/*
  ns_connection.c
*/

void	netsoul_login (PurpleAccount *account);

/*
  ns_listen.c
*/

PurpleBuddy	*get_good_stored_buddy(PurpleConnection *gc, char *fullname);
void	ns_listen(gpointer data, gint source, PurpleInputCondition cond);
char	*get_good_msg_user(PurpleConnection *gc, const char *who);


/*
  ns_output.c
*/

int	netsoul_write(NetsoulData *ns, char *data);
void	ns_watch_log_user(PurpleConnection *gc);
void	ns_list_users(PurpleConnection *gc, GList *list);
void	ns_list_users_login(PurpleConnection *gc, char *login);
void	ns_list_users_id(PurpleConnection *gc, int id);
void	ns_send_state(PurpleConnection *gc, int state, long int sincewhen);
void	ns_msg_user(PurpleConnection *gc, const char *who, const char *what);
void	ns_send_typing(PurpleConnection *gc, const char *who, PurpleTypingState typing);
NetsoulConn	*find_conn_id(NetsoulBuddy *nb, int id);

/*
  ns_utils.c
*/
char	*url_decode(char	*msg);
char	*url_encode(char	*msg);
char	*convertname(char **fulllogin);
char	*crypt_pass(char *password);
char	*ns_readable_time(long int tim);

/*
  ns_chat.c
*/
void ns_initiate_chat(PurpleConnection *gc, char *who);
void ns_chat_send_start(PurpleBlistNode *node, gpointer data);
void ns_chat_send_enter(PurpleConnection *gc, const char *who);

/*
  location_detector.c
*/
char *get_location(char *default_location);


#endif
