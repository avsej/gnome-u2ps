#ifndef _MAIL_H

#include <glib.h>

gchar* get_subject(GSList* mail_slist);
GSList* cut_headers(GSList* mail_slist);

#endif /* _MAIL_H */
