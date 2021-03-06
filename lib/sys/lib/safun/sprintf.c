/*
 *  Copyright (C) 1994 Haijo Schipper (abigail@xs4all.nl)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include "sprintf/version.h"
# include <limits.h>
# include "sprintf/config.h"
# include <type.h>
# include "sprintf/macros.h"
# include "sprintf/extra.c"
# ifdef   __TIME_CONVERSION__
# include "sprintf/time.h"
# include "sprintf/time.c"
# endif

private string convert_to_base (int i, int base);

string query_version () {return (VERSION);}


private int charp (mixed arg) {
  return (intp (arg) && CHAR_MIN <= arg && arg <= CHAR_MAX);
}


private string anything (mixed this) {
  switch (typeof (this)) {
    case T_INT:
    case T_FLOAT:
      return (this + "");
    Case T_STRING:
      return ("\"" + this + "\"");
    Case T_OBJECT:
      return ("OBJ <" + object_name (this) + ">");
    Case T_ARRAY: {
      int      i, sz;
      string * res;
      for (i = 0, res = allocate (sz = sizeof (this)); i < sz; i ++) {
        res [i] = anything (this [i]);
      }
      return ( "({ " + implode (res, ", ") + " })");
    }
    Case T_MAPPING: {
      int i, sz;
      mixed * idx, * vals;
      string * res;
      for (i = 0, res = allocate (sz = sizeof (idx = map_indices (this))),
           vals = map_values (this); i < sz; i ++) {
        res [i] = anything (idx [i]) + " : " + anything (vals [i]);
      }
      return ( "([ " + implode (res, ", ") + " ])");
    }
  }
}


private string give_padding (int n, string pad) {
  string padding;
  if (n <= 0) {return ("");}
  padding = pad;
  for (; strlen(ANSI_D->strip_colors (padding)) < n; padding += padding);
  return (padding [.. n - 1]);
}


/* Apply flags, width & precision to string. */
private string align (string this, int width, int precision, mapping options,
                      string padding) {
  int sz;
  if (options ["`"]) {this = reverse (this);}
  if (options ["~"]) {this = flip_case (this);}
  if (options ["<"]) {this = lower_case (this);}
  if (options [">"]) {this = upper_case (this);}
  if (options ["="]) {this = capitalize (this);}
  if (options ["&"]) {this = rot_13 (this);}
  sz = strlen(ANSI_D->strip_colors (this));
  if (options ["-"]) {this += give_padding (width - sz, padding);}
  else {
    if (options ["|"]) {
      this = give_padding ((width + 1 - sz) / 2, padding) + this +
             give_padding ((width - sz) / 2, padding);
    }
    else {this = give_padding (width - sz, padding) + this;}
  }
  return ((precision <= 0) || strlen(ANSI_D->strip_colors (this)) < precision
              ? this
              : options ["_"]
                   ? this [strlen(ANSI_D->strip_colors (this)) - precision ..]
                   : this [.. precision - 1]);
}


private string name_in_base (int i, int base) {
  /* Assume i < base */
  string tmp;
  tmp = "?";
  switch (i) {
    case  0 ..  9 : tmp [0] = ('0' + i); return (tmp);
    case 10 .. 35 : tmp [0] = ('a' + i - 10); return (tmp);
    case 36 .. 51 : tmp [0] = ('A' + i - 36); return (tmp);
    default : return (tmp);
  }
}

private string convert_to_base (int i, int base) {
  return (i < base ? name_in_base (i, base)
                   : convert_to_base (i / base, base) +
                     name_in_base (i % base, base));
}



private string numerical (int n, int base, int width, int precision, 
                          mapping options, string padding) {
  string digits, sign, header, this;
  int    sz;
  if (base != BASE) {digits = convert_to_base ((n < 0 ? -n : n), base);}
  else {digits = n + "";}
  sign = (n < 0 ? "-" : options ["+"] ? "+" : options [" "] ? " " : "");
  header = (options ["#"] ? (base == 8 ? "0" : base == 16 ? "0x" : "") : "");
# ifdef __TIME_CONVERSION__
  if (precision <= 0) {precision = options ["T"];}
# endif
  if (precision > 0) {
    digits = give_padding (precision - strlen(ANSI_D->strip_colors (digits)), "0") + digits;
  }
  if (options ["0"] && !options ["-"]) {
    digits = give_padding (width - strlen(ANSI_D->strip_colors (digits + sign + header)), "0") +
             digits;
  }
  sz = strlen(ANSI_D->strip_colors (this = sign + header + digits));
  if (options ["X"]) {this = upper_case (this);}
  if (options ["-"]) {this += give_padding (width - sz, padding);}
  else {this = give_padding (width - sz, padding) + this;}
  return (this);
}


private string round_off (string arg) {
  int i;
  i = strlen(ANSI_D->strip_colors (arg)) - 1;
  if (arg [i] != '9') {arg [i] += 1; return (arg);}
  if (i == 0) {return ("10");}
  return (round_off (arg [.. i - 1]) + "0");
}


private string do_float (float x, int width, int precision,
                         mapping options, string padding, int exponent) {
  float * parts;
  float   i_x, f_x;
  int     e_x;
  string  i_res, f_res, e_res, result;
  string  sign;
  int     i, sz;

  if (x < 0.0) {sign = "-"; x = -x;}
  else {if (options ["+"]) {sign = "+";}}
  if (width && sign) {width --;}

  if (precision == 0) {precision = FLT_PRECISION;}

  /* Find exponent. */
  if (exponent) {
    float y;
    y = x;
    while (x > 10.0) {e_x ++; x /= 10.0;}
    while (x < 1.0)  {e_x --; x *= 10.0;}
    if (exponent & 4) {
      if (e_x > -4 && e_x < (precision == -2 ? 0 : precision)) {
        /* Use %f. */
        x = y;
        exponent &= 6;  /* Don't print exponent. */
      }
    }
  }

  parts = modf (x);
  f_x = parts [0];
  i_x = parts [1];

  /* Build string for integer part. */
  for (i_res = ""; i_x >= 1.0; i_x = i_x / 10.0) {
    i_res = (string) (int) floor (fmod (i_x, 10.0)) + i_res;
  }
  if (!strlen(ANSI_D->strip_colors (i_res))) {i_res = "0";}
  if ((exponent & 4) && precision != -2) {
    precision -= strlen(ANSI_D->strip_colors (i_res));
  } /* Now precision should be the number of digits after the period. */

  /* Build string for fractional part. */
  if (f_x == 0.0) {f_res = "0";}
  else {
    for (f_res = ""; f_x < 0.1; f_res += "0", f_x *= 10.0);
    f_res += ((string) f_x) [2 ..];
  }

  if (precision == -2) {f_res = (options ["#"] ? "." : "");}
  else {
    if (strlen(ANSI_D->strip_colors (f_res)) == precision) {
      f_res = "." + f_res;
    }
    else {
      if (strlen(ANSI_D->strip_colors (f_res)) > precision) {
        if (f_res [precision] > '4') { /* Round off away from zero. */
          f_res = round_off (f_res [.. precision - 1]);
          if (precision != strlen(ANSI_D->strip_colors (f_res))) {  /* Overflow to integer part. */
            i_res = round_off (i_res);
            if ((exponent & 1) && i_res == "10") {
              /* Overflow to exponent */
              e_x ++;
              i_res = "1";
            }
            f_res = give_padding (precision, "0"); /* Fill with 0's. */
          }
          f_res = "." + f_res;
        }
        else {f_res = "." + f_res [.. precision - 1];}
      }
      else {
        f_res = "." + f_res + give_padding (precision - strlen(ANSI_D->strip_colors (f_res)), "0");
      }
    }
  }
  if ((exponent & 4) && strlen(ANSI_D->strip_colors (f_res))) { /* Strip trailing 0's and period. */
    for (i = strlen(ANSI_D->strip_colors (f_res)); f_res [-- i] == '0';);
    if (i) {f_res = f_res [.. i];}
    else {f_res = (options ["#"] ? "." : "");}
  }
  

  /* Build string for exponent part. */
  if (exponent & 1) {
    e_res = ((exponent & 2) ? "E" : "e");
    if (e_x < 0) {e_res += "-"; e_x = - e_x;}
    else {e_res += "+";}
    e_res += (e_x < 9 ? e_x == 0 ? "00" : "0" + e_x : (string) e_x);
  }
  else {e_res = "";}

  /* Add padding and sign. */
  if ((sz = strlen(ANSI_D->strip_colors (result = i_res + f_res + e_res))) < width) {
    string pads;
    if (options ["0"]) {padding = "0";}
    pads = give_padding (width - sz, padding);
    if (padding == " ") {result = pads + (sign ? sign : "") + result;}
    else {result = (sign ? sign : "") + pads + result;}
  }
  else {if (sign) {result = sign + result;}}

  return (result);
}



/* Scanning of the format strings. Result is an array of strings, */
/* either a conversion sequence, or a string of 'normal' chars.   */
private string * make_chunks (string format) {
  string * result;
  string   current;
  int      i, j, sz;
  string   tmp;
  string   option;
  result = ({ });
  i = j = 0;
  sz = strlen(ANSI_D->strip_colors (format));
  while (i < sz) {
    if (CONV_CHAR [format [i]]) { /* Encountered a '%' or '@' */
      option = format [i .. i];
      i ++;

      /* Scan flags. */
      while (i < sz && FLAGS [format [i]]) {i ++;}
      ENDCHECK (i, sz);
      tmp = format [j + 1 .. i - 1] + ",";  /* Flags */
      j = i;

      /* Scan width */
      if (format [i] == '*') {
        i ++;
        tmp += "-1" + ",";
      }
      else {
        while (i < sz && DIGIT [format [i]]) {i ++;}
        tmp += (format [j .. i - 1] == "" ? "0" : format [j .. i - 1]) + ",";
      }
      ENDCHECK (i, sz);

      /* Scan precision */
      if (format [i] == '.') {
        i ++;
        ENDCHECK (i, sz);
        j = i;
        if (format [i] == '*') {
          i ++;
          tmp += "-1" + ",";
        }
        else {
          while (i < sz && DIGIT [format [i]]) {i ++;}
          tmp += (format [j .. i - 1] == "" ? "0" : format [j .. i - 1]) + ",";
        }
        ENDCHECK (i, sz);
      }

      /* Chunck */
      result += ({ option + format [i .. i] + tmp });
      j = ++ i;
    }
    else {
      do { i ++; } while (i < sz && !CONV_CHAR [format [i]]);
      result += ({ format [j .. i - 1] });
      j = i;
    }
  }
  return (result);
}

# ifdef __CLOSE_TO_C__
static int sprintf (string out, string format, mixed args...) {
# else
static string sprintf (string format, mixed args...) {
# endif
  mixed  * arguments;
  string * chunks;
  string   result;
  string   flags, padding;
  string   tmp;
  string   cur;
  int      i, j, width, precision, sz, sz_args;
  int      exp;
  mapping  options;
  object   query_object;

# ifdef __CLOSE_TO_C__
  TYPECHECK (array, out, 0);
  if (!sizeof (out)) {error ("Sprintf: empty first argument");}
# endif

  TYPECHECK (string, format, 1);

  chunks = make_chunks (format);
  padding = " ";
  for (i = 0, sz = sizeof (chunks), sz_args = sizeof (args), result = "";
       i < sz; i ++) {
    if (!CONV_CHAR [chunks [i] [0]]) {result += chunks [i];}
    else {
      if (chunks [i] [0 .. 1] == "%%" || chunks [i] [0 .. 1] == "@@") {
        result += chunks [i] [1 .. 1];
      }
      else {
        width = precision = 0;
        ARGCOUNTCHECK (j, sz_args);
        switch (sscanf (chunks [i] [2 ..], "%s,%d,%d",
                        flags, width, precision)) {
          case 3: if (!precision) {precision = -2;}
# if 0
NOTE      case 2: if (!width) {width = -2;} /* Reserved for future use? */
# endif
        }
        if (width == -1) {
          TYPECHECK (int, args [j], j + 2);
          width = args [j ++];
          ARGCOUNTCHECK (j, sz_args);
        }
        if (precision == -1) {
          TYPECHECK (int, args [j], j + 2);
          precision = args [j ++];
          ARGCOUNTCHECK (j, sz_args);
        }
        options = mkmapping (explode (flags, ""), explode (flags, ""));
        switch (cur = chunks [i] [.. 1]) {
          case "%A":
          case "%a": {
            int k, sk;
            TYPECHECK (array, args [j], j + 2);
            for (k = 0, sk = sizeof (args [j]); k < sk; k ++) {
              if (cur == "%a" && nump (args [j] [k])) {args [j] [k] += "";}
              TYPECHECK (string, args [j] [k], j + 2);
              result += align (args [j] [k], width, precision,
                               options, padding);
            }
          }
          Case "%b": /* Binary      */
          case "%d": /* Decimal     */
          case "%i":
          case "%o": /* Octal       */
          case "%x": /* Hexadecimal */
          case "%X":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (args [j],
                                 cur == "%b" ?  2 :
                                 cur == "%o" ?  8 :
                                 cur == "%x" || cur == "%X" ? 16 : BASE,
                                 width, precision,
                                 cur == "%X" ? (options + ([ "X" : 1 ]))
                                             : options, padding);
          Case "%c":
            TYPECHECK (char, args [j], j + 2);
            tmp = " ";
            tmp [0] = args [j];
            result += align (tmp, width, precision, options, padding);
          Case "%H":
            TYPECHECK (string, args [j], j + 2);
            result += align (make_hex (args [j], padding), width, precision,
                             options, padding);
          Case "%h":
            TYPECHECK (string, args [j], j + 2);
            result += align (make_hex (args [j], nil), width, precision,
                             options, padding);
          Case "%n":
            TYPECHECK (array, args [j], j + 2);
            if (!sizeof (args [j])) {error ("No space for %n conversion");}
            args [j] [0] = strlen(ANSI_D->strip_colors (result));
          Case "%O":
            TYPECHECK (object, args [j], j + 2);
            result += align (object_name (args [j]), width, precision,
                             options, padding);
          Case "%p":
            TYPECHECK (char, args [j], j + 2);
            padding = "X";
            padding [0] = args [j];
          Case "%Q": 
            TYPECHECK (object, args [j], j + 2);
            query_object = args [j];
          Case "%q": {
            mixed res;
            TYPECHECK (string, args [j], j + 2);
            if (!query_object) {error ("%q used before %Q in sprintf.");}
            res = (mixed) call_other (query_object, args [j]);
            if (intp (res)) {res += "";}
            if (!stringp (res)) {
              res = anything (res);
            }
            result += align (res, width, precision, options, padding);
          }
          Case "%R":
            TYPECHECK (string, args [j], j + 2);
            result += align (crypt (args [j], 1), width, precision,
                             options, padding);
          Case "%r":
            TYPECHECK (string, args [j], j + 2);
            result += align (crypt (args [j], 0), width, precision,
                             options, padding);
          Case "%s":
            if (nump (args [j])) {args [j] += "";}
NOTE      case "%S":
            TYPECHECK (string, args [j], j + 2);
            result += align (args [j], width, precision, options, padding);
          Case "%y":
            result += align (anything (args [j]), width, precision,
                             options, padding);
# ifdef __FLOATS__       
          /* Bit 0 for exp, bit 1 for uppercase, bit 2 for conditional exp */
          Case "%G": 
          case "%g": exp |= 4;
          case "%E": if (cur != "%g") {exp |= 2;}
          case "%e": exp |= 1;
          case "%f": 
            if (intp (args [j])) {args [j] = (float) args [j];}
            TYPECHECK (float, args [j], j + 2);
            result += do_float (args [j], width, precision, options, padding,
                                exp);
            exp = 0;  /* Don't leave this out! */
# endif              
# ifdef __TIME_CONVERSION__
          /* Time related conversions. */
          Case "@a":
            TYPECHECK (int, args [j], j + 2);
            result += align (weekday (args [j], 0), width, precision,
                             options, padding);
          Case "@A":
            TYPECHECK (int, args [j], j + 2);
            result += align (weekday (args [j], 1), width, precision,
                             options, padding);
          Case "@b":
            TYPECHECK (int, args [j], j + 2);
            result += align (month (args [j], 0), width, precision,
                             options, padding);
          Case "@B":
            TYPECHECK (int, args [j], j + 2);
            result += align (month (args [j], 1), width, precision,
                             options, padding);
          Case "@c":
            TYPECHECK (int, args [j], j + 2);
            result += align (ctime (args [j]), width, precision, options,
                             padding);
          Case "@d":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (day (args [j]), BASE, width, precision,
                                 options + ([ "T" : 2 ]), padding);
          Case "@H":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (hour (args [j]), BASE, width, precision,
                                 options + ([ "T" : 2 ]), padding);
          Case "@I": {
            int h;
            TYPECHECK (int, args [j], j + 2);
            result += numerical ((h = hour (args [j])) ? h % NOON : NOON, BASE,
                                  width, precision, options + ([ "T" : 2 ]),
                                  padding);
          }
          Case "@j":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (day_of_year (args [j]), BASE, width, precision,
                                 options + ([ "T" : 3 ]), padding);
          Case "@m":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (month (args [j], 2) + 1, BASE, width,
                                 precision, options + ([ "T" : 2 ]), padding);
          Case "@M":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (minute (args [j]), BASE, width, precision,
                                 options + ([ "T" : 2 ]), padding);
          Case "@p":
            TYPECHECK (int, args [j], j + 2);
            result += align (hour (args [j]) < NOON ? AM : PM,
                             width, precision, options, padding);
          Case "@S":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (second (args [j]), BASE, width, precision,
                                 options + ([ "T" : 2 ]), padding);
          Case "@U":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (week_number (args [j], 0), BASE, width,
                                 precision, options + ([ "T" : 2 ]), padding);
          Case "@w":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (weekday (args [j], 2), BASE, width, precision,
                                 options + ([ "T" : 1 ]), padding);
          Case "@W":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (week_number (args [j], 1), BASE, width,
                                 precision, options + ([ "T" : 2 ]), padding);
          Case "@x":
            TYPECHECK (int, args [j], j + 2);
            result += align (date (args [j]), width, precision,
                             options, padding);
          Case "@X":
            TYPECHECK (int, args [j], j + 2);
            result += align (ttime (args [j]), width, precision,
                             options, padding);
          Case "@y":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (year (args [j]) % 100, BASE, width, precision,
                                 options + ([ "T" : 2 ]), padding);
          Case "@Y":
            TYPECHECK (int, args [j], j + 2);
            result += numerical (year (args [j]), BASE, width, precision,
                                 options, padding);
# ifdef __TIME_ZONE__
          Case "@Z":
            TYPECHECK (int, args [j], j + 2);
            result += align (timezone (args [j]), width, precision,
                             options, padding);
# endif
# endif       
          Default :
            error ("Unknown conversation character " + cur);
        }
        j ++;
      }
    }
  } 
# ifdef __CLOSE_TO_C__
  out [0] = result;
  return (strlen(ANSI_D->strip_colors (result)));
# else
  return (result);
# endif
}
