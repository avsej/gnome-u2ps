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

/* ISO-2022-JP <-> UCS2 supplimentary code table
 *
 * Most are assumed to be glibc code mapping failure (but fixable)
 * 'NOT WORK' means gs/libgnomeprint have the fault besides glibc
 */
static iso2022jp_pair iso2022jp_table[] = {
  { "\x2d\x21", 0x2460 }, /* CIRCLED DIGIT 1 */
  { "\x2d\x22", 0x2461 }, /* CIRCLED DIGIT 2 */
  { "\x2d\x23", 0x2462 }, /* CIRCLED DIGIT 3 */
  { "\x2d\x24", 0x2463 }, /* CIRCLED DIGIT 4 */
  { "\x2d\x25", 0x2464 }, /* CIRCLED DIGIT 5 */
  { "\x2d\x26", 0x2465 }, /* CIRCLED DIGIT 6 */
  { "\x2d\x27", 0x2466 }, /* CIRCLED DIGIT 7 */
  { "\x2d\x28", 0x2467 }, /* CIRCLED DIGIT 8 */
  { "\x2d\x29", 0x2468 }, /* CIRCLED DIGIT 9 */
  { "\x2d\x2a", 0x2469 }, /* CIRCLED DIGIT 10 */
  { "\x2d\x2b", 0x246A }, /* CIRCLED DIGIT 11 */
  { "\x2d\x2c", 0x246B }, /* CIRCLED DIGIT 12 */
  { "\x2d\x2d", 0x246C }, /* CIRCLED DIGIT 13 */
  { "\x2d\x2e", 0x246D }, /* CIRCLED DIGIT 14 */
  { "\x2d\x2f", 0x246E }, /* CIRCLED DIGIT 15 */
  { "\x2d\x30", 0x246F }, /* CIRCLED DIGIT 16 */
  { "\x2d\x31", 0x2470 }, /* CIRCLED DIGIT 17 */
  { "\x2d\x32", 0x2471 }, /* CIRCLED DIGIT 18 */
  { "\x2d\x33", 0x2472 }, /* CIRCLED DIGIT 19 */
  { "\x2d\x34", 0x2473 }, /* CIRCLED DIGIT 20 */
  { "\x2d\x35", 0x2160 }, /* ROMAN NUMERAL ONE */ /* NOT WORK */
  { "\x2d\x4a", 0x330D }, /* SQUARE KARORII */ /* NOT WORK */
  { "\x2d\x6a", 0x3231 }, /* PARENTHESIZED IDEOGRAPH STOCK */
  { "\x2d\x4B", 0x3326 }, /* SQUARE DORU */ /* NOT WORK */
  { "\x2d\x45", 0x3327 }, /* SQUARE TON */ /* NOT WORK */
  { "\x7c\x71", 0x2170 }, /* SMALL ROMAN NUMERAL ONE */ /* NOT WORK */
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
        g_assert(result != NULL);
        if( g_unichar_to_utf8(iso2022jp_table[i].to, result+bytes_written) != 3) {
          g_error("unichar is not 3 byte utf8");
        }
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

  if( !g_ascii_strcasecmp(codeset, "X-UNKNOWN") ) {
    result = g_convert(str, -1, "UTF-8", "us-ascii", &bytes_read, &bytes_written, &conv_error);
  } else {
    result = g_convert(str, -1, "UTF-8", codeset, &bytes_read, &bytes_written, &conv_error);
  }

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
  gint i = 0;
  gchar* tmpbuf = NULL;

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
