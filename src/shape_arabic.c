/* Arabic shaping code.
 *
 * $Id$
 *
 * Copyright (c) 2003 Arabeyes, Mohammed Sameer.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * I can say that this is a direct transltion for Nadim's shape_arabic.pl
 * script. All thanks to be directed to him, all blames to be directed to me.
 */

/* $ gcc -o do_shaping do_shaping.c `pkg-config glib-2.0 --cflags --libs`
 */

/*
 * TODO:
 * Use a static buffer somehow to avoid malloc/free.
 * Try to improve it a bit.
 * Any ideas ?
 */

/* ChangeLog:
 *  01/10/2003:
 *    Fixed the 0x649 presentation forms. "Thanks Nadim"
 */

#include <stdio.h>
#include <glib.h>

#define DEBUG(x) fprintf(stderr, "%s\n", x)

typedef struct
{
      gboolean junk;
      gunichar isolated;
      gunichar initial;
      gunichar medial;
      gunichar final;
}
char_node;

/* *INDENT-OFF* */
char_node shaping_table[] = {
/* 0x621 */  { FALSE, 0xFE80, 0x0000, 0x0000, 0x0000},
/* 0x622 */  { FALSE, 0xFE81, 0x0000, 0x0000, 0xFE82},
/* 0x623 */  { FALSE, 0xFE83, 0x0000, 0x0000, 0xFE84},
/* 0x624 */  { FALSE, 0xFE85, 0x0000, 0x0000, 0xFE86},
/* 0x625 */  { FALSE, 0xFE87, 0x0000, 0x0000, 0xFE88},
/* 0x626 */  { FALSE, 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A},
/* 0x627 */  { FALSE, 0xFE8D, 0x0000, 0x0000, 0xFE8E},
/* 0x628 */  { FALSE, 0xFE8F, 0xFE91, 0xFE92, 0xFE90},
/* 0x629 */  { FALSE, 0xFE93, 0x0000, 0x0000, 0xFE94},
/* 0x62A */  { FALSE, 0xFE95, 0xFE97, 0xFE98, 0xFE96},
/* 0x62B */  { FALSE, 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A},
/* 0x62C */  { FALSE, 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E},
/* 0x62D */  { FALSE, 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2},
/* 0x62E */  { FALSE, 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6},
/* 0x62F */  { FALSE, 0xFEA9, 0x0000, 0x0000, 0xFEAA},
/* 0x630 */  { FALSE, 0xFEAB, 0x0000, 0x0000, 0xFEAC},
/* 0x631 */  { FALSE, 0xFEAD, 0x0000, 0x0000, 0xFEAE},
/* 0x632 */  { FALSE, 0xFEAF, 0x0000, 0x0000, 0xFEB0},
/* 0x633 */  { FALSE, 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2},
/* 0x634 */  { FALSE, 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6},
/* 0x635 */  { FALSE, 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA},
/* 0x636 */  { FALSE, 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE},
/* 0x637 */  { FALSE, 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2},
/* 0x638 */  { FALSE, 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6},
/* 0x639 */  { FALSE, 0xFEC9, 0xFECB, 0xFECC, 0xFECA},
/* 0x63A */  { FALSE, 0xFECD, 0xFECF, 0xFED0, 0xFECE},
/* 0x63B */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63C */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63D */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63E */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63F */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x640 */  { FALSE, 0x0640, 0x0000, 0x0000, 0x0000},
/* 0x641 */  { FALSE, 0xFED1, 0xFED3, 0xFED4, 0xFED2},
/* 0x642 */  { FALSE, 0xFED5, 0xFED7, 0xFED8, 0xFED6},
/* 0x643 */  { FALSE, 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA},
/* 0x644 */  { FALSE, 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE},
/* 0x645 */  { FALSE, 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2},
/* 0x646 */  { FALSE, 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6},
/* 0x647 */  { FALSE, 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA},
/* 0x648 */  { FALSE, 0xFEED, 0x0000, 0x0000, 0xFEEE},
/* 0x649 */  { FALSE, 0xFEEF, 0x0000, 0x0000, 0xFEF0},
/* 0x64A */  { FALSE, 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2}
};
/* *INDENT-ON* */

int
main (int argc, char *argv[])
{
   FILE *fl;
   gchar buff[1024];
   glong len;

   if (!argv[1])
   {
      fprintf (stderr, "You didn't supply a file\n");
      return 1;
   }

   fl = fopen (argv[1], "r");
   if (!fl)
   {
      fprintf (stderr, "Can't open file\n");
      return 1;
   }

   while (fgets (buff, 1024, fl))
   {
      gint y;
      gunichar *ucs = NULL, *_ucs = NULL;
      gchar *utf = NULL;
      gint x = 0;		//strlen (buff);

      ucs = g_utf8_to_ucs4_fast (buff, -1, &len);
      _ucs = g_malloc (sizeof (gunichar) * (len + 1));
      _ucs[len] = 0;

      for (y = 0; y < len; y++)
      {
	 gboolean have_previous = TRUE, have_next = TRUE;

	 /* If it's not in our range, skip it. */
	 if ((ucs[y] < 0x621) || (ucs[y] > 0x64A))
	 {
	    DEBUG ("Character not in range");
	    _ucs[x++] = ucs[y];
	    continue;
	 }

	 /* The character wasn't included in the unicode shaping table. */
	 if (shaping_table[(ucs[y] - 0x621)].junk)
	 {
	    DEBUG ("Junk character");
	    _ucs[x++] = ucs[y];
	    continue;
	 }

	 if (((ucs[y - 1] < 0x621) || (ucs[y - 1] > 0x64A)) ||
	     (!(shaping_table[(ucs[y - 1] - 0x621)].initial) &&
	      !(shaping_table[(ucs[y - 1] - 0x621)].medial)))
	 {
	    DEBUG ("No previous");
	    have_previous = FALSE;
	 }

	 if (((ucs[y + 1] < 0x621) || (ucs[y + 1] > 0x64A)) ||
	     (!(shaping_table[(ucs[y + 1] - 0x621)].medial) &&
	      !(shaping_table[(ucs[y + 1] - 0x621)].final)
	      && (ucs[y + 1] != 0x640)))
	 {
	    DEBUG ("No next\n");
	    have_next = FALSE;
	 }

	 if (ucs[y] == 0x644)
	 {
	    if (have_next)
	    {
	       if ((ucs[y + 1] == 0x622) ||
		   (ucs[y + 1] == 0x623) ||
		   (ucs[y + 1] == 0x625) || (ucs[y + 1] == 0x627))
	       {
		  if (have_previous)
		  {
		     if (ucs[y + 1] == 0x622)
		     {
			_ucs[x++] = 0xFEF6;
		     }
		     else if (ucs[y + 1] == 0x623)
		     {
			_ucs[x++] = 0xFEF8;
		     }
		     else if (ucs[y + 1] == 0x625)
		     {
			_ucs[x++] = 0xFEFA;
		     }
		     else
		     {
			/* ucs[y+1] = 0x627 */
			_ucs[x++] = 0xFEFC;
		     }
		  }
		  else
		  {
		     if (ucs[y + 1] == 0x622)
		     {
			_ucs[x++] = 0xFEF5;
		     }
		     else if (ucs[y + 1] == 0x623)
		     {
			_ucs[x++] = 0xFEF7;
		     }
		     else if (ucs[y + 1] == 0x625)
		     {
			_ucs[x++] = 0xFEF9;
		     }
		     else
		     {
			/* ucs[y+1] = 0x627 */
			_ucs[x++] = 0xFEFB;
		     }
		  }
		  y++;
		  continue;
	       }
	    }
	 }

	 /** Medial **/
	 if ((have_previous) && (have_next)
	     && (shaping_table[(ucs[y] - 0x621)].medial))
	 {
	    DEBUG ("Medial condition");
/*
            if (shaping_table[(ucs[y] - 0x621)].medial)
	    {
 */
	       _ucs[x++] = shaping_table[(ucs[y] - 0x621)].medial;
/*
	    }
            else
	    {
	       _ucs[y] = ucs[y];
	    }
 */
	      continue;
	    }

	 /** Final **/
	 else if ((have_previous) && (shaping_table[(ucs[y] - 0x621)].final))
	 {
	    DEBUG ("Previous condition");
	    _ucs[x++] = shaping_table[(ucs[y] - 0x621)].final;
	    continue;
	 }

	 /** Initial **/
	 else if ((have_next) && (shaping_table[(ucs[y] - 0x621)].initial))
	 {
	    DEBUG ("Next condition");
	    _ucs[x++] = shaping_table[(ucs[y] - 0x621)].initial;
	    continue;
	 }

	 /** Isolated **/
	 else
	 {
	    if (shaping_table[(ucs[y] - 0x621)].isolated)
	    {
	       _ucs[x++] = shaping_table[(ucs[y] - 0x621)].isolated;
	    }
	    else
	    {
	       _ucs[x++] = ucs[y];
	    }
	    continue;
	 }
      }
/*
{
int x = 0;
char sz[32];
for (x = 0; x < len; x++)
{
printf("%i\n", x);
sprintf(sz, "0x%.4X", ucs[x]);
fprintf(stderr, "%s\t", sz);
sprintf(sz, "0x%.4X", _ucs[x]);
fprintf(stderr, "%s\n", sz);
}
}
*/
    _ucs[x] = 0x0;
    g_free (ucs);
    utf = g_ucs4_to_utf8 (_ucs, len, NULL, NULL, NULL);

    printf ("%s", utf);
    g_free (_ucs);
    g_free (utf);
   }

   fclose (fl);

   return 0;
}
