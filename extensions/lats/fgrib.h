/*----
  constants
-----*/

#ifndef _FGRIB_H
#define _FGRIB_H

#define VERB 0
#define MAXNBITS 24			     /* Maximum precision for calculated number of bits
					      (when decimal scale factor is used) */
#define DEFAULT_NBITS 16		     /* Default precision if neither precision or scale factor
					      specified. */

typedef struct USER_VAR {
           char name[20] ;
           char li[12] ;
            int lv;
  unsigned char cent;
  unsigned char yr;
  unsigned char mo;
  unsigned char da;
  unsigned char hr;
  unsigned char mn;
           char ti[12];
            int tv;
	  float undef;
} user_var;


typedef struct USER_GRID {

          int ni;
          int nj;
        float dx;
        float dy;

        float latb;
        float late;
        float lonb;
        float lone;
           char gi[12];
  unsigned char drt;
        float *lat;
        float *lon;
} user_grid;

typedef struct GRIB_VAR_TABLE {
     unsigned int id;
           char name[20] ;
           char desc[38] ;
           char units[16] ;
            int dsf;
            int nbits;
} grib_var_table;


/*--------
  IS (J:0,6)
-------*/
typedef struct GRIB_IS{
            int len;
  unsigned char is[8];
  unsigned char ver ; 
} grib_is ; 

/*--------
  ES (J:5,1)
-------*/
typedef struct GRIB_ES{
  unsigned char len ; 
           char es[4] ;
} grib_es; 

/*--------
  PDS (J:1,1)
-------*/

typedef struct GRIB_PDS {

  unsigned char *pds;
   unsigned int len;   /* len of PDS */
  unsigned char ver;   /* parameter table version */
  unsigned char ctr;   /* center T0 J:1,3 */
  unsigned char proc;  /* process TA J:1,4 */
  unsigned char grid;  /* grid TB J:1,5-8 */
  unsigned char gflg;  /* B1 1 = GDS included 0 = not T1 J:1,26 */
  unsigned char bflg;  /* B2 1 = BMS included 0 = not T1 J:1,26 */
  unsigned char parm;  /* parameter T2 J:1,27-28 */
  unsigned char ltyp;  /* level type indicator T3 T3a J:1,33-36 */
  unsigned char l1;    /* level 1 T3 J:1,33-34 */
  unsigned char l2;    /* level 2 T3 J:1,33-34 */
   unsigned int l12;   /* level 1 and 2 T3 J:1,33-34 */
  unsigned char yr;    /* year of century */
  unsigned char mo;    /* month */
  unsigned char da;    /* day */
  unsigned char hr;    /* hour */
  unsigned char mn;    /* min */
  unsigned char ftu;   /* forecast time unit T4 J:1,36 */
  unsigned char p1;    /* period 1 */
  unsigned char p2;    /* period 2 */
   unsigned int p12;   /* period 1 and 2 */
  unsigned char tri;   /* time range indicator T5 J:1,37 */
   unsigned int nave;  /* number of points in the average */
  unsigned char nmis;  /* number missing from average */
  unsigned char cent;  /* century 20=1900 21=2000 */
  unsigned char sctr;  /* sub center T0 J:1,3-4 */
            int dsf;   /* decimal scale factor */
            int nbits;   /* decimal scale factor */
} grib_pds;

/*--------
  BDS for lon-lat grids (J:4,1)
-------*/

typedef struct GRIB_BDS{
  unsigned char *bds;
   unsigned int len;    /* length of BDS  */
  unsigned char dt;     /* B1 data type 0 = grid point 1 = spec coef T11 J:4,6 */
  unsigned char pt;     /* B2 packing 0 = simple 1 = complex T11 J:4,6 */
  unsigned char od;     /* B3 orig data 0 = floats 1 = int T11 J:4,6 */
  unsigned char af;     /* B4 additional flags in octet 14 0 = no 1 = yes  T11 J:4,6 */
  unsigned char nub;    /* B6-5 number of unused bits J:4,1*/
            int bsf;    /* binary scale factor  J:4,1 */
           char ref[4]; /* reference value float IBM format J:4,1 */
  unsigned char nb;     /* # bits / grid point T11 J:4,6 */
} grib_bds;

/*--------
  BMS for lon-lat grids (J:3,1)
-------*/

typedef struct GRIB_BMS{
   unsigned char *bms;
    unsigned int len;    /* length of BMS  */
   unsigned char nub;    /* number of unused bits */
    unsigned int nu;     /* numeric = 0 of bitmap otherwise # of a predefined bitmap */
} grib_bms;

/*--------
  GDS for lon-lat grids (J:2,1)
-------*/

typedef struct GRIB_GDS_LL{
  unsigned char *gds;
   unsigned int len;   /* length of GDS (32) */
  unsigned char nv;    /* num vert coor parameter */
  unsigned char pv;    /* location (octet E) of vert or num of points in each row or 255 */
  unsigned char drt;   /* data representation type (T6 J:2,2) */
   unsigned int ni;    /* num points in i */
   unsigned int nj;    /* num points in j */ 
            int lat1;  /* lat of first point (deg*1000) */
            int lon1;  /* lon of first point (deg*1000) */
  unsigned char rcdi;  /* B1 res comp flag -- direction increments 1 = given (T7 J:2,11) */
  unsigned char rcre;  /* B2 res comp flag -- earth 0 = sphere r=6357.4kkm 1 = oblate spheriod) (T7) */
  unsigned char rcuv;  /* B5 res comp flag -- u,v 0 = earth relative 1 = grid relative (T7) */
            int lat2;  /* lat of last point (J:2,3) */
            int lon2;  /* lon of last point (J:2,3) */
   unsigned int dx;    /* dlon, if not given all bits 1 (J:2,3) */
   unsigned int dy;    /* dlat if reg lon/lat grid, num of points eq-pole for gauss grid (J:2,3) */
  unsigned char smi;   /* scan mode in i 0 = +i 1 = -i (T8 J:2,12) */ 
  unsigned char smj;   /* scan mode in j 0 = -j 1 = +i (T8 J:2,12) */ 
  unsigned char smdir; /* order 0 = consecutive points in i 1= cons in j */
} grib_gds_ll;

typedef struct GRIB_GRID_TABLE {
            char name[12] ;
   unsigned char drt;
  } grib_grid_table;

typedef struct GRIB_LEV_TABLE {
            char name[12] ;
   unsigned char ltyp;
   unsigned char l1;
   unsigned char l2;
   unsigned char l12;
  } grib_lev_table;

typedef struct GRIB_TIME_TABLE {
            char name[12] ;
   unsigned char ftu;
   unsigned char p1;
   unsigned char p2;
   unsigned char p12;
   unsigned char tri;
  } grib_time_table;


typedef struct GRIB_CENTER_TABLE {
            char name[12] ;
   unsigned char ctr;
  } grib_center_table;


typedef struct GRIB_SUBCENTER_TABLE {
            char name[12] ;
   unsigned char sctr;
  } grib_subcenter_table;

typedef struct GRIB_PROC_TABLE {
            char name[12] ;
   unsigned char proc;
  } grib_proc_table ;

typedef struct GRIB_VARVER_TABLE {
            char name[12] ;
   unsigned char ver;
  } grib_varver_table ;

/*-------
  prototypes
--------*/

extern void show_bits(unsigned char);

extern void pds_init(grib_pds *);
extern void gds_init(grib_gds_ll *);
extern void bds_init(grib_bds *);
extern void bms_init(grib_bms *);

extern int is_set(grib_is *, unsigned int);

extern int pds_set(char *, 
	    user_grid *,  user_var *,  grib_pds *,  
	    grib_var_table *, grib_grid_table *,
	    grib_lev_table *, grib_time_table *,
	    grib_center_table *, grib_subcenter_table *,
	    grib_proc_table *) ;

extern int gds_set( user_grid *,  user_var *,  grib_gds_ll *,  
	    grib_var_table *, grib_grid_table *,
	    grib_lev_table *, grib_time_table *,
	    grib_proc_table *) ;

extern int bds_set(float *,  grib_pds *,
	    grib_bds *,  grib_bms *, 
	    float, int , int, int, float) ;

/* fginit.c */

extern int fginit( char *, char *, char *, char *,
	 user_var *, 
	 user_grid *, 

	 grib_is *, 
	 grib_pds *, 
	 grib_gds_ll *, 
	 grib_bms *, 
	 grib_bds *, 
	 grib_es *, 

	 grib_var_table **,
	 grib_grid_table **,
	 grib_lev_table **,
	 grib_time_table **,
	 grib_center_table **,
	 grib_subcenter_table **,
	 grib_proc_table **, 
	 grib_varver_table **) ;

/* fgutil.c */

extern void set_int3(unsigned char *string, unsigned int n);
extern void set_int2(unsigned char *string, unsigned int n);
extern void list2bitstream(unsigned int *, unsigned char *, int, int); 


/*-------
  macros
-------*/

#define SETBIT(ch,n)	ch |= (1 << (n))
#define CLRBIT(ch,n)	ch &= ~(1 << (n))
#define GETBIT(ch,n)    ((ch & (1 << (n)) != 0)
#define NUMELM(ch)      (sizeof(ch)/sizeof(ch[0]))
#ifndef INT3
#define INT3(a,b,c) ((1-(int) ((unsigned) (a & 0x80) >> 6)) * (int) (((a & 127) << 16)+(b<<8)+c))
#endif
#ifndef INT2
#define INT2(a,b)   ((1-(int) ((unsigned) (a & 0x80) >> 6)) * (int) (((a & 127) << 8) + b))
#endif


#endif  /* _FGRIB_H */
