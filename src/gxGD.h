/* Copyright (C) 1988-2016 by George Mason University. See file COPYRIGHT for more information. */

/* Function prototypes for GD interface. 
   The interactive interface (X windows) is managed by the routines in gxX11.c
   Hardcopy output is managed in gxprintGD.c  */

/* function prototypes */
gaint gxGDinit (gadouble, gadouble, gaint, gaint, gaint, char *);
void  gxGDcol (gaint);
void  gxGDwid (gaint);
void  gxGDacol (gaint);
void  gxGDrec (gadouble, gadouble, gadouble, gadouble);
void  gxGDbpoly (void);
gaint gxGDepoly (gadouble *, gaint);
void  gxGDmov (gadouble, gadouble);
void  gxGDdrw (gadouble, gadouble);
void  gxGDflush (void);
void  gxGDpoly (gaint, gaint, gaint);
gaint gxGDfgimg (char *);
gaint gxGDend (char *, char *, gaint, gaint);


