/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * mail.c
 * This file is part of u2ps
 *
 * Copyright (C) 2004 Yukihiro Nakai
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
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA. * *
 */

#include <mail.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

static const gchar* worthy_headers[] = {
  "From",
  "To",
  "Reply-To",
  "Date",
  "Subject",
  "X-Mailer",
  "Sender",
  "Cc",
  "Organization",
  "User-Agent",
  NULL,
};

/* Returns UTF-8 str */
static gchar*
subject_decode(guchar* subject) {
  const gchar* base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  guchar* cursor;
  guchar* result = g_strdup("");
  guchar buf[4];
  gint i = 0;
  guchar* tmpbuf;
  gchar* prefix;
  gchar* b64codeset = NULL;

  GError* conv_error = NULL;
  gsize bytes_read, bytes_written;

  g_return_val_if_fail(subject != NULL, NULL);
  g_return_val_if_fail(*subject != '\0', NULL);

  /* base64 start check */
  cursor = strstr(subject, "=?");
  if( !cursor ) {
    return g_strdup(subject);
  }

  prefix = g_strndup(subject, cursor - subject);

  cursor += 2;

  /* base64 end check */
  if( !strstr(cursor, "?=") ) {
    g_free(prefix);
    return g_strdup(subject);
  }

  b64codeset = g_strndup(cursor, (guchar*)strstr(cursor, "?") - cursor );

  if( *b64codeset == '\0' ) {
    g_free(b64codeset);
    g_free(prefix);
    return g_strdup(subject);
  }

  cursor = strstr(cursor, "?");
  if( g_ascii_strncasecmp(cursor, "?b?", strlen("?b?")) ) {
    g_free(prefix);
    return g_strdup(subject); /* B encoding support, currently */
  }
  cursor += strlen("?b?");

  i = 0;
  while( strncmp(cursor, "?=", strlen("?=")) != 0 ) {
    guchar c;
    bzero(buf, 4);

    c = strchr(base64, cursor[0])-base64;
    buf[0] = c<<2;
    c = strchr(base64, cursor[1])-base64;
    buf[0] |= c>>4;
    buf[1]  = c<<4;
    c = (cursor[2] != '=') ? strchr(base64, cursor[2])-base64 : 0;
    buf[1] |= c>>2;
    buf[2]  = c<<6;
    c = (cursor[3] != '=') ? strchr(base64, cursor[3])-base64 : 0;
    buf[2] |= c;

    tmpbuf = g_strconcat(result, buf, NULL);
    g_free(result);
    result = tmpbuf;
    cursor += 4;
  }

  /* Convert to UTF-8 */
  tmpbuf = g_convert(result, -1, "UTF-8", b64codeset, &bytes_read, &bytes_written, &conv_error);
  if( conv_error ) {
    g_print("%s\n", conv_error->message);
    g_error_free(conv_error);
    if( tmpbuf )
      g_free(tmpbuf);

    g_free(result);
    g_free(prefix);
    return g_strdup(subject);
  }
  g_free(result);
  result = tmpbuf;

  tmpbuf = g_strconcat(prefix, result, cursor+2, NULL);
  g_free(result);
  g_free(prefix);
  result = tmpbuf;

  return result;
}

gchar*
get_charset(GSList* mail_slist)
{
  gchar* charset = NULL;
  gchar* content_type = NULL;
  gchar* tmpbuf;

  g_return_val_if_fail(mail_slist != NULL, NULL);
  g_return_val_if_fail(g_slist_length(mail_slist) > 0, NULL);

  gint i;
  for(i=0;i<g_slist_length(mail_slist);i++) {
    gchar* text = g_slist_nth_data(mail_slist, i);

    /* Header end */
    if( *text == '\0' )
      break;

    if( !g_ascii_strncasecmp(text, "Content-Type: ", strlen("Content-Type: ")) ) {
      content_type = g_strdup(text);
      continue;
    }

    if( content_type ) {
      if( *text == '\t' || *text == ' ' ) {
        tmpbuf = g_strconcat(content_type, text, NULL);
        g_free(content_type);
        content_type = tmpbuf;
      } else {
        break;
      }
    }
  }

  if( !content_type )
    return NULL;

  tmpbuf = content_type;
  content_type = g_ascii_strdown(tmpbuf, -1);
  g_free(tmpbuf);

  charset = strstr(content_type, "charset=");
  if( !charset ) {
    g_free(content_type);
    return NULL;
  }

  charset += strlen("charset=");

  if( *charset == '\"' ) {
    g_assert(strstr(charset+1, "\"") != NULL);
    tmpbuf = g_strndup(charset+1, strstr(charset+1, "\"")-(charset+1));
    charset = tmpbuf;
  } else {
    charset = g_strchomp(charset);
    charset = g_strdup(charset);
  }

  g_free(content_type);

  return charset;
}

gchar*
get_subject(GSList* mail_slist)
{
  gboolean sbj_start = FALSE;
  gchar* subject = g_strdup("");
  gchar* tmpbuf;
  gint i;

  GError* conv_error = NULL;
  gsize bytes_read, bytes_written;

  g_return_val_if_fail(mail_slist != NULL, NULL);
  g_return_val_if_fail(g_slist_length(mail_slist) > 0, NULL);

  for(i=0;i<g_slist_length(mail_slist);i++) {
    gchar* buf = g_slist_nth_data(mail_slist, i);

    /* Header part end */
    if( !strcmp(buf, "\n") || !strcmp(buf, "\r\n") ) {
      if( sbj_start ) {
        printf("%s", subject);
        sbj_start = FALSE;
      }
      break;
    }

    if( !g_ascii_strncasecmp(buf, "subject:", strlen("subject:")) ) {
      gchar* decoded = subject_decode(buf + strlen("subject: "));
      sbj_start = TRUE;
      tmpbuf = g_strconcat(subject, decoded, NULL);
      g_free(subject);
      subject = tmpbuf;
      if( decoded != NULL )
        g_free(decoded);
      continue;
    }
    if( sbj_start ) {
      if( *buf == '\t' || *buf == ' ' ) { /* 2nd line subject */
        gchar* decoded = subject_decode(buf+1);
        if( subject[strlen(subject)-1] == '\n' ) {
          subject[strlen(subject)-1] = '\0';
        }
        if( subject[strlen(subject)-1] == '\r' ) {
          subject[strlen(subject)-1] = '\0';
        }
        tmpbuf = g_strconcat(subject, decoded, NULL);
        g_free(subject);
        subject = tmpbuf;
        if( decoded )
          g_free(decoded);
        continue;
      } else { /* No more subject line */
        sbj_start = FALSE;
        break;
      }
    }
  }

  return subject;
}

GSList*
cut_headers(GSList* mail_slist)
{
  gint i,j;
  GSList* new_slist = NULL;
  gboolean worthy_on = FALSE; /* Header is continuing */
  gboolean subject_on = FALSE; /* Subject handling */

  for(i=0;i<g_slist_length(mail_slist);i++) {
    gchar* text = g_slist_nth_data(mail_slist, i);
    if( !strcmp(text, "") ) {
      /* Header end */
      break;
    }

    /* Header 2nd line or more */
    if( *text == '\t' || *text == ' ' ) {
      if( subject_on )
        continue;

      if( worthy_on ) {
        new_slist = g_slist_append(new_slist, text);
      }
      continue;
    }
    worthy_on = FALSE;
    subject_on = FALSE;

    /* Decode the subject */
    if( !g_ascii_strncasecmp(text, "subject: ", strlen("subject: ")) ) {
      gchar* subject = get_subject(mail_slist);
      gchar* hprefix = g_strndup(text, strlen("subject: "));
      g_free(text);
      text = g_strconcat(hprefix, subject, NULL);
      g_free(subject);
      g_free(hprefix);
      subject_on = TRUE;
    }

    for(j=0;worthy_headers[j] != NULL;j++) {
      if( !g_ascii_strncasecmp(text, worthy_headers[j], strlen(worthy_headers[j])) ) {
        new_slist = g_slist_append(new_slist, text);
        worthy_on = TRUE;
        break;
      }
    }
  }
  /* Append the rest */
  for(;i<g_slist_length(mail_slist);i++) {
    gchar* text = g_slist_nth_data(mail_slist, i);
    new_slist = g_slist_append(new_slist, text);
  }

  g_slist_free(mail_slist);

  return new_slist;
}
