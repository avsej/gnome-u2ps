/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * util.c
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

#include "util.h"

typedef struct _iso2022jp_pair {
  gchar* from;
  gunichar to;
} iso2022jp_pair;

static iso2022jp_pair iso2022jp_table[] = {
  { "\x2d\x21", 0x2460 }, /* CIRCLED DIGIT ONE */
  { "\x2d\x22", 0x2461 }, /* CIRCLED DIGIT TWO */
  { "\x2d\x23", 0x2462 }, /* CIRCLED DIGIT THREE */
  { NULL, 0 },
};

/* u2ps_iso2022jp_to_utf8() converts iso-2022-jp char which
   is not recognized by iconv */
gchar*
u2ps_iso2022jp_to_utf8(gchar* str)
{ 
  GError* conv_error = NULL;
  gsize bytes_read, bytes_written;
  gchar* result = NULL;
  gint i;
                                                                                
  g_return_val_if_fail(str != NULL, NULL);
  g_return_val_if_fail(*str != '\0', g_strdup(""));
                                                                                
  result = g_convert(str, -1, "UTF-8", "ISO-2022-JP", &bytes_read, &bytes_written, &conv_error);

  if( conv_error ) {
    if( result ) {
      g_free(result);
    }

    i=0;
    while(iso2022jp_table[i].from != NULL) {
      if( !strncmp(str + bytes_read, iso2022jp_table[i].from, strlen(iso2022jp_table[i].from)) ) {
        str[bytes_read] = '\x24'; /* Hiragana A */
        str[bytes_read+1] = '\x22';
        result = u2ps_iso2022jp_to_utf8(str);
        g_unichar_to_utf8(iso2022jp_table[i].to, result+bytes_written);
        g_error_free(conv_error);
        conv_error = NULL;
        break;
      }
      i++;
    }
  }
  if( conv_error ) {
    g_warning("%s\n", conv_error->message);
    debug_dump(str+bytes_read);
    g_error_free(conv_error);
    return NULL;
  }
                                                                                
  return result;
}

/* Wrapper for g_convert() */
gchar*
u2ps_convert(gchar* str, gchar* codeset) {
  GError* conv_error = NULL;
  gsize bytes_read, bytes_written;
  gchar* result = NULL;

  g_return_val_if_fail(str != NULL, NULL);
  //g_return_val_if_fail(*str != '\0', g_strdup(""));
  if( *str == '\0' )
    return g_strdup("");

  result = g_convert(str, -1, "UTF-8", codeset, &bytes_read, &bytes_written, &conv_error);

  if( conv_error ) {
    if( result ) {
      g_free(result);
    }

    if( !g_ascii_strcasecmp(codeset, "ISO-2022-JP") ) {
      g_error_free(conv_error);
      result = u2ps_iso2022jp_to_utf8(str);
      return result;
    }

    g_warning("%s\n", conv_error->message);
    g_error_free(conv_error);
    return NULL;
  }

  return result;
}

gchar*
tab2spaces(gchar* str) {
  gint i;
  gchar* tmpbuf;

  if( str == NULL )
    return NULL;

  tmpbuf = g_strdup(str);

  if( strstr(tmpbuf, "\t") ) {
    gchar** tmpbufpp = g_strsplit(tmpbuf, "\t", -1);
    gchar* newbuf = g_strjoinv("        ", tmpbufpp);
    g_strfreev(tmpbufpp);
    g_free(tmpbuf);
    tmpbuf = newbuf;
  }

  return tmpbuf;
}

void
debug_dump(guchar* str)
{
  gint i;

  for(i=0;i<16;i++) {
    if( str[i] == '\0')
      break;

    if( i != 0 )
      g_print(" ");

    if( i == 8 )
      g_print(" ");

    g_print("%X", str[i]);
  }

  g_print("\n");
}
