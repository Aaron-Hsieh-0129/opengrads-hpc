#define NRANSI

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif
#ifndef SIGN
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
#endif

static char pout[256];   /* Build error msgs here */

// modified from Numerical Recipes
int nrerror(char error_text[])
/* Numerical Recipes standard error handler */
{
	gaprnt(0,"ERROR: Run-time error...\n");
	sprintf(pout,"libbjt: %s\n",error_text);
        gaprnt(0,pout);
	return(1);
}

double *vector(int ncol)
/* allocate a double matrix */
{
	int i;
	double *m;

	/* allocate cols and set pointers to them */
	m=(double *) malloc(((ncol)*sizeof(double)));
	if (!m) gaprnt(0,"ERROR: allocation failure 1 in vector()");
	/* return pointer to array of pointers to rows */
	return m;
}

void free_vector(double *m)
/* free a double vector allocated by vector() */
{
	if(m!=NULL) free((char*) (m));
}

int *lvector(int ncol)
/* allocate a double matrix */
{
	int i;
	int *m;

	/* allocate cols and set pointers to them */
	m=(int *) malloc(((ncol)*sizeof(int)));
	if (!m) gaprnt(0,"ERROR: allocation failure 1 in vector()");
	/* return pointer to array of pointers to rows */
	return m;
}

void free_lvector(int *m)
/* free a double vector allocated by vector() */
{
	if(m!=NULL) free((char*) (m));
}

double **matrix(int nrow, int ncol)
/* allocate a double matrix */
{
	int i;
	double **m;

	/* allocate pointers to rows */
	m=(double **) malloc(((nrow)*sizeof(double*)));
	if (!m) gaprnt(0,"ERROR: allocation failure 1 in matrix()");
	/* allocate rows and set pointers to them */
	m[0]=(double *) malloc(((nrow*ncol)*sizeof(double)));
	if (!m[0]) gaprnt(0,"ERROR: allocation failure 2 in matrix()");
	for(i=1;i<nrow;i++) m[i]=m[i-1]+ncol;
	/* return pointer to array of pointers to rows */
	return m;
}

void free_matrix(double **m)
/* free a double matrix allocated by matrix() */
{
	if(m[0]!=NULL) free((char*) (m[0]));
	if(m!=NULL) free((char*) (m));
}

double ***tensor(int ndepth, int nrow, int ncol)
/* allocate a double tensor */
{
	int i,j;
	double ***m;

	/* allocate pointers to depths */
	m=(double ***) malloc(((ndepth)*sizeof(double **)));
	if (!m) gaprnt(0,"ERROR: depth pointer allocation failure in tensor()");
	/* allocate pointers to rows */
	m[0]=(double **) malloc(((ndepth*nrow)*sizeof(double*)));
	if (!m[0]) gaprnt(0,"ERROR: row pointer allocation failure in tensor()");
	/* allocate data */
	m[0][0]=(double *) malloc(((ndepth*nrow*ncol)*sizeof(double)));
	if (!m[0][0]) gaprnt(0,"ERROR: data allocation failure in tensor()");
	/* set pointers */
	for(i=1;i<ndepth;i++) {
		m[i]=m[i-1]+nrow;
		m[i][0]=m[i-1][0]+nrow*ncol;
		for(j=1;j<nrow;i++) m[i][j]=m[i][j-1]+ncol;
	}
	/* return pointer to array of pointers to rows */
	return m;
}

void free_tensor(double ***m)
/* free a double tensor allocated by tensor() */
{
	if(m[0][0]!=NULL) free((char*) (m[0][0]));
	if(m[0]!=NULL) free((char*) (m[0]));
	if(m!=NULL) free((char*) (m));
}

int ludcmpb(double **a, int n, int *luindx, double *d)
{
	double tiny=1.0e-20;
	int i,imax,j,k;
	double big,dum,sum,temp;
	double *vv;

	vv = (double *)malloc(sizeof(double)*n);	
	*d=1.0;
	for (i=0;i<n;i++) {
		big=0.0;
		for (j=0;j<n;j++)
			if ((temp=fabs(a[i][j])) > big) big=temp;
		if (big < tiny) {return 1;}
//		if (big < TINY) {gaprnt(0,"Singular matrix in routine ludcmp.\n"); return 1;}
//		if (big == 0.0) {gaprnt(0,"Singular matrix in routine ludcmp"); return 1;}
		vv[i]=1.0/big;
	}
	for (j=0;j<n;j++) {
		for (i=0;i<j;i++) {
			sum=a[i][j];
			for (k=0;k<i;k++) sum -= a[i][k]*a[k][j];
			a[i][j]=sum;
		}
		big=0.0;
		for (i=j;i<n;i++) {
			sum=a[i][j];
			for (k=0;k<j;k++) sum -= a[i][k]*a[k][j];
			a[i][j]=sum;
			if ((dum=vv[i]*fabs(sum)) >= big) {
				big=dum;
				imax=i;
			}
		}
		if (j != imax) {
			for (k=0;k<n;k++) {
				dum=a[imax][k];
				a[imax][k]=a[j][k];
				a[j][k]=dum;
			}
			*d = -*d;
			vv[imax]=vv[j];
		}
		luindx[j]=imax;
		if (a[j][j] < tiny) {return 1;}
//		if (a[j][j] < TINY) {gaprnt(0,"ERROR: Singular matrix in routine ludcmp.\n"); return 1;}
//		if (a[j][j] == 0.0) a[j][j]=TINY;
		if (j != n-1) {
			dum=1.0/(a[j][j]);
			for (i=j+1;i<n;i++) a[i][j] *= dum;
		}
	}
	free (vv);
	return 0;
}


void lubksb(double **a, int n, int *luindx, double b[])
{
	int i,ii=0,ip,j;
	double sum;

	for (i=0;i<n;i++) {
		ip=luindx[i];
		sum=b[ip];
		b[ip]=b[i];
		if (ii != 0)
			for (j=ii-1;j<i;j++) sum -= a[i][j]*b[j];
		else if (sum !=0.0) ii=i+1;
		b[i]=sum;
	}
	for (i=n-1;i>=0;i--) {
		sum=b[i];
		for (j=i+1;j<n;j++) sum -= a[i][j]*b[j];
		b[i]=sum/a[i][i];
	}
}
#define MAXIT 100
#define EPS 3.0e-7
#define FPMIN 1.0e-30

double betacf(double a, double b, double x)
{
	int m,m2;
	double aa,c,d,del,h,qab,qam,qap;

	qab=a+b;
	qap=a+1.0;
	qam=a-1.0;
	c=1.0;
	d=1.0-qab*x/qap;
	if (fabs(d) < FPMIN) d=FPMIN;
	d=1.0/d;
	h=d;
	for (m=1;m<=MAXIT;m++) {
		m2=2*m;
		aa=m*(b-m)*x/((qam+m2)*(a+m2));
		d=1.0+aa*d;
		if (fabs(d) < FPMIN) d=FPMIN;
		c=1.0+aa/c;
		if (fabs(c) < FPMIN) c=FPMIN;
		d=1.0/d;
		h *= d*c;
		aa = -(a+m)*(qab+m)*x/((a+m2)*(qap+m2));
		d=1.0+aa*d;
		if (fabs(d) < FPMIN) d=FPMIN;
		c=1.0+aa/c;
		if (fabs(c) < FPMIN) c=FPMIN;
		d=1.0/d;
		del=d*c;
		h *= del;
		if (fabs(del-1.0) < EPS) break;
	}
	if (m > MAXIT) {
		sprintf(pout,"x=%g, a(%g) or b(%g) too big, or MAXIT too small in betacf\n",x,a,b);
		gaprnt (0,pout);
	}	
	return h;
}

double gammln(const double xx)
{
	int j;
	double x,y,tmp,ser;
	static const double cof[6]={76.18009172947146,-86.50532032941677,
		24.01409824083091,-1.231739572450155,0.1208650973866179e-2,
		-0.5395239384953e-5};

	y=x=xx;
	tmp=x+5.5;
	tmp -= (x+0.5)*log(tmp);
	ser=1.000000000190015;
	for (j=0;j<6;j++) ser += cof[j]/++y;
	return -tmp+log(2.5066282746310005*ser/x);
}

double betai(const double a, const double b, const double x)
{
	double bt;

	if (x < 0.0 || x > 1.0) gaprnt (0,"ERROR: Bad x in routine beta\n");
	if (x <=FPMIN || x >= (1.0-FPMIN)) {
		if (x < (a+1.0)/(a+b+2.0)) return 0.;
		else return 1.0;
	} else {	
		bt=exp(gammln(a+b)-gammln(a)-gammln(b)+a*log(x)+b*log(1.0-x));
		if (x < (a+1.0)/(a+b+2.0)) return bt*betacf(a,b,x)/a;
		else return 1.0-bt*betacf(b,a,1.0-x)/b;
	}	
}

double ttestb(double t, double df)
{	// ttest:
	// probability for two means m1 and m2 to be different.
	// where t=(m1-m2)/std
	//       std = standard deviation
	//       df is the degree of freedom
	double prob;
	prob=1.-(double)betai((double)(0.5*df),(double)(0.5),(double)(df/(df+max(t*t,FPMIN))));
	return (prob);
}

double ftestb(double f, double df1, double df2)
{	// F test:
	// probability for f=var1/var2>1,
	// where var1 has df1 degree of freedom and
	//       var2 has df2 degree of freedom
	double prob;
	prob = (double)(1.-betai((double)(0.5*df2),(double)(0.5*df1),(double)(df2/(df2+df1*f))));
	if (prob > 1.0) prob=2.0-prob;
	return (prob);
}

#undef MAXIT
#undef EPS
#undef FPMIN

void svbksb(double **u, double w[], double **v, int m, int n, double b[], double x[])
{
	int jj,j,i;
	double s,*tmp;
	tmp=vector(n);
	for (j=0;j<n;j++) {
		s=0.0;
		if (w[j] != 0.0) {
			for (i=0;i<m;i++) s += u[i][j]*b[i];
			s /= w[j];
		}
		tmp[j]=s;
	}
	for (j=0;j<n;j++) {
		s=0.0;
		for (jj=0;jj<n;jj++) s += v[j][jj]*tmp[jj];
		x[j]=s;
	}
	free_vector(tmp);
}


double pythag(double a, double b)
{
	double absa,absb;
	absa=fabs(a);
	absb=fabs(b);
	if (absa > absb) return absa*sqrt(1.0+pow(absb/absa,2));
	else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+pow(absa/absb,2)));
}

int svdcmpb(double **a, int m, int n, double w[], double **v)
{
	int flag,i,its,j,jj,k,l,nm;
	double anorm,c,f,g,h,s,scale,x,y,z,*rv1;
	int true=1, false=0;

	rv1=vector(n);
	g=scale=anorm=0.0;
	for (i=0;i<n;i++) {
		l=i+2;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i < m) {
			for (k=i;k<m;k++) scale += fabs(a[k][i]);
			if (scale != 0.0) {
				for (k=i;k<m;k++) {
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l-1;j<n;j++) {
					for (s=0.0,k=i;k<m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale *g;
		g=s=scale=0.0;
		if (i+1 <= m && i+1 != n) {
			for (k=l-1;k<n;k++) scale += fabs(a[i][k]);
			if (scale != 0.0) {
				for (k=l-1;k<n;k++) {
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l-1];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l-1]=f-g;
				for (k=l-1;k<n;k++) rv1[k]=a[i][k]/h;
				for (j=l-1;j<m;j++) {
					for (s=0.0,k=l-1;k<n;k++) s += a[j][k]*a[i][k];
					for (k=l-1;k<n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l-1;k<n;k++) a[i][k] *= scale;
			}
		}
		anorm=max(anorm,(fabs(w[i])+fabs(rv1[i])));
	}
	for (i=n-1;i>=0;i--) {
		if (i < n-1) {
			if (g != 0.0) {
				for (j=l;j<n;j++)
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<n;j++) {
					for (s=0.0,k=l;k<n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=min(m,n)-1;i>=0;i--) {
		l=i+1;
		g=w[i];
		for (j=l;j<n;j++) a[i][j]=0.0;
		if (g != 0.0) {
			g=1.0/g;
			for (j=l;j<n;j++) {
				for (s=0.0,k=l;k<m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<m;j++) a[j][i] *= g;
		} else for (j=i;j<m;j++) a[j][i]=0.0;
		++a[i][i];
	}
	for (k=n-1;k>=0;k--) {
		for (its=0;its<30;its++) {
			flag=true;
			for (l=k;l>=0;l--) {
				nm=l-1;
				if (fabs(rv1[l])+anorm == anorm) {
					flag=false;
					break;
				}
				if (fabs(w[nm])+anorm == anorm) break;
			}
			if (flag) {
				c=0.0;
				s=1.0;
				for (i=l;i<k+1;i++) {
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if (fabs(f)+anorm == anorm) break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s = -f*h;
					for (j=0;j<m;j++) {
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) {
				if (z < 0.0) {
					w[k] = -z;
					for (j=0;j<n;j++) v[j][k] = -v[j][k];
				}
				break;
			}
			if (its == 29) { gaprnt (0,"ERROR: NR svdcmp: no convergence in 30 iterations"); return (1);}
			// nrerror("no convergence in 30 svdcmp iterations");
			x=w[l];
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0;
			for (j=l;j<=nm;j++) {
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g=g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=0;jj<n;jj++) {
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z;
				if (z) {
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=0;jj<m;jj++) {
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}
	free_vector(rv1);	// bjt
	return (0);
}

void svdvar(double **v, int ma, double w[], double **cvm)
{
	int k,j,i;
	double sum,*wti;
	wti=vector(ma);
	for (i=0;i<ma;i++) {
		wti[i]=0.0;
		if (w[i] != 0.0) wti[i]=1.0/(w[i]*w[i]);
	}
	for (i=0;i<ma;i++) {
		for (j=0;j<i+1;j++) {
			sum=0.0;
			for (k=0;k<ma;k++)
				sum += v[i][k]*v[j][k]*wti[k];
			cvm[j][i]=cvm[i][j]=sum;
		}
	}
	free_vector(wti);
}

#define TOL 1.0e-5


int svdfitb(double x[], double y[], double sig[], int ndata, double a[], int ma,
	double **u, double **v, double w[], double *chisq,
	void (*funcs)(double, double [], int))
{
	int j,i,rc;
	double wmax,tmp,thresh,sum,*b,*afunc;

	b=vector(ndata);
	afunc=vector(ma);

	for (i=0;i<ndata;i++) {
		(*funcs)(x[i],afunc,ma);
		tmp=1.0/sig[i];
		for (j=0;j<ma;j++) u[i][j]=afunc[j]*tmp;
		b[i]=y[i]*tmp;
	}
	rc=svdcmpb(u,ndata,ma,w,v);
	if (rc) return(rc);
	wmax=0.0;
	for (j=0;j<ma;j++)
		if (w[j] > wmax) wmax=w[j];
	thresh=TOL*wmax;
	for (j=0;j<ma;j++)
		if (w[j] < thresh) w[j]=0.0;
	svbksb(u,w,v,ndata,ma,b,a);
	*chisq=0.0;
	for (i=0;i<ndata;i++) {
		(*funcs)(x[i],afunc,ma);
		sum=0.0;
		for (j=0;j<ma;j++) sum += a[j]*afunc[j];
		*chisq += (tmp=(y[i]-sum)/sig[i],tmp*tmp);
	}
	free_vector(afunc);
	free_vector(b);
	return (0);
}

#undef TOL


int polintb(double xa[], double ya[], int n, double x, double *y, double *dy) {
//  Polynomial interpolaton and extrapolation 
//	Ben-Jei Tsuang
//	2005/02/02: modify from numerical recepies
//
	int i,m,ns=0;
	double den,dif,dift,ho,hp,w;
	double *c,*d;
	c=(double *)malloc(sizeof(double)*n);
	d=(double *)malloc(sizeof(double)*n);
	if (!c||!d) { 
		sprintf(pout,"ERROR: Out of memory in polint!\n");gaprnt (1,pout);
		if (NULL!=c) free(c);
		if (NULL!=d) free(d);
		return (1);
	}
	dif=fabs(x-xa[0]);
	for (i=0;i<n;i++) {
		if ((dift=fabs(x-xa[i])) < dif) {
			ns=i;
			dif=dift;
		}
		c[i]=ya[i];
		d[i]=ya[i];
	}
	*y=ya[ns--];
	for (m=1;m<n;m++) {
		for (i=0;i<n-m;i++) {
			ho=xa[i]-x;
			hp=xa[i+m]-x;
			w=c[i+1]-d[i];
			if ((den=ho-hp) == 0.0) { gaprnt (0,"ERROR: NR polintb: xa must be distinct"); return (1);}
			den=w/den;
			d[i]=hp*den;
			c[i]=ho*den;
		}
		*y += (*dy=(2*(ns+1) < (n-m) ? c[ns+1] : d[ns--]));
	}
	if (NULL!=c) free(c);
	if (NULL!=d) free(d);
	return (0);	
}



//#include "nr.h"
//#include "nrutil.h"
#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;
//#define M 7
//#define NSTACK 50

int sort2b(int n, double arr[], double brr[])
{
	const int M=7;
	int i,ir,j,k,jstack=-1,l=0,nstack=50,*istack;
	double a,b,temp;
//	nstack=(int)max(50,(int)2.*log((double)n)/log(2.));
	istack=lvector(nstack);
	ir=n-1;
	for (;;) {
		if (ir-l < M) {
			for (j=l+1;j<=ir;j++) {
				a=arr[j];
				b=brr[j];
				for (i=j-1;i>=l;i--) {
					if (arr[i] <= a) break;
					arr[i+1]=arr[i];
					brr[i+1]=brr[i];
				}
				arr[i+1]=a;
				brr[i+1]=b;
			}
			if (jstack < 0) break;
			ir=istack[jstack--];
			l=istack[jstack--];
		} else {
			k=(l+ir) >> 1;
			SWAP(arr[k],arr[l+1]);
			SWAP(brr[k],brr[l+1]);
			if (arr[l] > arr[ir]) {
				SWAP(arr[l],arr[ir]);
				SWAP(brr[l],brr[ir]);
			}
			if (arr[l+1] > arr[ir]) {
				SWAP(arr[l+1],arr[ir]);
				SWAP(brr[l+1],brr[ir]);
			}
			if (arr[l] > arr[l+1]) {
				SWAP(arr[l],arr[l+1]);
				SWAP(brr[l],brr[l+1]);
			}
			i=l+1;
			j=ir;
			a=arr[l+1];
			b=brr[l+1];
			for (;;) {
				do i++; while (arr[i] < a);
				do j--; while (arr[j] > a);
				if (j < i) break;
				SWAP(arr[i],arr[j]);
				SWAP(brr[i],brr[j]);
			}
			arr[l+1]=arr[j];
			arr[j]=a;
			brr[l+1]=brr[j];
			brr[j]=b;
			jstack += 2;
			if (jstack >= nstack) {
				sprintf(pout,"ERROR: sort2b:nstack too small in sort2.\n");
				gaprnt (0,pout);
//				nrerror("nstack too small in sort2.");
				return (1);
			}
			if (ir-i+1 >= j-l) {
				istack[jstack]=ir;
				istack[jstack-1]=i;
				ir=j-1;
			} else {
				istack[jstack]=j-1;
				istack[jstack-1]=l;
				l=i;
			}
		}
	}
	free_lvector(istack);
	return (0);
}

//#undef M
//#undef NSTACK
#undef SWAP

void spline(double x[], double y[], int n, double yp1, double ypn, double y2[])
{
	int i,k;
	double p,qn,sig,un,*u;

	u=vector(n-1);

	if (yp1 > 0.99e30)
		y2[0]=u[0]=0.0;
	else {
		y2[0] = -0.5;
		u[0]=(3.0/(x[1]-x[0]))*((y[1]-y[0])/(x[1]-x[0])-yp1);
	}
	for (i=1;i<n-1;i++) {
		sig=(x[i]-x[i-1])/(x[i+1]-x[i-1]);
		p=sig*y2[i-1]+2.0;
		y2[i]=(sig-1.0)/p;
		u[i]=(y[i+1]-y[i])/(x[i+1]-x[i]) - (y[i]-y[i-1])/(x[i]-x[i-1]);
		u[i]=(6.0*u[i]/(x[i+1]-x[i-1])-sig*u[i-1])/p;
	}
	if (ypn > 0.99e30)
		qn=un=0.0;
	else {
		qn=0.5;
		un=(3.0/(x[n-1]-x[n-2]))*(ypn-(y[n-1]-y[n-2])/(x[n-1]-x[n-2]));
	}
	y2[n-1]=(un-qn*u[n-2])/(qn*y2[n-2]+1.0);
	for (k=n-2;k>=0;k--) y2[k]=y2[k]*y2[k+1]+u[k];
	free_vector(u);	
}
int splintb(double xa[], double ya[], double y2a[], int n, double x, double *y)
{
	int k,rc;
	double h,b,a;

	int klo=0;
	int khi=n-1;

	while (khi-klo > 1) {
		k=(khi+klo) >> 1;
		if (xa[k] > x) khi=k;
		else klo=k;
	}
	h=xa[khi]-xa[klo];
	if (h == 0.0) {
		sprintf(pout,"ERROR: splintb: xa's must be distinct!\n");
		gaprnt (0,pout);
		return(1);
	}
	a=(xa[khi]-x)/h;
	b=(x-xa[klo])/h;
	*y=a*ya[klo]+b*ya[khi]+((a*a*a-a)*y2a[klo]
		+(b*b*b-b)*y2a[khi])*(h*h)/6.0;
	return(0);
}

void locate(double xx[], int n, double x, int *j)
{
	int ju,jm,jl;
	int ascnd;

	jl=-1;
	ju=n;
	ascnd=(xx[n-1] >= xx[0]);
	while (ju-jl > 1) {
		jm=(ju+jl) >> 1;
		if (x >= xx[jm] == ascnd)
			jl=jm;
		else
			ju=jm;
	}
	if (x == xx[0]) *j=0;
	else if (x == xx[n-1]) *j=n-2;
	else *j=jl;
}



#undef NRANSI

