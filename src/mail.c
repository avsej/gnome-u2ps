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
    return;

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
      result_slist = g_slist_append(result_slist, g_strdup_printf("[Inline Attachment: %s %s]", name, mimetype));
    } else if( mpart->mimetype && !strcmp(mpart->mimetype, "application/pgp-signature") ) {
      /* Do Nothing */
    } else {
      result_slist = g_slist_append(result_slist, g_strdup_printf("[Attachment: %s %s]", name, mimetype));
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
  GSList* header_slist = NULL;

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
#define TRANSFER_ENCODING "Content-Transfer-Encoding: "
#define CONTENT_TYPE "Content-Type: "
#define CONTENT_DISPOSITION "Content-Disposition: "

  /* Parse Header */
  header_slist = result->header_slist;
  for(i=0;i<g_slist_length(header_slist);i++) {
    gchar* text = g_slist_nth_data(header_slist, i);
    if( !g_ascii_strncasecmp(text, TRANSFER_ENCODING, strlen(TRANSFER_ENCODING) ) ) {
      text += strlen(TRANSFER_ENCODING);
      if( !g_ascii_strncasecmp(text, "base64", strlen("base64")) ) {
        result->is_base64 = TRUE;
        continue;
      }
      if( !g_ascii_strncasecmp(text, "quoted-printable", strlen("quoted-printable")) ) {
        result->is_qp = TRUE;
      }
      continue; /* Go to the next header */
    }
    if( !g_ascii_strncasecmp(text, CONTENT_TYPE, strlen(CONTENT_TYPE)) ) {
      text += strlen(CONTENT_TYPE);
      /* Get the mimetype */
      tmpbuf = strchr(text, ';');
      if( tmpbuf ) {
        result->mimetype = g_strndup(text, tmpbuf - text);
      } else {
        result->mimetype = g_strdup(text);
        continue; /* No need to parse more */
      }
      /* Get its name */
      tmpbuf = strstr(text, "name=");
      if( tmpbuf ) {
        gchar* semicolon = NULL;
        gchar* dquote = NULL;
        tmpbuf += strlen("name=");
        if( *tmpbuf == '"' )
          tmpbuf++;
        semicolon = strchr(tmpbuf, ';');
        dquote = strchr(tmpbuf, '"');
        if( !semicolon && !dquote ) {
          result->name = g_strdup(tmpbuf);
          continue; /* No more property */
        } else if ( semicolon && dquote ) {
          result->name = g_strndup(tmpbuf, MIN(semicolon, dquote)-tmpbuf);
        } else if( semicolon ) {
          result->name = g_strndup(tmpbuf, semicolon-tmpbuf);
        } else if( dquote ) {
          result->name = g_strndup(tmpbuf, dquote-tmpbuf);
        }
      }
      /* Get its charset */
      tmpbuf = strstr(text, "charset=");
      if( tmpbuf ) {
        gchar* semicolon = NULL;
        gchar* dquote = NULL;
        tmpbuf += strlen("charset=");
        if( *tmpbuf == '"' )
          tmpbuf++;
        semicolon = strchr(tmpbuf, ';');
        dquote = strchr(tmpbuf, '"');
        if( !semicolon && !dquote ) {
          result->charset = g_strdup(tmpbuf);
          continue; /* No more property */
        } else if ( semicolon && dquote ) {
          result->charset = g_strndup(tmpbuf, MIN(semicolon, dquote)-tmpbuf);
        } else if( semicolon ) {
          result->charset = g_strndup(tmpbuf, semicolon-tmpbuf);
        } else if( dquote ) {
          result->charset = g_strndup(tmpbuf, dquote-tmpbuf);
        }
      }
      continue; /* Go to the next header */
    }
    if( !g_ascii_strncasecmp(text, CONTENT_DISPOSITION, strlen(CONTENT_DISPOSITION)) ) {
      text += strlen(CONTENT_DISPOSITION);
      /* Get its filename */
      tmpbuf = strstr(text, "filename=");
      if( tmpbuf ) {
        gchar* semicolon = NULL;
        gchar* dquote = NULL;
        tmpbuf += strlen("filename=");
        if( *tmpbuf == '"' )
          tmpbuf++;
        semicolon = strchr(tmpbuf, ';');
        dquote = strchr(tmpbuf, '"');
        if( !semicolon && !dquote ) {
          result->filename = g_strdup(tmpbuf);
          continue; /* No more property */
        } else if ( semicolon && dquote ) {
          result->filename = g_strndup(tmpbuf, MIN(semicolon, dquote)-tmpbuf);
        } else if( semicolon ) {
          result->filename = g_strndup(tmpbuf, semicolon-tmpbuf);
        } else if( dquote ) {
          result->filename = g_strndup(tmpbuf, dquote-tmpbuf);
        }
      }
      continue; /* Go to the next header */
    }
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
//        dump_multi_part(mpart);
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
