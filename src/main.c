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
#include <libgen.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnome/libgnome.h>

#include "mail.h"
#include "util.h"

static gboolean show_version = FALSE;
static gboolean parse_mail = FALSE;
char const *output_filename = NULL;
char const *input_encoding = NULL;
char const *familyname = NULL; /* Fallback set */

#define DEFAULT_FAMILY_NAME "Luxi Sans"

#define g_str_is_euckr(str) g_str_is_eucjp(str)

static struct poptOption const
u2ps_popt_options[] = {
  { "version", 'v', POPT_ARG_NONE, &show_version, 0,
    N_("Display u2ps version"), NULL },
  { "output", 'o', POPT_ARG_STRING, &output_filename, 0,
    N_("Specify output filename"), N_("FILE")},
  { "encoding", 'X', POPT_ARG_STRING, &input_encoding, 0,
    N_("Encoding of the input"), N_("ENCODING") },
  { "gpfamily", '\0', POPT_ARG_STRING, &familyname, 0,
    N_("Specify libgnomeprint font family name"), N_("FAMILYNAME") },
  { "mail", '\0', POPT_ARG_NONE, &parse_mail, 0,
    N_("Parse input file as mail"), NULL },
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
  { "ar", "KacstBook" },
  { NULL, NULL }
};

void
u2ps_show_version()
{
    g_print("Copyright (C) 2004 Yukihiro Nakai <nakai@gnome.gr.jp>\n");
    g_print(_("%s version is %s\n"), PACKAGE, VERSION);
}

/* g_str_is_big5()
   if text is Big5  --> returns TRUE
   if text is ascii --> returns TRUE
   if text is UTF-8,etc --> returns FALSE

   Big5 code mapping:
    1st byte: 0xA1-0xC6, 0xC9-0xF9
    2nd byte: 0x40-0x7E, 0xA1-0xFE
*/
gboolean
g_str_is_big5(gchar* str) {
  gint shift = 0;
  gint i;
  guchar* text = (guchar*)str;

  if( text == NULL || *text == '\0' )
    return TRUE;

  for(i=0;i<strlen(text);i++) {
    if( shift == 0 && (text[i] & 0x80) ) {
      if( text[i] < 0xA1 || (text[i] > 0xC6 && text[i] < 0xC9) || text[i] > 0xF9) {
        /* Outside of Big5 mapping area */
        return FALSE;
      }
      shift++;
    } else { /* shift = 1 */
      if( text[i] < 0x40 || (text[i] > 0x7E && text[i] < 0xA1) || text[i] > 0xFE) {
        /* Outside of Big5 mapping 2nd byte condition */
        return FALSE;
      }
      shift = 0;
    }
  }

  return TRUE;
}

/* g_str_is_gb18030()
   if text is GB18030  --> returns TRUE
   if text is ascii    --> returns TRUE
   if text is UTF-8,etc --> returns FALSE

   GB18030 code mapping:
   1byte area: 0x00-0x7F

   2bytes area:
    1st: 0x81-0xFE
    2nd: 0x40-0x7E,0x80-0xFE

   4bytes area:
    1st: 0x81-0xFE
    2nd: 0x30-0x39
    3rd: 0x81-0xFE
    4th: 0x30-0x39
*/
gboolean
g_str_is_gb18030(gchar* str) {
  gint shift = 0;
  gint i;
  guchar* text = (guchar*)str;

  if( text == NULL || *text == '\0' )
    return TRUE;

  for(i=0;i<strlen(text);i++) {
    if( shift == 3 ) {
      if ( text[i] >= 0x30 && text[i] <= 0x39 ) {
        shift = 0; /* Clear */
        continue;
      }
      return FALSE;
    }

    /* 3rd byte check */
    if( shift == 2 ) {
      if( text[i] >= 0x81 && text[i] <= 0xFE ) {
        shift = 3;
        continue;
      }
      return FALSE;
    }

    /* 2nd byte check */
    if( shift == 1 ) {
      if( (text[i] >= 0x40 && text[i] <= 0x7E) || (text[i] >= 0x80 && text[i] <= 0xFE) ) {
        shift = 0;
        continue;
      } else if ( text[i] >= 0x30 && text[i] <= 0x39 ) {
        shift = 2;
        continue;
      }
      return FALSE;
    }

    /* 1st byte check */
    if( text[i] < 0x7F )
      continue;

    if( text[i] >= 0x81 && text[i] <= 0xFE ) {
      shift = 1;
      continue;
    }

    /* Others are not Big5 */
    return FALSE;
  }

  return TRUE;
}

/* g_str_is_eucjp()
   if text is eucjp --> returns TRUE
   if text is ascii --> returns TRUE
   if text is UTF-8,etc --> returns FALSE

   EUC-JP code mapping:
     1st byte: 0xA1-0xFE
     2nd byte: 0xA1-0xFE
*/
gboolean
g_str_is_eucjp(gchar* str) {
  gint shift = 0;
  gint i;
  guchar* text = (guchar*)str;

  if( text == NULL || *text == '\0' )
    return TRUE;

  for(i=0;i<strlen(text);i++) {
    if( shift == 1 ) {
      if( text[i] < 0xA1 || text[i] > 0xFE ) {
        return FALSE;
      } else {
        shift = 0;
        continue;
      }
    }
    if( text[i] & 0x80 ) {
      if( text[i] < 0xA1 || text[i] > 0xFE ) {
        /* Outside of EUC-JP mapping area */
        return FALSE;
      }
      shift = 1;
    }
  }

  return TRUE;
}

/* g_str_is_sjis()
   if text is sjis --> returns TRUE
   if text is ascii --> returns TRUE
   if text is UTF-8,etc --> returns FALSE
*/
gboolean
g_str_is_sjis(gchar* str) {
  gint shift = 0;
  gint i;
  guchar* text = (guchar*)str;

  if( text == NULL || *text == '\0' )
    return TRUE;

  for(i=0;i<strlen(text);i++) {
    guchar c = text[i];
    if( shift == 0 ) {
      if( ((c>=0x81 && c<=0x9F) || ( c>=0xE0 && c<=0xEF)) ) {
        shift = 1;
        continue;
      }
      if( c & 0x80 ) {
        if( c>=0xA1 && c<=0xDF ) {
          continue;
        } else {
          return FALSE;
        }
      }
    }
    if( shift == 1 ) {
      if( (c>=0x40 && c<=0x7E) || (c>=0x80 && c<=0xFC) ) {
        shift = 0;
      } else {
        return FALSE;
      }
    }
  }

  return TRUE;
}

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

/* get_unbreakable() returns the unbreakable boolean in char position.
   The idea is very simple and similar to Pango's.
   The input string should valid UTF-8.
   The input string should not have newline char.
*/
gboolean*
get_unbreakable(gchar* text) {
  gboolean* unbreakable = NULL;
  gboolean* breakable = NULL;
  gint i;

  g_return_val_if_fail(text != NULL, NULL);

  /* Check breakable first for European languages */
  breakable = g_new0(gboolean, g_utf8_strlen(text, -1)+1);

  for(i=0;i<g_utf8_strlen(text, -1);i++) {
    gunichar c = g_utf8_get_char(g_utf8_offset_to_pointer(text, i));
    switch(c) {
      case ' ':
      case '\t':
      case ',':
      case '.':
      case '-':
        breakable[i] = TRUE;
        breakable[i+1] = TRUE;
      break;
    default:
      break;
    }
  }

  /* Check unbreakable */
  unbreakable = g_new0(gboolean, g_utf8_strlen(text, -1)+1);
  for(i=0;i<g_utf8_strlen(text, -1);i++) {
    unbreakable[i] = !breakable[i]; /* Set default */
  }

  g_free(breakable);
  return unbreakable;
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
            gchar* newtext2;
            gboolean* unbreakable;
            g_assert(j > 1);
            unbreakable = get_unbreakable(cursor);
            if( unbreakable[j] ) {
              gint k;
              for(k=j;k>1;k--) {
                if( unbreakable[k] == FALSE ) {
                  j = k+1;
                  break;
                }
              }
              /* Do nothing when all is unbreakable */
            }
            g_free(unbreakable);
            newtext2 = g_utf8_strndup(cursor, j-1);
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
  gdouble datew = 0;
  GnomeFont* font;
  gchar* pagenum;
  gdouble pagenumw;

  gnome_print_setlinewidth(context, 0.5);

  gnome_print_setrgbcolor(context, 0.95, 0.95, 0.95);
  gnome_print_rect_filled(context, xoffset+30, pageh-60, pagew-40, 20);
  gnome_print_setrgbcolor(context, 0, 0, 0);
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
  datew = gnome_font_get_width_utf8(font, datestr);
  if( !parse_mail ) {
    gnome_print_moveto(context, xoffset +30 +10, pageh - 55.0);
    gnome_print_show(context, datestr);
  }

  /* Header Page */
  pagenum = g_strdup_printf(_("%d/%d Pages"), nthpage, maxpage);
  pagenumw = gnome_font_get_width_utf8(font, pagenum);
  gnome_print_moveto(context, xoffset +30 + (pagew-30) - pagenumw - 30, pageh - 55.0);
  gnome_print_show(context, pagenum);
  g_free(pagenum);

  /* Header Title */
  titlew = gnome_font_get_width_utf8(font, title);
  if( parse_mail ) {
    //gnome_print_clip(context);
    gnome_print_newpath(context);
    gnome_print_moveto(context, xoffset, 0);
    gnome_print_lineto(context, xoffset +(pagew-30) -pagenumw ,0);
    gnome_print_lineto(context, xoffset +(pagew-30) -pagenumw ,pageh);
    gnome_print_lineto(context, xoffset, pageh);
    gnome_print_closepath(context);
    gnome_print_eoclip(context);

    gnome_print_moveto(context, xoffset +30 +10 , pageh - 55.0);
  } else {
    gdouble xoffset1;
    gdouble xoffset2;

    gnome_print_newpath(context);
    gnome_print_moveto(context, xoffset +30 +10 +datew +10, 0);
    gnome_print_lineto(context, xoffset +30 +10 +(pagew-30) -pagenumw -60 ,0);
    gnome_print_lineto(context, xoffset +30 +10 +(pagew-30) -pagenumw -60 ,pageh);
    gnome_print_lineto(context, xoffset +30 +10 +datew +10, pageh);
    gnome_print_closepath(context);
    gnome_print_eoclip(context);

    xoffset1 = xoffset +30 +(pagew-30)/2 - titlew/2;
    xoffset2 = xoffset +30 +10 +datew +10;

    gnome_print_moveto(context, MAX(xoffset1, xoffset2), pageh - 55.0);
  }
  gnome_print_show(context, title);
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
  const gchar* locale;
  GValue value = { 0, };
  poptContext ctx;
  gchar **popt_args;
  gchar *filename = NULL;
  GSList* new_slist = NULL; /* use for temporary */
  gchar* page_title = NULL;

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
    u2ps_show_version();
    return 0;
  }

  g_value_init(&value, G_TYPE_POINTER);
  g_object_get_property(G_OBJECT(program), GNOME_PARAM_POPT_CONTEXT, &value);
  ctx = g_value_get_pointer(&value);
  g_value_unset(&value);

  popt_args = (char**) poptGetArgs(ctx);

  if( popt_args ) {
    filename = popt_args[0];
  }

  if( !filename ) {
    g_print(_("Specify filename for input.\n"));
    exit(0);
  }

  /* Detect inputfile overwrite mistake */
  if( output_filename && !strcmp(output_filename, filename) ) {
    g_error(_("Input and output file is same.\n"));
  }

  /* Read the Input Text */
  fp = fopen(filename, "r");
  if( !fp ) {
    g_error(_("File is not found: %s\n"), filename);
  }
  while(fgets(buf, 1024, fp) > 0 ) {
    text_slist = g_slist_append(text_slist, g_strdup(buf));
  }
  fclose(fp);

  /* Concatenate long line */
  for(i=0;i<g_slist_length(text_slist);i++) {
    gchar* tmpbuf = g_slist_nth_data(text_slist, i);

    if( strlen(tmpbuf) <= 0 )
      continue;

    if( tmpbuf[strlen(tmpbuf)-1] != '\n' ) {
      while(++i<g_slist_length(text_slist)) {
        tmpbuf = g_strconcat(tmpbuf, g_slist_nth_data(text_slist, i), NULL);
        if( tmpbuf[strlen(tmpbuf)-1] == '\n' )
          break; 
      }
    }
    new_slist = g_slist_append(new_slist, tmpbuf);
  }
  g_slist_free(text_slist);
  text_slist = new_slist;

  /* Cut the newline character */
  for(i=0;i<g_slist_length(text_slist);i++) {
    gchar* tmpbuf = g_slist_nth_data(text_slist, i);

    if( strlen(tmpbuf) > 1 && !strcmp(tmpbuf+strlen(tmpbuf)-2, "\r\n") ) {
      tmpbuf[strlen(tmpbuf)-2] = '\0';
    } else if( strlen(tmpbuf) > 0 && tmpbuf[strlen(tmpbuf)-1] == '\n' )
      tmpbuf[strlen(tmpbuf)-1] = '\0';
  }

  if( !input_encoding && parse_mail ) {
    input_encoding = get_charset(text_slist);
  }

  /* Korean codeset auto detection - EUC-KR */
  if( !input_encoding && !strncmp(locale, "ko_KR", strlen("ko_KR")) ) {
    gboolean is_euckr = TRUE;
    for(i=0;i<g_slist_length(text_slist);i++) {
      gchar* tmpbuf = g_slist_nth_data(text_slist, i);
      if( !(is_euckr = g_str_is_euckr(tmpbuf)) )
        break;
    }
    if( is_euckr ) {
      input_encoding = "EUC-KR";
    }
  }

  /* Simplified Chinese(zh_CN) codeset auto detection - GB18030 */
  if( !input_encoding && !strncmp(locale, "zh_CN", strlen("zh_CN")) ) {
    gboolean is_gb18030 = TRUE;
    for(i=0;i<g_slist_length(text_slist);i++) {
      gchar* tmpbuf = g_slist_nth_data(text_slist, i);
      if( !(is_gb18030 = g_str_is_gb18030(tmpbuf)) )
        break;
    }
    if( is_gb18030 ) {
      input_encoding = "GB18030";
    }
  }

  /* Traditional Chinese(zh_TW) codeset auto detection - Big5 */
  if( !input_encoding && !strncmp(locale, "zh_TW", strlen("zh_TW")) ) {
    gboolean is_big5 = TRUE;
    for(i=0;i<g_slist_length(text_slist);i++) {
      gchar* tmpbuf = g_slist_nth_data(text_slist, i);
      if( !(is_big5 = g_str_is_big5(tmpbuf)) )
        break;
    }
    if( is_big5 ) {
      input_encoding = "Big5";
    }
  }

  /* Japanese codeset auto detection - iso-2022-jp */
  /* Just check the 8th bit for simplify */
  if( !input_encoding && !strncmp(locale, "ja_JP", strlen("ja_JP")) ) {
    gboolean is_2022jp = TRUE;
    for(i=0;i<g_slist_length(text_slist);i++) {
      gchar* tmpbuf = g_slist_nth_data(text_slist, i);
      gint j;
      for(j=0;j<strlen(tmpbuf);j++) {
        if( tmpbuf[j] & 0x80 ) {
          is_2022jp = FALSE;
          break;
        }
      }
      if( !is_2022jp )
        break;
    }
    if( is_2022jp ) {
      input_encoding = "ISO-2022-JP";
    }
  }

  /* Japanese codeset auto detection - EUC-JP */
  if( !input_encoding && !strncmp(locale, "ja_JP", strlen("ja_JP")) ) {
    gboolean is_eucjp = TRUE;
    for(i=0;i<g_slist_length(text_slist);i++) {
      gchar* tmpbuf = g_slist_nth_data(text_slist, i);
      if( !(is_eucjp = g_str_is_eucjp(tmpbuf)) )
        break;
    }
    if( is_eucjp ) {
      input_encoding = "EUC-JP";
    }
  }

  /* Japanese codeset auto detection - SJIS */
  if( !input_encoding && !strncmp(locale, "ja_JP", strlen("ja_JP")) ) {
    gboolean is_sjis = TRUE;
    for(i=0;i<g_slist_length(text_slist);i++) {
      gchar* tmpbuf = g_slist_nth_data(text_slist, i);
      if( !(is_sjis = g_str_is_sjis(tmpbuf)) )
        break;
    }
    if( is_sjis ) {
      input_encoding = "SJIS";
    }
  }

  //g_print("input_encoding: \"%s\"\n", input_encoding);

  /* Encoding option */
  if( input_encoding ) {
    gsize bytes_read, bytes_written;
    GError *conv_error = NULL;
    GSList* convtext_slist = NULL;

    for(i=0;i<g_slist_length(text_slist);i++) {
       gchar* conv_before = g_slist_nth_data(text_slist, i);
       gchar* conv_after = g_convert(conv_before, -1, "UTF-8", input_encoding, &bytes_read, &bytes_written, &conv_error);
       if( conv_error ) {
         g_free(conv_after);
         g_print("%s\n", conv_error->message);
         g_print(_("Falling back to UTF-8\n"));
         break; /* Falling back to UTF-8 */
       }
       convtext_slist = g_slist_append(convtext_slist, conv_after);
    }

    if( conv_error ) {
      /* Let's forget the trial of conversion */
      g_error_free(conv_error);
      g_slist_free(convtext_slist);
    } else {
      /* convtext_slist is now text_slist */
      g_slist_free(text_slist);
      text_slist = convtext_slist;
    }
  }

  /* Get Page Title */
  if( parse_mail ) {
    page_title = get_subject(text_slist);
  } else {
    page_title = basename(filename);
  }

  /* Cut Headers */
  if( parse_mail ) {
    text_slist = cut_headers(text_slist);
  }

  /* UTF-8 check */
  for(i=0;i<g_slist_length(text_slist);i++) {
    const gchar* end;
    gchar* tmpbuf = g_slist_nth_data(text_slist, i);
    if( !g_utf8_validate(tmpbuf, -1, &end) ) {
      g_error(_("File is not valid UTF-8\n"));
    }
  }

  /* Replace tab to 8 spaces */
  /* Tab handling should be later as possible, for the sake of
     mail header handling */
  for(i=0;i<g_slist_length(text_slist);i++) {
    gchar* tmpbuf = g_slist_nth_data(text_slist, i);
    g_slist_nth(text_slist, i)->data = tab2spaces(tmpbuf);
    g_free(tmpbuf);
  }

  /* Prepare Printing */
  job = gnome_print_job_new(NULL);
  context = gnome_print_job_get_context(job);
  config = gnome_print_job_get_config(job);

  gnome_print_config_set(config, "Printer", "GENERIC");

  if( output_filename ) {
    gnome_print_job_print_to_file(job, (gchar*)output_filename);
  }

  gnome_print_config_set(config, GNOME_PRINT_KEY_PAPER_SIZE, "A4");
  gnome_print_config_set(config, GNOME_PRINT_KEY_PAGE_ORIENTATION, "R0");
  gnome_print_config_set(config, GNOME_PRINT_KEY_PAPER_ORIENTATION, "R180");
  gnome_print_config_set(config, GNOME_PRINT_KEY_DUPLEX, "false");
  gnome_print_config_set(config, GNOME_PRINT_KEY_LAYOUT, "2_1");
  gnome_print_config_get_page_size(config, &pagew, &pageh);

  gnome_print_beginpage(context, "1");

  /* Font Face from familyname */
  if( familyname ) {
    face = gnome_font_face_find_closest_from_weight_slant(familyname, GNOME_FONT_MEDIUM, FALSE);
  }

  /* Font Face 1st fallback - Use language default */
  if( !face ) {
    for(i=0;localefont[i].prefix != NULL;i++) {
      if( !strncmp(locale, localefont[i].prefix, strlen(localefont[i].prefix)) ) {
        familyname = localefont[i].fontname;
        face = gnome_font_face_find_closest_from_weight_slant(familyname, GNOME_FONT_MEDIUM, FALSE);
        break;
      }
    }
  }

  /* Font Face 2nd fallback - Use DEFAULT_FAMILY_NAME */
  if( !face ) {
    face = gnome_font_face_find_closest_from_weight_slant(DEFAULT_FAMILY_NAME, GNOME_FONT_MEDIUM, FALSE);
  }

  /* Abort when any fallback doesn't make sense */
  g_assert(face != NULL);

  /* Text Font */
  font = gnome_font_face_get_font_default(face, 12);
  g_assert(font != NULL);
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
      gnome_print_setfont(context, font); /* setfont again after beginpage */
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
    draw_pageframe(context, face, xoffset, pagew, pageh, page_title, i+1, pages);
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
