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
