#include <mail.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

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

static gchar*
subject_decode(guchar* subject) {
  const gchar* base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  guchar* cursor;
  guchar* result = g_strdup("");
  guchar buf[4];
  gint i = 0;
  guchar* tmpbuf;
  gchar* prefix;

  g_return_val_if_fail(subject != NULL, NULL);
  g_return_val_if_fail(*subject != '\0', NULL);

  cursor = strstr(subject, "=?");
  if( !cursor ) {
    return g_strdup(subject);
  }

  if( !g_strncasecmp(cursor+2, "iso-2022-jp?b?", strlen("iso-2022-jp?b?")) ){
  } else {
    return g_strdup(subject);
  }

  prefix = g_strndup(subject, cursor-subject);
  cursor += strlen("=?iso-2022-jp?b?");

  i = 0;
  while( strncmp(cursor, "?=", strlen("?=")) != 0 ) {
    guchar c;
    bzero(buf, 4);

    c = strchr(base64, cursor[0])-base64;
    buf[0] = c<<2;
    c = strchr(base64, cursor[1])-base64;
    buf[0] |= c>>4;
    buf[1]  = c<<4;
    c = strchr(base64, cursor[2])-base64;
    buf[1] |= c>>2;
    buf[2]  = c<<6;
    c = (cursor[3] != '=') ? strchr(base64, cursor[3])-base64 : 0;
    buf[2] |= c;

    tmpbuf = g_strconcat(result, buf, NULL);
    g_free(result);
    result = tmpbuf;
    cursor += 4;
  }

  tmpbuf = g_strconcat(prefix, result, cursor+2, NULL);
  //tmpbuf = g_strconcat(result,NULL);
  g_free(result);
  g_free(prefix);
  result = tmpbuf;

  return result;
}

gchar*
get_subject(GSList* mail_slist)
{
  gchar* result;
  gboolean sbj_start = FALSE;
  gchar* subject = g_strdup("");
  gchar* tmpbuf;
  gint i;

  GError* conv_error = NULL;
  gsize bytes_read, bytes_written;

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

    if( !g_strncasecmp(buf, "subject:", strlen("subject:")) ) {
      gchar* decoded = subject_decode(buf + strlen("subject: "));
      sbj_start = TRUE;
      tmpbuf = g_strconcat(subject, decoded, NULL);
      g_free(subject);
      subject = tmpbuf;
      if( decoded != NULL )
        g_free(decoded);
      continue;
    }
    if( sbj_start ) {
      if( strstr(buf, ": ") ) { /* If another header starts, */
        sbj_start = FALSE;
        //printf("%s", subject);
      } else { /* 2nd or more of subject lines */
        gchar* decoded = subject_decode(buf);
        if( subject[strlen(subject)-1] == '\n' ) {
          subject[strlen(subject)-1] = '\0';
        }
        if( subject[strlen(subject)-1] == '\r' ) {
          subject[strlen(subject)-1] = '\0';
        }
        if( decoded && (*decoded == '\t' || *decoded == ' ') )
          tmpbuf = g_strconcat(subject, decoded+1, NULL);
        else
          tmpbuf = g_strconcat(subject, decoded, NULL);
        g_free(subject);
        subject = tmpbuf;
        if( decoded )
          g_free(decoded);
        continue;
      }
    }
  }

  result = g_convert(subject, -1, "UTF-8", "ISO-2022-JP", &bytes_read, &bytes_written, &conv_error);
  if( conv_error ) {
    g_print("%s\n", conv_error->message);
    if( result )
      g_free(result);
    return subject;
  }
  g_free(subject);
  
  return result;
}

GSList*
cut_headers(GSList* mail_slist)
{
  gint i,j;
  GSList* new_slist = NULL;

  for(i=0;i<g_slist_length(mail_slist);i++) {
    gchar* text = g_slist_nth_data(mail_slist, i);
    if( !strcmp(text, "") ) {
      /* Header end */
      break;
    }
    //new_slist = g_slist_append(new_slist, text);
    for(j=0;worthy_headers[j] != NULL;j++) {
      if( !g_strncasecmp(text, worthy_headers[j], strlen(worthy_headers[j])) ) {
        new_slist = g_slist_append(new_slist, text);
        break;
      }
    }
  }
  /* Append the rest */
  for(;i<g_slist_length(mail_slist);i++) {
    gchar* text = g_slist_nth_data(mail_slist, i);
    new_slist = g_slist_append(new_slist, text);
  }

  g_slist_free(mail_slist);

  return new_slist;
}
