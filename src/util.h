/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * util.h 
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

#ifndef _UTIL_H

#include <glib.h>

gchar* u2ps_convert(gchar* str, gchar* codeset);
gchar* tab2spaces(gchar* str);
gchar* u2ps_iso2022jp_to_utf8(gchar* str);
void debug_dump(guchar* str);
void dump_text_slist(GSList* text_slist);

/* Arabic */
GSList* shape_arabic(GSList* text_slist);
GSList* parse_fribidi(GSList* text_slist, GSList** rtl_slist_p);

#endif /* _UTIL_H */
