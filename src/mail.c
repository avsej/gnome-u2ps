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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libgnome/gnome-i18n.h>

#include "util.h"
#include "mail.h"

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

static const guchar* hexchar = "0123456789ABCDEF";

/* Get a header line from GSList.
 *
 * Returned value is NOT newly allocated.
 * Argument header should NOT have ": " at the tail,
 * like "Content-Type" or "From".
 */
static const gchar*
get_a_header(GSList* header_slist, const gchar* header) {
  guint i = 0;
  const gchar* text = NULL;
  gchar* headerstr = NULL;

  g_return_val_if_fail(header != NULL, NULL);
  g_return_val_if_fail(*header != '\0', NULL);
  g_return_val_if_fail(!strchr(header, ':'), NULL);
  g_return_val_if_fail(!strchr(header, ' '), NULL);

  headerstr = g_strconcat(header, ": ", NULL);

  for(i=0;i<g_slist_length(header_slist);i++) {
    text = g_slist_nth_data(header_slist, i);
    if( !g_ascii_strncasecmp(text, headerstr, strlen(headerstr)) ) {
      g_free(headerstr);
      return text;
    }

    if( *text == '\0' || *text == ' ' || *text == '\t' ) {
      g_warning(_("Header handling is broken: %s\n"), text);
    }
  }

  g_free(headerstr);
  return NULL;
}

/* Get mimetype from Content-Type: header.
 *
 *  Returned value is newly acllocated.
 */
gchar*
get_mimetype(GSList* header_slist) {
  const gchar* header = get_a_header(header_slist, "Content-Type");
  gchar* cursor = NULL;

  if( !header )
    return NULL;

  header += strlen("Content-Type") + 2; /* Add extra strlen(": ") */
  cursor = strchr(header, ';');
  if( cursor ) {
    return g_strndup(header, cursor - header);
  } else {
    return g_strdup(header);
  }
  
  return NULL;
}

/* Get Header Property
 *
 *  Returned value is newly acllocated.
 */
gchar*
get_header_property(GSList* header_slist, const gchar* header, const gchar* property) {
  const gchar* text = NULL;
  gchar* cursor = NULL;
  gchar* propstr = NULL;
  gchar* scolon = NULL; /* Semicolon ';' */
  gchar* dquote = NULL; /* Double quote '"' */

  g_return_val_if_fail(header != NULL, NULL);
  g_return_val_if_fail(*header != '\0', NULL);
  g_return_val_if_fail(!strchr(header, ':'), NULL);
  g_return_val_if_fail(!strchr(header, ' '), NULL);
  g_return_val_if_fail(property != NULL, NULL);
  g_return_val_if_fail(*property != '\0', NULL);

  text = get_a_header(header_slist, header);

  if( !text )
    return NULL;

  text += strlen(header) + 2; /* Add an extra strlen(": ") */
  propstr = g_strconcat(property, "=", NULL);

  cursor = strstr(text, propstr);

  if( !cursor ) {
    g_free(propstr);
    return NULL;
  }

  cursor += strlen(propstr);
  g_free(propstr);

  if( *cursor == '"' )
    cursor++;

  scolon = strchr(cursor, ';');
  dquote = strchr(cursor, '"');

  if( scolon == NULL && dquote == NULL ) {
    return g_strdup(cursor);
  } else if ( scolon != NULL && dquote != NULL ) {
    return g_strndup(cursor, MIN(scolon, dquote)-cursor);
  } else if( scolon != NULL ) {
    return g_strndup(cursor, scolon-cursor);
  } else { /* dquote != NULL */
    return g_strndup(cursor, dquote-cursor);
  }

  return NULL; /* Never reach here, logically */
}

/* Get headers from mail or mpart
 * 
 *  Returned value is newly allocated.
 */
GSList*
get_headers(GSList* msg_slist) {
  guint i = 0;
  GSList* header_slist = NULL;
  const gchar* text = NULL;

  for(i=0;i<g_slist_length(msg_slist);i++) {
    text = g_slist_nth_data(msg_slist, i);

    if( *text == '\0' )
      break; /* Header end */

    if( header_slist != NULL && (*text == ' ' || *text == '\t') ) {
      gchar* tmpbuf = NULL;
      gchar* last = g_slist_last(header_slist)->data;
      if( !last ) {
        continue;
      }
      tmpbuf = g_strconcat(last, text+1, NULL);
      g_free(last);
      g_slist_last(header_slist)->data = tmpbuf;
      continue;
    }

    header_slist = g_slist_append(header_slist, g_strdup(text));
  }

  return header_slist;
}

/* Get body from mail or mpart
 * 
 *  Returned value is newly allocated.
 */
GSList*
get_body(GSList* msg_slist) {
  guint i = 0;
  gchar* text = NULL;
  GSList* body_slist = NULL;
  
  for(i=0;i<g_slist_length(msg_slist);i++) {
    text = g_slist_nth_data(msg_slist, i);

    if( *text == '\0' )
      break;
  }

  if( !text || *text != '\0' )
    return NULL;

  for(;i<g_slist_length(msg_slist);i++) {
    text = g_slist_nth_data(msg_slist, i);
    body_slist = g_slist_append(body_slist, g_strdup(text));
  }

  return body_slist;
}

/* Decode quoted-printable message */
gchar*
decode_quoted_printable(gchar* str) {
  guint i = 0;
  gboolean shift = 0;
  gchar *cursor = NULL, *result = NULL;
  guchar c = '\0';

  g_return_val_if_fail(str != NULL, NULL);

  cursor = result = g_new0(gchar, strlen(str)+1);

  for(i=0;i<strlen(str);i++) {
    if( shift == 0 && str[i] == '=' ) {
      shift = 1;
      c = '\0';
    } else if( shift == 1 ) {
      gchar* pos = strchr((gchar*)hexchar, str[i]);
      if( pos ) {
        c = (pos - (gchar*)hexchar)<<4;
      }
      shift = 2;
    } else if( shift == 2 ) {
      gchar* pos = strchr((gchar*)hexchar, str[i]);
      if( pos ) {
        c += pos - (gchar*)hexchar;
      }
      *cursor++ = c;
      shift = 0;
    } else {
      *cursor++ = str[i];
    }
  }

  return result;
}

GSList*
decode_qp_message(GSList* text_slist) {
  GSList* new_slist = NULL;
  guint i = 0;
  gboolean quoted_printable = FALSE;
  gboolean message_body = FALSE; /* Should not decode headers */
  gchar* prevtext = NULL;

  g_return_val_if_fail(text_slist != NULL, NULL);

  /* Checking the header declares quoted-printable or not */
  for(i=0;i<g_slist_length(text_slist);i++) {
    gchar* text = g_slist_nth_data(text_slist, i);
    if( !g_ascii_strncasecmp(text, "Content-Transfer-Encoding: ", strlen("Content-Transfer-Encoding: ")) ) {
      if( !g_ascii_strncasecmp(text+strlen("Content-Transfer-Encoding: "), "quoted-printable", strlen("quoted-printable")) ) {
        quoted_printable = TRUE;
        break;
      }
    }
  }

  if( !quoted_printable )
    return NULL;

  for(i=0;i<g_slist_length(text_slist);i++) {
    gchar* newtext = NULL;
    gchar* text = g_slist_nth_data(text_slist, i);

    /* Header part should not be decoded */
    if( !message_body ) {
      if( *text == '\0' ) {
        message_body = TRUE;
      }
      new_slist = g_slist_append(new_slist, g_strdup(text));
      continue;
    }

    if( prevtext ) {
      gchar* tmpbuf = g_strconcat(prevtext, text, NULL);
      text = tmpbuf;
      g_free(prevtext);
      prevtext = NULL;
    }

    if( text[strlen(text)-1] == '=' ) {
      text[strlen(text)-1] = '\0';
      prevtext = text;
      continue;
    }

    newtext = decode_quoted_printable(text);
    new_slist = g_slist_append(new_slist, newtext);
  }

  if( prevtext ) {
    new_slist = g_slist_append(new_slist, prevtext);
    g_free(prevtext);
  }

  return new_slist;
}

static guchar*
base64_decode_simple(guchar* str) {
  const gchar* base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  guchar* result = NULL;
  guchar* cursor = NULL;
  guchar* cursor2 = NULL;
  if(strlen(str) % 4 != 0 ) {
    g_warning("Cannot decode base64");
    return g_strdup(str);
  }

  cursor2 = result = g_new0(guchar, strlen(str)+ 1);
  cursor = str;

  while( *cursor != '\0' ) {
    guchar c = 0;

    c = strchr(base64, cursor[0])-base64;
    cursor2[0] = c<<2;
    c = strchr(base64, cursor[1])-base64;
    cursor2[0] |= c>>4;
    cursor2[1]  = c<<4;
    c = (cursor[2] != '=') ? strchr(base64, cursor[2])-base64 : 0;
    cursor2[1] |= c>>2;
    cursor2[2]  = c<<6;
    c = (cursor[3] != '=') ? strchr(base64, cursor[3])-base64 : 0;
    cursor2[2] |= c;

    cursor2 += 3;
    cursor += 4;
  }

  return result;
}

/* Returns UTF-8 str */
static gchar*
base64_decode_sub(guchar* subject) {
  const gchar* base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  guchar* cursor;
  guchar* result = g_strdup("");
  guchar buf[4];
  gint i = 0;
  guchar* tmpbuf;
  gchar* prefix = NULL;
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
  /* Q encoding is supported */
  if( !g_ascii_strncasecmp(cursor, "?q?", strlen("?q?")) ) {
    cursor += strlen("?q?");
    while( *cursor != '\0' && strncmp(cursor, "?=", strlen("?=")) != 0 ) {
      guchar* cpos;
      guchar c = 0;

      if( *cursor != '=' ) {
        tmpbuf = g_malloc0(strlen(result)+2);
        strcpy(tmpbuf, result);
        g_free(result);
        result = tmpbuf;
        if( *cursor == '_' ) {
          result[strlen(result)] = ' ';
        } else {
          result[strlen(result)] = *cursor;
        }
        cursor++;
        continue;
      }

      cursor++;
      cpos = strchr(hexchar, *cursor);
      if( !cpos ) {
        g_free(b64codeset);
        g_free(prefix);
        return g_strdup(subject); /* Never allow fault */
      }
      c = (cpos - hexchar) << 4;
      cursor++;
      cpos = strchr(hexchar, *cursor);
      if( !cpos ) {
        g_free(b64codeset);
        g_free(prefix);
        return g_strdup(subject); /* Never allow fault */
      }
      c += cpos - hexchar;
      tmpbuf = g_malloc0(sizeof(gchar)*(strlen(result)+2));
      strcpy(tmpbuf, result);
      tmpbuf[strlen(result)] = c;
      g_free(result);
      result = tmpbuf;
      cursor++;
    }
    if( !strncmp(cursor, "?=", strlen("?=")) ) {
      cursor += strlen("?=");
    }
    tmpbuf = u2ps_convert(result, b64codeset);
    if( tmpbuf ) {
      g_free(result);
      result = tmpbuf;
    }
    tmpbuf = g_strconcat(prefix, result, cursor, NULL);
    g_free(prefix);
    g_free(result);
    return tmpbuf;
  }

  /* B encoding is supported */
  if( g_ascii_strncasecmp(cursor, "?b?", strlen("?b?")) != 0 ) {
    g_free(prefix);
    return g_strdup(subject);
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
  tmpbuf = u2ps_convert(result, b64codeset);
  if( tmpbuf ) {
    g_free(result);
    result = tmpbuf;
  }

  tmpbuf = g_strconcat(prefix, result, cursor+2, NULL);
  g_free(result);
  g_free(prefix);
  result = tmpbuf;

  return result;
}

/* Recurse base64 decode */
gchar*
base64_decode(guchar* str) {
  gchar* tmpbuf = NULL;
  gchar* decoded = base64_decode_sub(str);

  if( !decoded )
    return g_strdup(str);

  while( strstr(decoded, "=?") ) {
    tmpbuf = base64_decode_sub(decoded);

    if( !tmpbuf )
      break;

    if( !strcmp(tmpbuf, decoded) ) {
      g_free(tmpbuf);
      break;
    }

    g_free(decoded);
    decoded = tmpbuf;
  }

  return decoded;
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

  /* If the mail is multipart, no need to handle charset here */
  if( !g_ascii_strncasecmp(content_type, "Content-Type: multipart/", strlen("Content-Type: multipart/")) ) {
    g_free(content_type);
    return g_strdup("us-ascii");
  }

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

  /* Cut the other Content-Type properties after ';' */
  tmpbuf = strchr(charset, ';');
  if( tmpbuf ) {
    *tmpbuf = '\0';
  }

  g_free(content_type);

  return charset;
}

GSList*
print_multi_part(MultiPart* mpart) {
  guint i = 0;
  gchar* name = NULL;
  gchar* mimetype = "";
  GSList* result_slist = NULL;

  if( !mpart )
    return NULL;

  if( mpart->filename )
    name = mpart->filename;

  /* Name should be overwrite to filename */
  if( mpart->name )
    name = mpart->name;

  if( mpart->mimetype )
    mimetype = mpart->mimetype;

  if( name ) {
    if( mpart->mimetype
      && (!strcmp(mpart->mimetype, "text/plain")
      ||  !strcmp(mpart->mimetype, "text/x-patch")) ) {
      result_slist = g_slist_append(result_slist, g_strdup_printf(_("[Inline Attachment: %s %s]"), name, mimetype));
    } else if( mpart->mimetype && !strcmp(mpart->mimetype, "application/pgp-signature") ) {
      /* Do Nothing */
    } else {
      result_slist = g_slist_append(result_slist, g_strdup_printf(_("[Attachment: %s %s]"), name, mimetype));
    }
  }

  if( mpart->mimetype
    && (!strcmp(mpart->mimetype, "text/plain")
    /* Cannot handle multipart/alternative currently */
    ||  !strcmp(mpart->mimetype, "multipart/alternative")
    /* Cannot handle multpart/signed, either */
    ||  !strcmp(mpart->mimetype, "multipart/signed")
    ||  !strcmp(mpart->mimetype, "text/x-patch")
    ||  !strcmp(mpart->mimetype, "application/pgp-signature") ) ) {
    for(i=0;i<g_slist_length(mpart->body_slist);i++) {
      gchar* text = g_slist_nth_data(mpart->body_slist, i);
      result_slist = g_slist_append(result_slist, g_strdup(text));
    }
    //g_print("\n"); /* Print extra newline for text/plain, for upcoming attachments */
  }

  return result_slist;
}

void
dump_multi_part(MultiPart* mpart) {
  guint i;

  if( !mpart )
    return;

  g_print("\n");
  for(i=0;i<g_slist_length(mpart->header_slist);i++) {
    gchar* text = g_slist_nth_data(mpart->header_slist, i);
    g_print("HEADER: %s\n", text);
  }
//  for(i=0;i<g_slist_length(mpart->body_slist);i++) {
//    gchar* text = g_slist_nth_data(mpart->body_slist, i);
//    g_print("BODY: %s\n", text);
//  }

  g_print("MIMETYPE: %s\n", mpart->mimetype);
  g_print("CHARSET: %s\n", mpart->charset);
  g_print("NAME: %s\n", mpart->name);
  g_print("FILENAME: %s\n", mpart->filename);
  g_print("BASE64: %s\n", mpart->is_base64 ? "TRUE" : "FALSE");
  g_print("QP: %s\n", mpart->is_qp ? "TRUE" : "FALSE");
}

void
multi_part_free(MultiPart* mpart) {
  if( !mpart )
    return;

  g_slist_free(mpart->header_slist);
  g_slist_free(mpart->body_slist);

  mpart->mimetype ? g_free(mpart->mimetype) : TRUE;
  mpart->charset ? g_free(mpart->charset) : TRUE;
  mpart->name ? g_free(mpart->name) : TRUE;
  mpart->filename ? g_free(mpart->filename) : TRUE;
}

MultiPart*
multi_part_new(GSList* mpart_slist) {
  MultiPart* result = g_new0(MultiPart, 1);
  guint i = 0;
  gchar* tmpbuf = NULL;
  gchar* lastdata = NULL;

  for(i=0;i<g_slist_length(mpart_slist);i++) {
    gchar* text = g_slist_nth_data(mpart_slist, i);

    if( *text =='\0' )
      break;

    if( *text == ' ' || *text == '\t' ) {
      lastdata = g_slist_last(result->header_slist)->data;
      tmpbuf = g_strconcat(lastdata, text+1, NULL);
      g_free(lastdata);
      g_slist_last(result->header_slist)->data = tmpbuf;
      continue;
    }

    result->header_slist = g_slist_append(result->header_slist, g_strdup(text));
  }

  for(;i<g_slist_length(mpart_slist);i++) {
    gchar* text = g_slist_nth_data(mpart_slist, i);
    result->body_slist = g_slist_append(result->body_slist, g_strdup(text));
  }

  /* Some directives */
#define HDR_CTE "Content-Transfer-Encoding"
#define HDR_CT  "Content-Type"
#define HDR_CD  "Content-Disposition"

  /* Parse Header */
  tmpbuf = (gchar*)get_a_header(result->header_slist, HDR_CTE);
  if( tmpbuf && !g_ascii_strncasecmp(tmpbuf+strlen(HDR_CTE)+2,
    "base64", strlen("base64"))) {
    result->is_base64 = TRUE;
  }
  if( tmpbuf && !g_ascii_strncasecmp(tmpbuf+strlen(HDR_CTE)+2,
     "quoted-printable", strlen("quoted-printable"))) {
     result->is_qp = TRUE;
  }
  result->mimetype = get_mimetype(result->header_slist);
  result->name = get_header_property(result->header_slist, HDR_CT, "name");
  result->charset = get_header_property(result->header_slist, HDR_CT, "charset");
  result->multipart_boundary = get_header_property(result->header_slist, HDR_CT, "boundary");
  result->filename = get_header_property(result->header_slist, HDR_CD, "filename");

  /* Recurse multipart */
  if( result->multipart_boundary ) {
    GSList* newbody_slist = parse_multipart(result->body_slist, result->multipart_boundary);
    g_slist_free(result->body_slist);
    result->body_slist = newbody_slist;
    return result;
  }

  /* Base64 decode */
  if( result->name ) {
    tmpbuf = base64_decode(result->name);
    g_free(result->name);
    result->name = tmpbuf;
  }
  if( result->filename ) {
    tmpbuf = base64_decode(result->filename);
    g_free(result->filename);
    result->filename = tmpbuf;
  }

  /* Base64 Body decode - for text/plain,text/x-patch only */
  if( result->mimetype && result->is_base64
    && (!strcmp(result->mimetype, "text/plain")
    ||  !strcmp(result->mimetype, "text/x-patch")) ) {
    GSList* body_slist = NULL;
    gchar** strpp = NULL;
    gchar** strpp2 = NULL;
    gchar* strp = NULL;

    for(i=0;i<g_slist_length(result->body_slist);i++) {
      const gchar* base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      gchar* text = g_slist_nth_data(result->body_slist, i);
      gchar* buf = NULL;
      gchar* cursor = text;
      gchar* bufp = NULL;

      if( *text == '\0' )
        continue;

      buf = bufp = g_new0(gchar, strlen(text)+1);
      while( *cursor != '\0' ) {
        guchar c = 0;
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
        buf += 3;
        if( strlen(cursor) < 4 ) {
          g_warning("base64 line is not 4 multiply.\n");
          break;
        }
        cursor += 4;
      }

      body_slist = g_slist_append(body_slist, bufp);
    }

    /* Split at newline and remake body_slist */
    strpp = g_new0(gchar*, g_slist_length(body_slist)+1);
    for(i=0;i<g_slist_length(body_slist);i++) {
      strpp[i] = g_strdup(g_slist_nth_data(body_slist, i));
    }
    g_slist_free(body_slist);
    body_slist = NULL;

    strp = g_strjoinv("", strpp);
    g_strfreev(strpp);
    strpp = g_strsplit(strp, "\r\n", -1);
    g_free(strp);
    strp = g_strjoinv("\n", strpp);
    g_strfreev(strpp);
    strpp = g_strsplit(strp, "\n", -1);
    g_free(strp);
    strpp2 = strpp;
    while(*strpp != NULL) {
      //g_print("%s\n", *strpp);
      body_slist = g_slist_append(body_slist, g_strdup(*strpp++));
    }
    g_strfreev(strpp2);


    g_slist_free(result->body_slist);
    result->body_slist = body_slist;
  }

  /* Quoted Printable Body decode */
  if( result->is_qp ) {
    GSList* body_slist = NULL;
    for(i=0;i<g_slist_length(result->body_slist);i++) {
      gchar* text = g_slist_nth_data(result->body_slist, i);
      gchar* newtext = decode_quoted_printable(text);
      body_slist = g_slist_append(body_slist, newtext);
    }
    g_slist_free(result->body_slist);
    result->body_slist = body_slist;
  }

  /* Codeset Convert */
  if( result->charset ) {
    GSList* body_slist = NULL;
    for(i=0;i<g_slist_length(result->body_slist);i++) {
      gchar* text = g_slist_nth_data(result->body_slist, i);
      gchar* utf8text = u2ps_convert(text, result->charset);
      body_slist = g_slist_append(body_slist, utf8text);
    }
    g_slist_free(result->body_slist);
    result->body_slist = body_slist;
  }

  return result;
}

GSList*
parse_multipart(GSList* body_slist, gchar* multipart_boundary) {
  guint i = 0;
  GSList* mpart_slist = NULL;
  GSList* result_slist = NULL;

  for(i=0;i<g_slist_length(body_slist);i++) {
    gchar* text = g_slist_nth_data(body_slist, i);
    if( multipart_boundary && strstr(text, multipart_boundary) ) {
      if( mpart_slist ) {
        MultiPart* mpart = multi_part_new(mpart_slist);
        //dump_multi_part(mpart);
        result_slist = g_slist_concat(result_slist, print_multi_part(mpart));
        g_slist_free(mpart_slist);
        mpart_slist = NULL;
        multi_part_free(mpart);
      }
      continue;
    }
    mpart_slist = g_slist_append(mpart_slist, g_strdup(text));
  }

  if( mpart_slist )
    g_slist_free(mpart_slist);

  return result_slist;
}

gchar*
get_subject(GSList* mail_slist)
{
  gboolean sbj_start = FALSE;
  gchar* subject = g_strdup("");
  gchar* tmpbuf;
  gint i;

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
      gchar* decoded = base64_decode(buf + strlen("subject: "));
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
        gchar* decoded = base64_decode(buf+1);
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

  /* If there is other base64 part, recurse */
  while( strstr(subject, "=?") ) {
    gchar* newsbj = base64_decode(subject);
    if( !strcmp(newsbj, subject) ) {
      g_free(newsbj);
      break;
    }
    g_free(subject);
    subject = newsbj;
  }

  /* Tab to 8 spaces */
  tmpbuf = tab2spaces(subject);
  g_free(subject);
  subject = tmpbuf;

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

  /* Test */
#if 0
  for(i=0;i<g_slist_length(new_slist);i++) {
    gchar* text = g_slist_nth_data(new_slist, i);
    g_print("%s\n", text);
  }
#endif

  return new_slist;
}

void
mail_shape_header(Mail* mail) {
  guint i = 0;
  GSList* header_slist = NULL;
  const gchar* text = NULL;
  gboolean is_visible = FALSE;
  gchar* tmpbuf = NULL;
  guint j = 0;

  if( !mail )
    return;

  for(i=0;i<g_slist_length(mail->mail_slist);i++) {
    text = g_slist_nth_data(mail->mail_slist, i);

    if( *text == '\0' )
      break; /* Header end */

    if( header_slist && is_visible && (*text == ' ' || *text == '\t') ) {
      gchar* last = g_slist_last(header_slist)->data;
      if( !last ) {
        continue;
      }
      tmpbuf = g_strconcat(last, text+1, NULL);
      g_free(last);
      g_slist_last(header_slist)->data = tmpbuf;
      continue;
    }

    j = 0;
    is_visible = FALSE;
    while(worthy_headers[j] != NULL) {
      if( !g_ascii_strncasecmp(text, worthy_headers[j], strlen(worthy_headers[j])) && !strncmp(text + strlen(worthy_headers[j]), ": ", 2) ) {
        is_visible = TRUE;
        break;
      }
      j++;
    }

    if( is_visible && mail->subject
        && !g_ascii_strncasecmp(text, "Subject: ", strlen("Subject: ")) ) {
      gchar* header_subject = g_strndup(text, strlen("Subject: "));
      header_slist = g_slist_append(header_slist, g_strconcat(header_subject, mail->subject, NULL));
      g_free(header_subject);
      is_visible = FALSE;
      continue;
    }
    if( is_visible && mail->from
        && !g_ascii_strncasecmp(text, "From: ", strlen("From: ")) ) {
      gchar* header_from = g_strndup(text, strlen("From: "));
      header_slist = g_slist_append(header_slist, g_strconcat(header_from, mail->from, NULL));
      g_free(header_from);
      is_visible = FALSE;
      continue;
    }

    if( is_visible ) {
      header_slist = g_slist_append(header_slist, g_strdup(text));
    }
  }

  g_slist_free(mail->header_slist);
  mail->header_slist = header_slist;
}

Mail*
mail_new(GSList* mail_slist) {
  Mail* result = g_new0(Mail, 1);
  gchar* tmpbuf = NULL;

  result->mail_slist = mail_slist;
  result->header_slist = get_headers(mail_slist);
  result->body_slist = get_body(mail_slist);

  /* Parse Subject */
  tmpbuf = (gchar*)get_a_header(result->header_slist, "Subject");
  if( tmpbuf ) {
    result->subject = base64_decode(tmpbuf + strlen("Subject: "));
  }

  /* Parse From */
  tmpbuf = (gchar*)get_a_header(result->header_slist, "From");
  if( tmpbuf ) {
    result->from = base64_decode(tmpbuf + strlen("From: "));
  }

  /* QP check */
  tmpbuf = (gchar*)get_a_header(result->header_slist, "Content-Transfer-Encoding");
  if( tmpbuf ) {
    tmpbuf += strlen("Content-Transfer-Encoding")+2;
    if( !strncmp(tmpbuf, "quoted-printable", strlen("quoted-printable")) ) {
      result->is_qp = TRUE;
    }
    if( !strncmp(tmpbuf, "base64", strlen("base64")) ) {
      result->is_base64 = TRUE;
    }
  }

  /* Charset */
  result->charset = (gchar*)get_header_property(result->header_slist, "Content-Type", "charset");

  /* Parse mimetype */
  result->mimetype = (gchar*)get_mimetype(result->header_slist);

  /* If no charset, u2ps handle it as iso-2022-jp for 7bit mail,
   * so for multipart, it should be us-ascii.
   * Otherwise, it will convert in earlier stage.
   */
  if( !result->charset && result->mimetype
      && strstr(result->mimetype, "multipart") ) {
    result->charset = g_strdup("us-ascii");
  }

  /* Decode base64 body */
  if( result->is_base64 ) {
    guint i = 0;
    gchar* total = NULL;
    GSList* body_slist = NULL;
    gchar** strpp = NULL;
    for(i=0;i<g_slist_length(result->body_slist);i++) {
      gchar* text = g_slist_nth_data(result->body_slist, i);
      gchar* newtext = base64_decode_simple(text);
      gchar* tmpbuf = total;
      if( !total ) {
        total = g_strdup(newtext);
      } else {
        total = g_strconcat(tmpbuf, newtext, NULL);
      }
      g_free(newtext);
      g_free(tmpbuf);
    }

    strpp = g_strsplit(total, "\n", -1);
    for(i=0;strpp[i] != NULL;i++) {
      body_slist = g_slist_append(body_slist, g_strdup(strpp[i]));
    }
    g_strfreev(strpp);
    g_free(total);
    
    g_slist_free(result->body_slist);
    result->body_slist = body_slist;
  }
  /* Decode quoted-printing body */
  if( result->is_qp ) {
    guint i = 0;
    GSList* body_slist = NULL;
    for(i=0;i<g_slist_length(result->body_slist);i++) {
      gchar* text = g_slist_nth_data(result->body_slist, i);
      gchar* newtext = decode_quoted_printable(text);
      body_slist = g_slist_append(body_slist, newtext);
    }
    g_slist_free(result->body_slist);
    result->body_slist = body_slist;
  }

  /* Convert body to UTF-8 */
  if( result->charset && (!result->mimetype || !strstr(result->mimetype, "multipart")) ) {
    guint i = 0;
    GSList* utf8_slist = NULL;
    for(i=0;i<g_slist_length(result->body_slist);i++) {
      gchar* text = g_slist_nth_data(result->body_slist, i);
      gchar* utf8 = u2ps_convert(text, result->charset);
      utf8_slist = g_slist_append(utf8_slist, utf8);
    }
    g_slist_free(result->body_slist);
    result->body_slist = utf8_slist;
  }

  /* Parse Multipart */
  result->multipart_boundary = (gchar*)get_header_property(result->header_slist, "Content-Type", "boundary");
  if( result->multipart_boundary ) {
    GSList* new_slist = parse_multipart(result->body_slist, result->multipart_boundary);
    g_slist_free(result->body_slist);
    result->body_slist = new_slist;
  }

  return result;
}

void
dump_mail(Mail* mail) {
  guint i = 0;

  if( !mail ) {
    g_print("mail is NULL\n");
    return;
  }

  for(i=0;i<g_slist_length(mail->header_slist);i++) {
    gchar* text = g_slist_nth_data(mail->header_slist, i);
    g_print("HEADER: %s\n", text);
  }

  /* Body part dump is very verbose */

  g_print("FROM: %s\n", mail->from);
  g_print("SUBJECT: %s\n", mail->subject);
  g_print("MIMETYPE: %s\n", mail->mimetype);
  g_print("IS_BASE64: %s\n", mail->is_base64 ? "TRUE": "FALSE");
  g_print("IS_QP: %s\n", mail->is_qp ? "TRUE" : "FALSE");
  g_print("MULTIPART_BOUNDARY: %s\n", mail->multipart_boundary );

  /* mail->multipart_slist dump here */
}

void
mail_free(Mail* mail) {
  guint i = 0;

  if( !mail )
    return;

  g_slist_free(mail->header_slist);
  g_slist_free(mail->body_slist);

  g_free(mail->from);
  g_free(mail->subject);
  g_free(mail->mimetype);
  g_free(mail->charset);

  for(i=0;i<g_slist_length(mail->multipart_slist);i++) {
    MultiPart* mpart = g_slist_nth_data(mail->multipart_slist, i);
    multi_part_free(mpart);
    g_slist_nth(mail->multipart_slist, i)->data = NULL;
  }
  g_slist_free(mail->multipart_slist);
  g_free(mail);
}
