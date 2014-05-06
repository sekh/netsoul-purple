/*
 * @file location_detector.c
 *
 * gaim-netsoul Protocol Plugin
 *
 * Copyright (C) 2013, Mickael Falck <lastmikoi@gmail.com>
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

#include <net/ethernet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
//#include <net/if.h>
#include <stropts.h>

/* Including kernel headers in userspace */
#ifndef __user
#define __user
#endif
#include <linux/wireless.h>

#include "netsoul.h"

typedef struct s_location
{
  const char	*ap_addr;
  const char	*verbose_name;
}		t_location;

static t_location	g_ap_list[] =
  {
      {"00:00:00:00:00:00", NULL},
      {"40:18:B1:0A:F5:15", "Near lab cisco"},
  };

/*
* Taken from wireless-tools/iwlib.c
* Copyright (c) 1997-2007 Jean Tourrilhes <jt@hpl.hp.com>
*
* Open a socket.
* Depending on the protocol present, open the right socket. The socket
* will allow us to talk to the driver.
*/
int iw_sockets_open(void)
{
  static const int families[] = {
	AF_INET, AF_IPX, AF_AX25, AF_APPLETALK
  };
  unsigned int i;
  int sock;

  /*
   * Now pick any (exisiting) useful socket family for generic queries
   * Note : don't open all the socket, only returns when one matches,
   * all protocols might not be valid.
   * Workaround by Jim Kaba <jkaba@sarnoff.com>
   * Note : in 99% of the case, we will just open the inet_sock.
   * The remaining 1% case are not fully correct...
   */

  /* Try all families we support */
  for(i = 0; i < sizeof(families)/sizeof(int); ++i)
    {
      /* Try to open the socket, if success returns it */
      sock = socket(families[i], SOCK_DGRAM, 0);
      if(sock >= 0)
	return sock;
    }
  return -1;
}

/*
 * Inspired from wireless-tools/iwlib.{h,c}
 */
static void	ap_addr_to_buf(const struct ether_addr *eth, char * buf)
{
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
      eth->ether_addr_octet[0], eth->ether_addr_octet[1],
      eth->ether_addr_octet[2], eth->ether_addr_octet[3],
      eth->ether_addr_octet[4], eth->ether_addr_octet[5]);
}

static struct ether_addr *get_ap_addr()
{
  struct ether_addr	*ethp;
  struct iwreq		wrq;
  int			socketfd;
  char			*ifname = "wlan0";


  ethp = malloc(sizeof(*ethp));
  socketfd = iw_sockets_open();
  /* Set device name */
  strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
  /* Do the request */
  if (ioctl(socketfd, SIOCGIWAP, &wrq) == -1)
    return (NULL);
  close(socketfd);
  memcpy(ethp, wrq.u.ap_addr.sa_data, sizeof(*ethp));
  return (ethp);
}

/* --- */

static const char	*find_ap_name(const char *ap_addr)
{
  size_t		i;

  for (i = 0; i != (sizeof(g_ap_list) / sizeof(*g_ap_list)); ++i)
    if (strcmp(ap_addr, g_ap_list[i].ap_addr) == 0)
      return g_ap_list[i].verbose_name;
  return (NULL);
}

char			*get_location(char *default_location)
{
  const char		*verbose_loc;
  char			buff[256];
  struct ether_addr	*ethp;

  buff[0] = 0;
  ethp = get_ap_addr();
  if (ethp == NULL)
    return (default_location);
  ap_addr_to_buf(ethp, buff);
  free(ethp);
  if ((verbose_loc = find_ap_name(buff)) != NULL)
    return (strdup(verbose_loc));
  else
    return (strdup(default_location));
}
