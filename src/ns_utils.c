/*
** ns_utils.c
**
** Made by (Edward Hervey)
** Login   <bilboed@bilboed.com>
**
** Started on  Wed Jul  3 00:05:30 2002 Edward Hervey
** Last update Sun Feb  4 12:58:24 2007 
**
** The code for ns_encode/ns_decode is mostly from case (planar_m@epita.fr)
**
*/
#include <string.h>
#include "netsoul.h"
#define BACK_SLASH_N '\n'
#define CHAR_URI_ESC '%'
#define BACK_SLASH '\\'



int ns_back_slash_decode(char *buff)
{
  char          *write;

  for (write = buff; *buff; buff++, write++)
    if (*buff == BACK_SLASH)
    {
      if (*(buff + 1) == BACK_SLASH)
      {
	*write = BACK_SLASH;
	buff++;
      }
      else if (*(buff + 1) == 'n')
	{
	  *write = '\n';
	  buff++;
	}
      else
	*write = BACK_SLASH;
    }
    else
      *write = *buff;
  *write = 0;
  return (1);
}

int ns_url_decode(char *buff)
{
  char          *write;
  char          nbr[3];
  guint           chr;

  nbr[2] = 0;
  for (write = buff; *buff; buff++, write++)
    if (*buff == CHAR_URI_ESC)
      {
	if (*(++buff) == CHAR_URI_ESC)
	  *write = CHAR_URI_ESC;
	else
	  {
	    memcpy(nbr, buff, 2);
	    if ((sscanf(nbr, "%x", &chr) == 1)) {
	      if (chr < 256)
		*write = (char) chr;
	      else
		return (0);
	    } else
	      return (0);
	    buff++;
	  }
      }
    else
      *write = *buff;
  *write = 0;
  return (1);
}

void	ns_url_encode_char(char *dst, unsigned char src)
{
  const char * conv = "0123456789abcdef";

  sprintf(dst, "%%%c%c", conv[(src / 16) % 16], conv[src % 16]);
}


char  *ns_url_encode(char *buff)
{
  int nbr;
  char  *ret;

  if (!buff)
    return NULL;
  for (ret = buff, nbr = 0; *ret; ret++)
    if (!((*ret >= 'A' && *ret <= 'Z') ||
	  (*ret >= 'a' && *ret <= 'z') ||
	  (*ret >= '0' && *ret <= '9')))
      nbr += 3;
    else
      nbr++;
  if ((ret = calloc(1, nbr + 1)) == NULL)
    {
      printf("[%s] MALLOC ERROR!\n", __FUNCTION__);
      return (NULL);
    }
  for (nbr = 0; *buff; buff++)
    if (!((*buff >= 'A' && *buff <= 'Z') ||
	  (*buff >= 'a' && *buff <= 'z') ||
	  (*buff >= '0' && *buff <= '9')))
      {
	ns_url_encode_char(ret + nbr, (unsigned char) *buff);
	nbr += 3;
      }
    else
      {
	ret[nbr] = *buff;
	nbr++;
      }
  ret[nbr] = 0;
  return (ret);
}

/* encode \n */
char  *ns_back_slash_encode(char *buff)
{
  int nbr;
  char  *ret;

  if (!buff)
    return NULL;
  for (ret = buff, nbr = 0; *ret; ret++)
    if (*ret == BACK_SLASH_N)
      nbr += 2;
    else
      nbr++;
  if ((ret = calloc(1, nbr + 1)) == NULL)
  {
    printf("[%s] MALLOC ERROR!\n", __FUNCTION__);
    return (NULL);
  }
  for (nbr = 0; *buff; buff++)
    if (*buff == BACK_SLASH_N)
    {
      sprintf(ret + nbr, "\\n");
      nbr += 2;
    }
    else
    {
      ret[nbr] = *buff;
      nbr++;
    }
  ret[nbr] = 0;
  return (ret);
}


char  *url_decode(char  *msg)
{
  char  *ret;
  char  *ret2;

  if (!msg)
    return NULL;
  if ((ret = g_strdup(msg)) == NULL) {
    free(msg);
    purple_debug_warning("netsoul", "pointeur NULL: url_decode(1)");
    return (NULL);
  }
  if (ns_url_decode(ret) == 0)
  {
    free(ret);
    purple_debug_warning("netsoul", "pointeur NULL: url_decode(2)");
    return (NULL);
  }
  if (ns_back_slash_decode(ret) == 0)
  {
    free(ret);
    purple_debug_warning("netsoul", "pointeur NULL: url_decode(3)");
    return (NULL);
  }
  ret2 = g_convert(ret, strlen(ret), "UTF-8", "ISO-8859-15", NULL, NULL, NULL);
  /*  ret2 = g_locale_to_utf8(ret, strlen(ret), NULL, NULL, NULL); */
  if (ret2)
    g_free(ret);
  else
    ret2 = ret;
  return (ret2);
}



char  *url_encode(char  *msg)
{
  char *tmp;
  char *ret;
  char  *msg2;

  if (!msg)
    return NULL;
  g_strescape(msg, NULL);
  msg2 = g_convert(msg, strlen(msg), "ISO-8859-15", "UTF-8", NULL, NULL, NULL);
  /*  msg2 = g_locale_from_utf8(msg, strlen(msg), NULL, NULL, NULL); */
  if (!msg2)
    msg2 = g_strdup(msg);
  if ((tmp = ns_back_slash_encode(msg2)) == NULL)
    {
      purple_debug_warning("netsoul", "pointeur NULL: url_encode(1)");
      return (NULL);
    }
  g_free(msg2);
  if ((ret = ns_url_encode(tmp)) == NULL)
  {
    purple_debug_warning("netsoul", "pointeur NULL: url_encode(2)");
    g_free (tmp);
    return (NULL);
  }
  g_free(tmp);
  return (ret);
}

/*
  convertname
  converts the full login info into a login@location string
*/
char  *convertname(char **fulllogin)
{
  char    **tab;
  char    *loc;
  char    *res;

  tab = g_strsplit(fulllogin[3], "@", 0);
  loc = url_decode(fulllogin[5]);
  res = g_strdup_printf("%s@%s", *tab, loc);
  g_strfreev(tab);
  g_free(loc);
  return (res);
}


char  *crypt_pass(char *password)
{
  unsigned char	pass[16];
  char		*out;
  size_t	len;
  PurpleCipher *md5;
  PurpleCipherContext *context;

  md5 = purple_ciphers_find_cipher ("md5");
  context = purple_cipher_context_new (md5, NULL);
  purple_cipher_context_append (context, (const guchar *)password, strlen (password));
  purple_cipher_context_digest (context, strlen (password), pass, &len);
  purple_cipher_context_destroy (context);

  out = g_strdup_printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			pass[0],pass[1],pass[2],pass[3],pass[4],pass[5],pass[6],pass[7],pass[8],
			pass[9],pass[10],pass[11],pass[12],pass[13],pass[14],pass[15]);
  return (out);
}

/*
  ns_readable_time
  Returns readable version of tim (since 01/01/1970 00:00)
*/

char  *ns_readable_time(long int tim)
{
  return (g_strdup(ctime((const time_t *) &tim)));
}
