/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * main.c
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
#include <string.h>
#include <locale.h>
#include <time.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnome/libgnome.h>

static int show_version = FALSE;

static struct poptOption const
u2ps_popt_options[] = {
  { "version", 'v', POPT_ARG_NONE, &show_version, 0,
    "Display u2ps version", NULL },
  POPT_AUTOHELP
  POPT_TABLEEND
};

static GnomeProgram *program;

const struct {
  const gchar* prefix;
  const gchar* fontname;
} localefont[] = {
  { "ja_JP", "Kochi Gothic" },
  { "ko_KR", "Baekmuk Batang" },
  { "zh_CN", "AR PL KaitiM GB" },
  { "zh_TW", "AR PL KaitiM Big5" },
  { "zh_HK", "AR PL KaitiM Big5" },
  { NULL, NULL }
};

gchar*
g_utf8_strndup(gchar* utf8text, gint n)
{
  gchar* result;

  gint len = g_utf8_strlen(utf8text, -1);

  if( n < len && n > 0 )
    len = n;

  result = g_strndup(utf8text, g_utf8_offset_to_pointer(utf8text, len) - utf8text);

  return result;
}

GSList*
enable_hyphenation(GSList* text_slist, GnomeFont* font, gdouble maxw)
{
  guint i;
  GSList* new_slist = NULL;

  g_return_val_if_fail(font != NULL, text_slist);

  for(i=0;i<g_slist_length(text_slist);i++)
  {
    gchar* text = g_slist_nth_data(text_slist, i);
    gdouble textw = gnome_font_get_width_utf8(font, text);
    if( textw > maxw ) {
      gchar* cursor = text;
      while(gnome_font_get_width_utf8(font, cursor) > maxw) {
        gint j;
        for(j=1;j<=g_utf8_strlen(cursor, -1);j++) {
          gchar* newtext = g_utf8_strndup(cursor, j);
          gdouble newtextw = gnome_font_get_width_utf8(font, newtext);
          g_free(newtext);
          if( newtextw > maxw ) {
            g_assert(j > 1);
            gchar* newtext2 = g_utf8_strndup(cursor, j-1);
            new_slist = g_slist_append(new_slist, newtext2);
            cursor = g_utf8_offset_to_pointer(cursor, j-1);
            break;
          }
        }
      }
      new_slist = g_slist_append(new_slist, g_strdup(cursor));
    } else {
      new_slist = g_slist_append(new_slist, text);
    }
  }

  g_slist_free(text_slist);

  return new_slist;
}

void
draw_pageframe_single(GnomePrintContext* context, GnomeFontFace* face, gdouble xoffset, gdouble pagew, gdouble pageh, gchar* datestr, gchar* title, gint nthpage, gint maxpage) {

  gdouble fontheight = 0;
  gdouble lineheight = 0;
  gdouble titlew = 0;
  GnomeFont* font;
  gchar* pagenum;
  gdouble pagenumw;

  gnome_print_setlinewidth(context, 0.5);

  gnome_print_rect_stroked(context, xoffset+30, 40, pagew-40, pageh-80);
  gnome_print_line_stroked(context, xoffset+30, pageh-60, xoffset+20+pagew-30, pageh-60);

  /* Header Font */
  font = gnome_font_face_get_font_default(face, 14);
  g_assert(font != NULL);
  gnome_print_setfont(context, font);

  /* Header Font Height */
  fontheight = gnome_font_get_ascender(font) + gnome_font_get_descender(font);
  lineheight = fontheight + 5;

  /* Header Date */
  gnome_print_moveto(context, xoffset +30 +10, pageh - 55.0);
  gnome_print_show(context, datestr);

  titlew = gnome_font_get_width_utf8(font, title);
  gnome_print_moveto(context, xoffset +30 + (pagew-30)/2 - titlew/2, pageh - 55.0);
  gnome_print_show(context, title);

  pagenum = g_strdup_printf(_("%d/%d Pages"), nthpage, maxpage);
  pagenumw = gnome_font_get_width_utf8(font, pagenum);
  gnome_print_moveto(context, xoffset +30 + (pagew-30) - pagenumw - 30, pageh - 55.0);
  gnome_print_show(context, pagenum);
  g_free(pagenum);
}

void
draw_pageframe(GnomePrintContext* context, GnomeFontFace* face, gdouble xoffset, gdouble pagew, gdouble pageh, gchar* title, gint nthpage, gint maxpage) {
  gchar datestr[1024];
  time_t now_t = time(NULL);
  strftime(datestr, 1000, _("%B %e %Y %H:%M"), localtime(&now_t));

  draw_pageframe_single(context, face, xoffset, pagew, pageh, datestr, title, nthpage, maxpage);
}

int main(int argc, char** argv) {
  GnomePrintJob* job;
  GnomePrintContext* context;
  GnomePrintConfig* config;
  GnomeFont* font;
  GnomeFontFace* face;
  gdouble fontheight = 0;
  gdouble lineheight = 0;
  gdouble pagew = 1.0;
  gdouble pageh = 0;
  FILE* fp;
  GSList* text_slist = NULL; /* Text list */
  gchar buf[1024];
  guint pages = 1;
  gint i=0;
  gint maxlines = 0;
  guint nthline = 0;
  const gchar* familyname = "Kochi Gothic"; /* Fallback set */
  const gchar* locale;
  GValue value = { 0, };
  poptContext ctx;
  gchar **popt_args;

  locale = setlocale(LC_ALL, "");

#ifdef ENABLE_NLS
  bindtextdomain(PACKAGE, U2PS_LOCALEDIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  textdomain(PACKAGE);
#endif

  g_type_init();

  program = gnome_program_init(PACKAGE, VERSION,
			       LIBGNOME_MODULE,
			       argc, argv,
			       GNOME_PARAM_POPT_TABLE, u2ps_popt_options,
			       NULL);

  if( show_version ) {
    g_print("Copyright (C) 2004 Yukihiro Nakai <nakai@gnome.gr.jp>\n");
    g_print(_("%s version is %s\n"), PACKAGE, VERSION);
    return 0;
  }

  g_value_init(&value, G_TYPE_POINTER);
  g_object_get_property(G_OBJECT(program), GNOME_PARAM_POPT_CONTEXT, &value);
  ctx = g_value_get_pointer(&value);
  g_value_unset(&value);

  popt_args = (char**) poptGetArgs(ctx);

  if( popt_args ) {
    for(i=0;popt_args[i];i++) {
      g_print("%s\n", popt_args[i]);
    }
  }
  exit(0);

  for(i=0;localefont[i].prefix != NULL;i++) {
    if( !strncmp(locale, localefont[i].prefix, strlen(localefont[i].prefix)) ) {
      familyname = localefont[i].fontname;
      break;
    }
  }

  /* Read the Input Text */
  fp = fopen("test.txt", "r");
  if( !fp ) {
    g_error(_("File is not found.\n"));
  }
  while(fgets(buf, 1024, fp) > 0 ) {
    const gchar* end;
    gchar* tmpbuf = g_strdup(buf);
    if( tmpbuf[strlen(tmpbuf)-1] == '\n' )
      tmpbuf[strlen(tmpbuf)-1] = '\0';
    text_slist = g_slist_append(text_slist, tmpbuf);
    if( !g_utf8_validate(tmpbuf, -1, &end) ) {
      g_error("File is not valid UTF-8\n");
    }
  }
  fclose(fp);

  /* Prepare Printing */
  job = gnome_print_job_new(NULL);
  context = gnome_print_job_get_context(job);
  config = gnome_print_job_get_config(job);

  gnome_print_config_set(config, "Printer", "GENERIC");
  gnome_print_job_print_to_file(job, "output.ps");
  gnome_print_config_set(config, GNOME_PRINT_KEY_PAPER_SIZE, "A4");
  gnome_print_config_set(config, GNOME_PRINT_KEY_PAGE_ORIENTATION, "R0");
  gnome_print_config_set(config, GNOME_PRINT_KEY_PAPER_ORIENTATION, "R180");
  gnome_print_config_set(config, GNOME_PRINT_KEY_DUPLEX, "false");
  gnome_print_config_set(config, GNOME_PRINT_KEY_LAYOUT, "2_1");
  gnome_print_config_get_page_size(config, &pagew, &pageh);

  gnome_print_beginpage(context, "1");

  face = gnome_font_face_find_closest_from_weight_slant(familyname, GNOME_FONT_MEDIUM, FALSE);
  g_assert(face != NULL);

  /* Text Font */
  font = gnome_font_face_get_font_default(face, 12);
  g_assert(face != NULL);
  gnome_print_setfont(context, font);
  /* Text Font Height */
  fontheight = gnome_font_get_ascender(font) + gnome_font_get_descender(font);
  lineheight = fontheight + 5;

  text_slist = enable_hyphenation(text_slist, font, pagew-40-20);

  /* Text max lines per page */
  maxlines = (pageh -40 -40 -20) / lineheight - 1;

  /* Max page */
  pages = g_slist_length(text_slist)/maxlines;
  if( g_slist_length(text_slist) > maxlines * pages )
    pages++;

  nthline = 0;
  for(i=0;i<pages;i++) {
    guint j;
    gdouble xoffset = (i % 2) == 0 ? 0 : -20;

    if( i != 0) {
      gchar* pagestr = g_strdup_printf("%d", i/2+1);
      gnome_print_beginpage(context, pagestr);
      g_free(pagestr);
    }

    for(j=0;j<maxlines;j++) {
      gchar* text;

      if( j+nthline >= g_slist_length(text_slist) )
        break;

      text = g_slist_nth_data(text_slist, j+nthline);
      gnome_print_moveto(context, xoffset + 40, pageh -40 -20 -lineheight*(j+1));
      gnome_print_show(context, text);
    }
    nthline += j;
    draw_pageframe(context, face, xoffset, pagew, pageh, _("Test Title"), i+1, pages);
    gnome_print_showpage(context);
  }

  gnome_print_end_doc(context);
  gnome_print_context_close(context);

  g_object_unref(G_OBJECT(font));

  gnome_print_job_close(job);
  gnome_print_job_print(job);

  g_object_unref(G_OBJECT(config));
  g_object_unref(G_OBJECT(context));
  g_object_unref(G_OBJECT(job));

  return 0;
}
