/*

    Main Program for Secure GrADS.

---
    Copyright (C) 2011 by Arlindo da Silva <dasilva@opengrads.org>
    Copyright (C) 2011 by Daniel da Silva <ddasilva@umd.edu>

    All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; using version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, please consult  
              
              http://www.gnu.org/licenses/licenses.html

    or write to the Free Software Foundation, Inc., 59 Temple Place,
    Suite 330, Boston, MA 02111-1307 USA

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(XLIBEMU32) 
#   define MAIN GRXMain
#else
#   define MAIN main
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int MAIN (int argc, char *argv[])  {

  Main(argc,argv);

}

#ifdef HAVE_FWRITE_UNLOCKED
#include <execinfo.h>

int trace_has_func(char *func_name) {
  void *array[10];
  size_t size;
  char **strings;
  size_t i, j;
  int found = 0;
  
  size = backtrace(array, 10);
  strings = backtrace_symbols(array, size);

  for(i=0; i < size; i++) {    
    char *s = strings[i];
    int m = -1, n = -1;

    for (j=0; j < strlen(s); j++) {
      if (s[j] == '(') {
	m = j + 1;
      } else if (m > 0 && s[j] == '+') {
	n = j;
      }
    }
    s[n] = '\0';
    s = s+m;
    //printf("%s\n", s);
    if (!strcmp(s, func_name)) {
      found = 1;
      break;
    }
  }
  free(strings);
  return found;
}

size_t fwrite(const void *array, size_t size, size_t count, FILE *stream) {
  if (trace_has_func("png_write_data") || trace_has_func("readline")) {
    return fwrite_unlocked(array, size, count, stream);
  } else {
    printf("Fwrite calls not supported in Secure GrADS.\n");
  }
}
#endif

int system(const char *command) {
  printf("System calls not supported in Secure GrADS.\n");
}

int fputc(int c, FILE *fp) {
  printf("Fputc calls not supported in Secure GrADS.\n");
}

int fputs(const char *str, FILE *stream) {
  printf("Fputs calls not supported in Secure GrADS.\n");
}
