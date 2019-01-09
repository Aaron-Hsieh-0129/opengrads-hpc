      subroutine ftntmhilo( 
     $     guesslon,guesslat,ictype,
     $     fld, undef, xi, yi, ni, nj, 
     $     itype, jtype, ilinr, jlinr,
     $     xbegi, dxi, ybegi,dyi,
     $     maxmin, resopt, radinf,
     $     opoints,nr,nv
     $     )
c
c---------------------------------------------------------------------c
c
c     ABSTRACT: This GrADS UDF scans an array of data values to find
c     the max or min in the vicinity of a user-provided guess position.
c     This program assumes that the user has clicked a position that is
c     close to some type of storm, whether that be represented as a 
c     min in the mslp field, max in the 850 zeta field, etc.  This 
c     program will center its search on that user-provided position.
c
c     Author:    Tim Marchok  (timothy.marchok@noaa.gov)
c     Date:      August, 2005
c     Language:  Fortran90
c
c---------------------------------------------------------------------c
c
      USE gex
      USE trig_vals
      USE grid_bounds

      implicit none

C         
C         input from libmf.c
C         
      real(kind=iGaKind) ::  fld(ni,nj), xi(ni), yi(nj)
      real(kind=iGaKind) :: undef

C         
C         gettrk_main.f vars
C

      real(kind=iGaKind) :: guesslon,guesslat,ctlon,ctlat,xval,cvalb
      real(kind=iGaKind) :: dval,xvalb, vt,vtavg,vr,parmlat,parmlon,parmval
      real(kind=iGaKind) :: pthresh,vthresh
      real(kind=iGaKind) :: xbegi,dxi,ybegi,dyi,grdspc

      real(kind=iGaKind) :: cxi,cyj
      real(kind=iGaKind) :: head,radinf,elat,elon
      real(kind=iGaKind) :: maxcgrad,mincgrad,meancgrad
      integer nradii

      integer, parameter :: iunit_diag=6
      integer  nargs,irvret,iopret,icrret,ifmret
      integer ni,nj,itype, jtype, ilinr, jlinr, imax, jmax, numpts
      integer maxmin,resopt
      integer i,j,jj
      integer ixi,iyi,ixdi
      integer ist,isiret
      integer nr,nv
      integer ictype

      character*1  stormcheck
      character  cmaxmin*3,cparm*4,cres*5
      
      logical compflag
      logical verb

C         
C         output
C         
      real(kind=iGaKind) ::  opoints(nv,nr)
C         
C         arrays         
C

      real(kind=iGaKind),       save, allocatable  ::  xdata_2d(:,:)
      real(kind=iGaKind),       save, allocatable  ::  rlatv(:)
      real(kind=iGaKind),       save, allocatable  ::  rlonv(:)
      logical(1), save, allocatable  ::  valid_pt(:,:)


C-----------------------------------------------main

      
      if(ictype.eq.0) cparm='pmsl'   ! no special treatment
      if(ictype.eq.1) cparm='vmag'   ! wind mag 
      if(ictype.eq.2) cparm='zeta'   ! vorticity

      verb=.false.
      
      pi = 4. * atan(1.)   ! Both pi and dtr were declared in module
      dtr = pi/180.0       ! trig_vals, but were not yet defined.

C         
C         convert to deg lat for rhumb line
C         

      undef  = undef
      itype  = itype
      jtype  = jtype
      imax   = ni
      jmax   = nj
      numpts = ni*nj
      xbegi  = xbegi
      dxi    = dxi
      ybegi  = ybegi
      dyi    = dyi

      if(verb) then
        write (iunit_diag,*) 'undef(1)=       ',undef
        write (iunit_diag,*) 'itype(2)=       ',itype
        write (iunit_diag,*) 'jtype(3)=       ',jtype
        write (iunit_diag,*) 'nx(4)=          ',imax
        write (iunit_diag,*) 'ny(5)=          ',jmax
        write (iunit_diag,*) 'numpts=         ',numpts
        write (iunit_diag,*) 'iscalar flag =  ',ilinr
        write (iunit_diag,*) 'jscalar flag =  ',jlinr
        write (iunit_diag,*) 'xbegi(8)=       ',xbegi
        write (iunit_diag,*) 'dxi(9)=         ',dxi
        write (iunit_diag,*) 'ybegi(10)=      ',ybegi
        write (iunit_diag,*) 'dyi(11)=        ',dyi
        write (iunit_diag,*) 'guesslon        ',guesslon
        write (iunit_diag,*) 'guesslat        ',guesslat
        
      endif

      allocate (xdata_2d(imax,jmax),stat=ixdi)
      allocate (rlonv(imax),stat=ixi)
      allocate (rlatv(jmax),stat=iyi)
      allocate (valid_pt(imax,jmax),stat=ixdi)

      if (ixdi /= 0 .or. ixi /= 0 .or. iyi /= 0) then
        print *,' '
        print *,'!!! ERROR allocating arrays.'
        print *,'!!! ixdi = ',ixdi,' ixi= ',ixi,' iyi= ',iyi
        icrret = 94
        return
      endif
      



C***      do the work of  subroutine conv1d2d_real(kind=iGaKind) :: (inp,icrret) here
C***      yflip for consistency with old gfdl code
c
c     ABSTRACT: This subroutine converts a 1-dimensional input
c     array of real(kind=iGaKind) :: data into a 2-dimensional output array of 
c     real(kind=iGaKind) :: data.  For use with GFDL data, point (1,1) is in 
c     the far SW corner and point (imax,jmax) is in the far
c     NE corner of the grid, but because we are using center-fixing
c     code that I pulled out of my tracker which requires that point
c     (1,1) be in the far NW corner and point (imax,jmax) in the 
c     far SE corner (i.e., flipped), we will flip the arrays in this
c     subroutine.

      do j=1,nj
        jj=nj-j+1
        jj=j
        do i=1,ni
          xdata_2d(i,j)=fld(i,jj)
        end do
      end do

      do i=1,ni
        rlonv(i)=xi(i)
      end do

      do j=1,nj
        jj=nj-j+1
        jj=j
        rlatv(j)=yi(jj)
      end do

      do i=1,ni
        do j=1,nj
          valid_pt(i,j)=.true.
          if(xdata_2d(i,j).eq.undef) valid_pt(i,j)=.false.
        end do
      end do


      if(maxmin >= 0) cmaxmin='max'
      if(maxmin < 0) cmaxmin='min'
      grdspc=dxi

      ist=1

C         
C         set up params in grid_bounds
C

      glatmax=rlatv(jmax)
      glatmin=rlatv(1)

      glonmax=rlonv(imax)
      glonmin=rlonv(1)

      call find_maxmin (imax,jmax,grdspc,cparm,xdata_2d,cmaxmin,ist
     &             ,guesslon,guesslat,rlonv,rlatv,valid_pt,compflag
     &             ,ctlon,ctlat,xval,ifmret)

      call lonlat2ij(ctlon,ctlat,rlonv,rlatv,imax,jmax,undef,cxi,cyj)
      call bssl5(cxi,cyj,fld,imax,jmax,cvalb,undef)
      

      opoints(1,1)=ctlon
      opoints(2,1)=ctlat
      opoints(3,1)=xval
      opoints(4,1)=cvalb
      
      deallocate(xdata_2d); deallocate(rlonv); 
      deallocate(rlatv); deallocate(valid_pt)

      return
      end

