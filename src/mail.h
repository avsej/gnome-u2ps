/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * mail.h 
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

#ifndef _MAIL_H

#include <glib.h>

typedef struct _Mail {
  GSList* mail_slist; /* Original Mail */
  GSList* header_slist;
  GSList* body_slist;

  gchar* from;
  gchar* subject;
  gchar* mimetype;
  gchar* charset;
  gboolean is_base64;
  gboolean is_qp;
  gchar* multipart_boundary;
  GSList* multipart_slist;
} Mail;

typedef struct _MultiPart {
  GSList* header_slist;
  GSList* body_slist;

  gchar* mimetype;
  gchar* charset;
  gchar* name; /* Name property in 'Content-Type:' header line */
  gchar* filename; /* Filename property in 'Content-Disposition: ' header */
  gboolean is_base64;
  gboolean is_qp; /* Quoted Printable Format */
  gchar* multipart_boundary;
  GSList* multipart_slist;
} MultiPart;

/* Generic Mail func */
Mail* mail_new(GSList* mail_slist);
void dump_mail(Mail* mail);
void mail_free(Mail* mail);
void mail_shape_header(Mail* mail);

/* Multipart funcs */
GSList* parse_multipart(GSList* body_slist, gchar* multipart_boundary);

/* Mail handling funcs */
gchar* get_mimetype(GSList* header_slist);
GSList* get_headers(GSList* mail_slist);
GSList* get_body(GSList* mail_slist);
gchar* get_charset(GSList* mail_slist);
gchar* get_subject(GSList* mail_slist);
GSList* decode_qp_message(GSList* text_slist);

#endif /* _MAIL_H */
