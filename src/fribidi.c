/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * fribidi.c
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
#include "fribidi.h"

GSList*
parse_fribidi(GSList* text_slist, GSList** rtl_slist_p) {
  guint i = 0;
  glong bytes_read, bytes_written;
  GSList* new_slist = NULL;
  GSList* rtl_slist = *rtl_slist_p;

  g_return_val_if_fail(text_slist != NULL, NULL);

  for(i=0;i<g_slist_length(text_slist);i++) {
    gunichar* ucs4buf = NULL;
    gchar* newtext = NULL;
    FriBidiChar *visual = NULL;
    FriBidiCharType base = FRIBIDI_TYPE_ON;
    gchar* text = g_slist_nth_data(text_slist, i);

    ucs4buf = g_utf8_to_ucs4(text, -1, &bytes_read, &bytes_written, NULL);
    visual = g_new0(FriBidiChar, bytes_written+1);
    fribidi_log2vis((FriBidiChar*)ucs4buf, bytes_written, &base, visual, NULL, NULL, NULL);
    newtext = g_ucs4_to_utf8((gunichar*)visual, bytes_written, NULL, NULL, NULL);
    g_free(ucs4buf);
    g_free(visual);
    new_slist = g_slist_append(new_slist, newtext);
    rtl_slist = g_slist_append(rtl_slist, (gpointer)(FRIBIDI_IS_RTL(base)));
  }

  g_assert(g_slist_length(new_slist) == g_slist_length(rtl_slist));

  *rtl_slist_p = rtl_slist;

  return new_slist;
}
