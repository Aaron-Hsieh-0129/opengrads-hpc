c
c-----------------------------------------------------------------------
c
c-----------------------------------------------------------------------
      subroutine find_maxmin (imax,jmax,grdspc,cparm,fxy,maxmin,ist
     &             ,guesslon,guesslat,rlonv,rlatv,valid_pt,compflag
     &     ,ctlon,ctlat,xval,ifmret)


c
c     This routine  finds the location (clon,clat) of and value of the
c     the max or min of fxy in the vicinity of slon,slat.  The value of
c     the input flag maxmin determines whether to look for a max or a
c     min value.  The max/min is determined by finding the point which 
c     gives the max/min value of a single point barnes analysis of fxy 
c     with e-folding radius re (km) and influence radius ri (km). The 
c     initial search is restricted to a radius rads around the point 
c     (slon,slat) on a grid with lon,lat spacing grdspc. The location is
c     refined by reducing the spacing of the search grid by a factor of
c     two, nhalf times.
c
c     INPUT:
c     imax     Num pts in i direction on input grid
c     jmax     Num pts in j direction on input grid
c     grdspc   Grid spacing on input grid
c     cparm    Char string indicating what parm is being passed in
c     fxy      Real array of data values
c     maxmin   Char string indicating whether to search for a max or min
c     ist      Number of the storm being processed
c     guesslon Guess longitude of the storm
c     guesslat Guess latitude of the storm
c     rlonv    Array containing longitude values of input grid points
c     rlatv    Array containing latitude values of input grid points
c     valid_pt Logical bitmap masking non-valid grid points.  This is a
c              concern for the regional models, which are interpolated 
c              from Lam-Conf or NPS grids onto lat/lon grids, leaving 
c              grid points around the edges which have no valid data.
c
c     INPUT/OUTPUT:
c     compflag Logical; continue processing this storm or not (would be
c              set to FALSE if, for example, the guess position is 
c              outside the domain of a regional grid)
c
c     OUTPUT:
c     ctlon    Center longitude of storm found for this parameter
c     ctlat    Center latitude of storm found for this parameter
c     xval     Max or Min value found at the (ctlon,ctlat)
c     ifmret   Return code from this subroutine
c         
      USE gex
      USE radii; USE grid_bounds; USE set_max_parms; USE level_parms
      USE trig_vals

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

c
      real(kind=iGaKind) ::     fxy(imax,jmax),rlonv(imax),rlatv(jmax)
      real(kind=iGaKind) ::    ctlon,ctlat

c
      character(*)  maxmin,cparm
      logical(1)    compflag, valid_pt(imax,jmax)
      integer       igetgds(200)
      integer       bskip,nhalf

      logical verb


      verb=.false.
c 
c
      ifmret = 0
      nhalf = 5
c
c     -----------------------------------------------------------
c     Set initial parms for use in find_maxmin.
c     Different radii used for V magntitude than for other parms, 
c     see discussion in module radii for more details.
c
      if (cparm == 'vmag') then
        rads = rads_vmag; re = retrk_vmag; ri = ritrk_vmag
        re = (float(maxvgrid)/4) * (grdspc * dtk) ! Basically, this
c               sets re equal to half the distance from the gridpoint
c               in question to the farthest point that will be
c               sampled.  Thus, just ignore the parameter retrk_vmag,
c               and use this instead.
      else if (grdspc < 1.26) then
        rads = rads_most; re = retrk_most; ri = ritrk_most
      else 
        rads = rads_coarse; re = retrk_coarse; ri = ritrk_coarse
      endif
c         
      if(verb) then
        print *,' '
        print *,'At beg of find_maxmin, rads= ',rads,' re= ',re 
     &       ,' ri= ',ri,' cparm= ',cparm,' grdspc= ',grdspc
      endif

c
      dell = grdspc
      npts = rads/(dtk*dell)
      fmax  = -1.0e+15; fmin  =  1.0e+15
      ctlon = 0.0; ctlat = 0.0
c
      if (npts == 0) npts = 1

c     For the  barnes analysis, we will want to speed things up for 
c     finer resolution grids.  We can do this by skipping some of 
c     the points in the  barnes analysis.

      if (grdspc > 0.20) then
        bskip = 1
      else if (grdspc > 0.10 .and. grdspc <= 0.20) then
        bskip = 2
      else if (grdspc <= 0.10) then
        bskip = 3
      endif
c
c     If input parm is vmag, we've already done the minimizing by 
c     interpolating to the fine mesh grid, so we'll simply send the 
c     bounds that were input to this subroutine to barnes
c     as boundaries for the array to search.  For all other parms, 
c     however, no minimizing has been done yet, so we need to call 
c     get_ij_bounds to set the boundaries for a much smaller grid that
c     surrounds the storm (as opposed to having subroutine  barnes 
c     search the entire global grid).
c 
      if (cparm == 'vmag') then

        ibeg=1; jbeg=1; iend=imax; jend=jmax
        vmag_latmax = rlatv(1)    ! N-most lat of vmag subgrid
        vmag_latmin = rlatv(jmax) ! S-most lat of vmag subgrid
        vmag_lonmin = rlonv(1)    ! W-most lon of vmag subgrid
        vmag_lonmax = rlonv(imax) ! E-most lon of vmag subgrid

        write (6,44) vmag_latmax,vmag_lonmin,360.-vmag_lonmin,imax,jmax
        write (6,46) vmag_latmin,vmag_lonmax,360.-vmag_lonmax
 44     format (' vmag_latmax= ',f8.3,' vmag_lonmin= ',f8.3
     &         ,'E  (',f8.3,'W)  imax= ',i4,' jmax= ',i4)
 46     format (' vmag_latmin= ',f8.3,' vmag_lonmax= ',f8.3
     &         ,'E  (',f8.3,'W)')

        npts = ceiling(rads/(dtk*dell))

      else

        call get_ij_bounds (npts,0,ri,imax,jmax,grdspc
     &             ,glatmax,glatmin,glonmax,glonmin,guesslon,guesslat
     &             ,ilonfix,jlatfix,ibeg,jbeg,iend,jend,igiret)

        if (igiret /= 0) then
          print *,' '
          print *,'!!! ERROR in find_maxmin from call to get_ij_bounds,'
          print *,'!!! stopping processing for storm number ',ist
          ifmret = 92
          return
        endif

      endif

c
c     ---------------------------------------------------------------
c         
      if(verb) then
        print *,' '
        write (6,39) guesslon,360.-guesslon,guesslat
 39     format (' guesslon= ',f8.3,'E  (',f8.3,'W)   guesslat= ',f8.3)
        print *,'ilonfix= ',ilonfix,' jlatfix= ',jlatfix
     &       ,' npts= ',npts
        if (cparm == 'vmag') then
          print *,'ilonfix and jlatfix are meaningless for computing'
          print *,'vmag, so ignore the large values you see for them.'
        endif
        print *,'ibeg= ',ibeg,' jbeg= ',jbeg,' imax= ',imax
        print *,'iend= ',iend,' jend= ',jend,' jmax= ',jmax
      endif

      ibct=0
      ibarnes_loopct = 0

      jix = 0

      jloop: do j=-npts,npts,bskip

        jix = jix + 1
        rlatt = guesslat + dell*float(j)

        iix = 0

c        vlat(jix) = rlatt

        iloop: do i=-npts,npts,bskip

          iix = iix + 1
          rlont = guesslon + dell*float(i)

c          print '(a12,i2,a4,i2,2(a8,f8.3))','in find_max, i= ',i,' j= '
c     &           ,j,' rlatt= ',rlatt,' rlont= ',rlont
c
c         If any points in the search grid would extend beyond the grid
c         boundaries, then just set the input calcparm to FALSE and 
c         return to calling subroutine without trying to get max/min
c
          if (rlatt > glatmax .or. rlatt < glatmin .or.
     &        rlont > glonmax .or. rlont < glonmin) then
            print *,' '
            print *,'!!! Lat/Lon value outside grid boundary in'
            print *,'!!! subroutine  find_maxmin'
            print *,'!!! rlatt= ',rlatt,' rlont= ',rlont
            print *,'!!! guesslat= ',guesslat,' guesslon= ',guesslon
            print *,'!!! Storm number = ',ist
            print *,'!!! Processing will NOT continue for this storm'
            compflag = .FALSE.
            ifmret = 94
            return
          endif
c
          call calcdist(rlont,rlatt,guesslon,guesslat,dist)
          if (dist .gt. rads) cycle iloop


          if (cparm == 'vmag') then

c           This next bit of code gets the ij coordinates for an 8x8 
c           box around the current point under consideration. These ij
c           coordinates are sent to barnes so that barnes only loops 
c           64 times, as opposed to nearly 10,000 if the whole 97x97
c           array were sent.  So, fix rlatt to the grid point just 
c           northward of rlatt and fix rlont to the grid point just 
c           eastward of rlont.  Note that this makes for a modified 
c           barnes analysis in that we're sort of specifying ahead of
c           time exactly which grid points will be included and we'll
c           be excluding some points that would be near the periphery
c           of each (rlont,rlatt)'s range, but as long as we're consis-
c           tent and do it this way for each point, it's well worth the
c           trade-off in cpu time.  Parameter maxvgrid determines what 
c           size array to send to barnes (maxvgrid=8 means 8x8)

            jvlatfix = int((vmag_latmax - rlatt)/grdspc + 1.)
            ivlonfix = int((rlont - vmag_lonmin)/grdspc + 2.)

            ibeg = ivlonfix - (maxvgrid/2)
            iend = ivlonfix + (maxvgrid/2 - 1)
            jbeg = jvlatfix - (maxvgrid/2 - 1)
            jend = jvlatfix + (maxvgrid/2)

            if (ibeg < 1 .or. jbeg < 1 .or. 
     &          iend > imax .or. jend > jmax) then
              print *,' '
              print *,'!!! Gridpoint in vmag subgrid would be outside'
              print *,'!!! the boundaries in subroutine find_maxmin.'
              print *,'!!! rlatt= ',rlatt,' rlont= ',rlont
              print *,'!!! guesslat= ',guesslat,' guesslon= ',guesslon
              print *,'!!! Storm number = ',ist,' maxvgrid= ',maxvgrid
              print *,'!!! ibeg= ',ibeg,' iend= ',iend,' imax= ',imax
              print *,'!!! jbeg= ',jbeg,' jend= ',jend,' jmax= ',jmax
              compflag = .FALSE.
              ifmret = 94
              return
            endif

          endif

          ibct = ibct + 1

          call barnes(rlont,rlatt,rlonv,rlatv,imax,jmax,ibeg,jbeg
     &         ,iend,jend,fxy,valid_pt,bskip,re,ri,ftemp,icount,
     &         cparm,iret)

          ibarnes_loopct = ibarnes_loopct + icount

          if (iret /= 0) then
            print *,' '
            print *,'!!! Non-zero RCC from barnes...Exiting find_maxmin'
            compflag = .FALSE.
            ifmret = iret
            return
          endif
c
          if (maxmin == 'max') then
            if (ftemp > fmax) then
              fmax = ftemp
              ctlon = rlont
              ctlat = rlatt
            endif
          else
            if (ftemp < fmin) then
              fmin = ftemp
              ctlon = rlont
              ctlat = rlatt
            endif
          endif

        enddo iloop
      enddo jloop

      if(verb) then
        print *,' '
        print *,'After 1st findmax loop, # calls to barnes = ',ibct
        print *,'Total # of barnes loop iterations = ',ibarnes_loopct
c         
 55     format ('i= ',i3,' j= ',i3,'  rln= ',f7.3,'  rlt= ',f7.3
     &       ,'  barnval= ',f11.5)
 56     format ('k= ',i3,' i= ',i3,' j= ',i3,'  rln= ',f7.3,'  rlt= '
     &       ,f7.3,'  barnval= ',f11.5)
c         
        if (cparm == 'zeta') then
          print *,'             !!! Zeta check, fmax= ',fmax,' fmin= ',fmin
          write (6,61) 360.-ctlon,ctlat,fmax*100000.,fmin*100000.
        else
          write (6,63) 360.-ctlon,ctlat,fmin
        endif
 61     format (' After first run, ctlon= ',f8.3,'W  ctlat= ',f8.3
     &       ,' fmax (x10e5) = ',f10.3,' fmin (x10e5) = ',e15.3)
 63     format (' After first run, ctlon= ',f8.3,'W  ctlat= ',f8.3
     &       ,' fmin = ',e15.3)
 111    format (i2,'  rlont= ',f7.2,'W   rlatt= ',f7.2,'  zeta= ',f13.8)

      endif
c         
c         Since through interpolation the grid for vmag has already been
c     minimized considerably, we don't need to go through the 2nd part
c     of this subroutine, which halves the grid spacing.
c
      if (nhalf < 1 .or. cparm == 'vmag') then
        if (maxmin == 'max') then
          xval = fmax
        else
          xval = fmin
        endif
        return
      endif

c     -------------------------------------------------------------
c     If nhalf > 3, check the grid spacing.  If the grid spacing is 
c     fine enough (I've chosen 0.2-deg as a min threshold), there is
c     no need to halve the grid more than 3 times, as halving a 
c     0.2-deg grid 3 times gives a resolution of 0.025-deg (2.7 km),
c     or a max error in the position estimate of 2.7/2 = 1.35 km.

      if (nhalf > 3) then
        if (grdspc <= 0.2) then
          nhalf = 3
        endif
      endif

c     ---------------------------------------------------------------
c     ---------------------------------------------------------------
c     Halve the grid spacing to refine the location and value of the
c     max/min value, but restrict the area of the new search grid.

cTIM TPM     npts = 3
      npts = npts/2

c     -------------------------------------------------------------
c     First, recalculate the i and j beginning and ending points to
c     be used in the  barnes analysis subroutine.  Only
c     do this once for this grid-refinement (even though the grid is
c     redefined 3 times in this subroutine), but make sure to have the
c     possible search grid be big enough to allow the possibility of
c     the grid shifting way right or way left each time through the
c     loop (get_ij_bounds takes care of this).


      call get_ij_bounds (npts,nhalf,ri,imax,jmax,grdspc
     &              ,glatmax,glatmin,glonmax,glonmin,ctlon,ctlat
     &              ,ilonfix,jlatfix,ibeg,jbeg,iend,jend,igiret)

      if (igiret /= 0) then
        print *,' '
        print *,'!!! ERROR in find_maxmin from call to get_ij_bounds'
        print *,'!!! just before nhalf loop.  Stopping processing'
        print *,'!!! for storm number ',ist
        ifmret = 92
        return
      endif
c
c     --------------------------------------------------------------
c     Now do the actual searching for the max/min value 
c

      if (grdspc <= 1.25 .and. ri >= 300 .and. re >= 150) then
        retmp = re
        ritmp = ri
        re = re * 0.5
        ri = ri * 0.5
        if(verb) then
          print *,'After first pass through barnes, re has been reduced'
          print *,'from ',retmp,' to ',re,', and ri has been reduced '
          print *,'from ',ritmp,' to ',ri
        endif
      else
        if(verb) then
          print *,'After first pass through barnes, re and ri have NOT '
          print *,'been changed.  re = ',re,' ri = ',ri
        endif
      endif

      ibct=0
      ibarnes_loopct = 0
      do k=1,nhalf

        dell = 0.5*dell
        tlon = ctlon
        tlat = ctlat
        fmax = -1.0e+15; fmin = 1.0e+15

        if (k < nhalf) then
          iskip = bskip
        else
          iskip = 1
        endif

        if(verb) then
          print *,' '
          print *,'find_maxmin nhalf loop, cparm= ',cparm,
     &            ' k= ',k,' dx: ',dell
          write (6,161) tlon,360.-tlon,tlat
          print *,'ilonfix= ',ilonfix,' jlatfix= ',jlatfix
     &         ,' npts= ',npts
          print *,'ibeg= ',ibeg,' jbeg= ',jbeg,' imax= ',imax
          print *,'iend= ',iend,' jend= ',jend,' jmax= ',jmax
        endif
          
        do j=-npts,npts,iskip

          rlatt = tlat + dell*float(j)

          do i=-npts,npts,iskip

            rlont = tlon + dell*float(i)
c
            if (rlatt > glatmax .or. rlatt < glatmin .or.
     &          rlont > glonmax .or. rlont < glonmin) then
              print *,' '
              print *,'!!! Lat/Lon value outside grid boundary in'
              print *,'!!! subr find_maxmin, in the nhalf loop.'
              print *,'!!! In nhalf loop, k currently = ',k
              print *,'!!! rlatt= ',rlatt,' rlont= ',rlont
              print *,'!!! tlat= ',tlat,' tlon= ',tlon
              print *,'!!! Storm number = ',ist
              compflag = .FALSE.
              ifmret = 94
              return
            endif

            ibct = ibct + 1
            call barnes(rlont,rlatt,rlonv,rlatv,imax,jmax,ibeg,jbeg
     &           ,iend,jend,fxy,valid_pt,bskip,re,ri,ftemp,icount,
     &           cparm,iret)

            ibarnes_loopct = ibarnes_loopct + icount

            if (iret /= 0) then
              print *,' '
              print *,'!!! Non-zero RCC from barnes, k= ',k
              print *,'!!! Exiting find_maxmin'
              compflag = .FALSE.
              ifmret = iret
              return
            endif

            if (maxmin == 'max') then
              if (ftemp > fmax) then
                fmax = ftemp
                ctlon = rlont
                ctlat = rlatt
              endif
            else
              if (ftemp < fmin) then
                fmin = ftemp
                ctlon = rlont
                ctlat = rlatt
              endif
            endif

          enddo
        enddo

        if(verb) then
          if (cparm == 'zeta') then
            write (6,71) k,360.-ctlon,ctlat,fmax*100000.,fmin*100000.
          else
            write (6,73) k,360.-ctlon,ctlat,fmax,fmin
          endif
        endif

      enddo

  71  format (' nhalf findmax, k= ',i2,' ctlon= ',f8.3,'W  ctlat= '
     &       ,f8.3,' fmax (x10e5) = ',f10.3,' fmin (x10e5) = ',e15.3)
  73  format (' nhalf findmax, k= ',i2,' ctlon= ',f8.3,'W  ctlat= ',f8.3
     &       ,' fmax = ',e15.3,' fmin = ',e15.3)

 161  format (' guesslon= ',f8.3,'E  (',f8.3,'W)   guesslat= ',f8.3)
      
      if(verb) then
        print *,' '
        print *,'ppp after 2nd findmax loop, # calls to barnes =  ',ibct
        print *,'ppp Total # of barnes loop iterations = ',ibarnes_loopct
      endif

      if (maxmin == 'max') then
        xval = fmax
      else
        xval = fmin
      endif
c
      return
      end
c
c----------------------------------------------------------------------
c
c----------------------------------------------------------------------
      subroutine get_ij_bounds (npts,nhalf,ri,imax,jmax,grdspc
     &             ,rglatmax,rglatmin,rglonmax,rglonmin,geslon,geslat
     &             ,ilonfix,jlatfix,ibeg,jbeg,iend,jend,igiret)

c
c     -----------------------------------------------------------
c     ABSTRACT: This subroutine figures out, based on ri, grdspc and
c     the guess latitude and longitude positions, the farthest reaching
c     grid points that are searchable by an analysis subroutine.  The
c     purpose is to return indices for a subgrid that is much smaller 
c     than the original, full grid.  This smaller subgrid can then be 
c     passed to a subsequent analysis or interpolation subroutine, and 
c     work can be done on this smaller array.  This can help save time, 
c     especially in the  barnes analysis subroutine, as work will only 
c     be done on, say, a 20 x 20 array (400 pts) instead of on a 
c     360 x 181 array (65160 pts).  It's crucial, however, to make sure 
c     that the ibeg, jbeg, iend and jend are extended far enough out to 
c     fully encompass any search that would be done.  Below is a 
c     diagram showing the different grids....
c
c Full Global or Regional Model Grid  (Grid F) ----------->
c     ----------------------------------------------------------------
c  |  |                            (ibeg,jbeg)                       |
c  |  | x = ij position that the        |      (Grid R)              |
c  |  |     geslat/geslon is fixed to.  ._______________.            |
c  |  |                                 |               |            |
c  |  | Even though only the points     |    (Grid B)   |            |
c  |  | within Grid B will be checked   |   . . . . k   |            |
c  v  | later on for a max/min (in the  |   . . . . .   |            |
c     | case of a subsequent call to    |   . . x . .   |            |
c     | find_maxmin), the  barnes anal- |   . . . . .   |            |
c     | ysis will include all pts sur-  |   . . . . .   |            |
c     | rounding these Grid B points    |               |            |
c     | that are within a radius of ri. ._______________.            |
c     | So in the case of pt. k, that ri                             |
c     | radius may extend all the way to the Grid R     |            |
c     | boundary, thus we need to include those    (iend,jend)       |
c     | points within our ibeg-jbeg-iend-jend bounds.                |
c     |                                                              |
c     ----------------------------------------------------------------
c
c     Remember that the grids we deal with start north and increase 
c     south, so the northernmost latitude on the input grid will have 
c     a j index of 1.
c
c     INPUT:
c     npts     Num pts from x to edge of max/min search grid (Grid B)
c              (i.e., You define the size of Grid B by the value of
c               npts that you pass into this subroutine).
c     nhalf    Number of times the grid spacing will be halved
c     ri       Radius of influence (for use in barnes analysis)
c     imax     Number of points in x-direction on original grid
c     jmax     Number of points in y-direction on original grid
c     grdspc   Input grid spacing for the original grid
c     rglatmax Value of northern-most latitude on original grid
c     rglatmin Value of southern-most latitude on original grid
c     rglonmax Value of eastern-most longitude on original grid
c     rglonmin Value of western-most longitude on original grid
c     geslat   Value of latitude of guess position of storm
c     geslon   Value of longitude of guess position of storm
c
c     OUTPUT:
c     ilonfix  i index on full, input grid that storm is fixed to
c     jlatfix  j index on full, input grid that storm is fixed to
c     ibeg     i index for top left of sub-array (Grid R) of input grid
c     iend     i index for bot right of sub-array (Grid R) of input grid
c     jbeg     j index for top left of sub-array (Grid R) of input grid
c     jend     j index for bot right of sub-array (Grid R) of input grid
c     igiret   Return code from this subroutine
c
      USE gex
      USE trig_vals

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

c
      igiret = 0
c
c     --------------------------------------
c     GET BEGINNING AND ENDING J POINTS....
c
c     (1) Calculate number of searchable, max/min pts, that is, the pts 
c         from x to the edge of Grid B.
c     (2) Calculate number of pts beyond the last search point in Grid 
c         B, but are within the bounds of Grid R and thus can be 
c         included in the  barnes analysis.
c     (3) Add (1) and (2) to get the max number of pts to subtract/add
c         to x to get jbeg and jend.
c
c     If nhalf > 0: This occurs in the case of a call from fmax, when
c     the grid spacing is halved nhalf times.  In this case, we have to
c     do extra work to figure out the maximum possible grid point.  For
c     this case:
c       jhlatpts = # of grid pts to last possible search pt (from x to
c                  edge of Grid B in above diagram), plus a cushion.
c       jripts   = # of searchable grid points within radius ri of last
c                  possible search pt (num pts between edge of Grid B
c                  and edge of Grid R in above diagram), plus a cushion
c       jbmaxlatpts = # of pts (in j direction) from x to the edge of
c                     Grid R to include in this subgrid. 
c
c     If nhalf = 0: In this case, the grid spacing will not be reduced,
c     so the number of pts in j direction from x to the edge of Grid
c     B will be the input parameter npts, and just multiply it by 2,
c     and add 2 for a cushion to get jmaxlatpts.  Typically, this sub
c     is called from find_maxmin, and in that sub, the first time that
c     this sub is called, nhalf will = 0.  Then, after a first-shot
c     center is found, the grid spacing will be cut in order to rerun 
c     barnes on a smaller grid, and that's when nhalf will be sent 
c     here as 3.
c
      if (nhalf > 0) then
        rdeg = 0.0
        do i = 1,nhalf
          rdeg = rdeg + float(npts) * (1./(float(i)*2)) * grdspc 
        enddo
        jhlatpts = ceiling(rdeg/grdspc) + 1
        jripts   = ceiling((ri + 1.)/(dtk*grdspc)) + 1
        jbmaxlatpts = jhlatpts + jripts
      else
        jbmaxlatpts = npts * 2 + 2
      endif
c
c
c     Roughly fix geslat to the grid point just poleward of geslat.
c
      if (geslat >= 0.0) then
        jlatfix = int((rglatmax - geslat)/grdspc + 1.)
      else
        jlatfix = ceiling((rglatmax - geslat)/grdspc + 1.)
      endif
      jbeg = jlatfix - jbmaxlatpts
      jend = jlatfix + jbmaxlatpts
      if (jbeg > jmax ) then
        print *,'!!! ERROR in get_ij_bounds, jbeg > jmax'
        print *,'!!! jbeg = ',jbeg,' jmax= ',jmax
        igiret = igiret + 1
        return
      endif
      if (jend < 1) then
        print *,'!!! ERROR in get_ij_bounds, jend < 1, jend = ',jend
        igiret = igiret + 1
        return
      endif
      if (jbeg < 1) jbeg = 1
      if (jend > jmax) jend = jmax

      ! If using a global grid, avoid using the pole points, or else
      ! you'll get a cosfac = 0 and then a divide by zero!!!

      if (jend == jmax .and. rglatmin == -90.0) then
        jend = jmax - 2
      endif
      if (jbeg == 1    .and. rglatmax == 90.0) then
        jbeg = 3
      endif

c     -----------------------------------------
c     NOW GET BEGINNING AND ENDING I POINTS....
c
c     Figure out what the most poleward latitude is that we could have
c     in this smaller search grid, based on jbeg (NH) or jend (SH).

      if (geslat >= 0.0) then
        platmax = rglatmax - ( float(jbeg - 1) * grdspc )
      else
        platmax = rglatmax - ( float(jend - 1) * grdspc )
      endif

c     Now, using the map factor (cos lat), figure out, based on ri, the
c     max distance beyond the last search point in x-direction (in
c     degrees) that could be searched at this most poleward latitude
c     (i.e., in the diagram above, the max num pts from pt. k eastward
c     to the edge of Grid R).  Calculate how many grid points that is,
c     add 2 to it for a cushion, & add the number of points (npts) 
c     within the defined search grid (Grid B) to get ibmaxlonpts.

      if (platmax >  89.0) platmax =  89.0
      if (platmax < -89.0) platmax = -89.0
      cosfac = cos (platmax * dtr)
      dlon   = ri / (cosfac * dtk)
c
      if (nhalf > 0) then
        ihlonpts    = ceiling(rdeg/grdspc) + 1
        ibmaxlonpts = ihlonpts + ceiling(dlon/grdspc) + 2
      else
        ibmaxlonpts = npts + ceiling(dlon/grdspc) + 2
      endif
c
c     Roughly fix geslon to the grid point just EASTward of geslon.
c
      ilonfix = int((geslon - rglonmin)/grdspc + 2.)
c
      ibeg = ilonfix - ibmaxlonpts
      iend = ilonfix + ibmaxlonpts
      if (ibeg > imax ) then
        print *,'!!! ERROR in get_ij_bounds, ibeg > imax'
        print *,'!!! ibeg = ',ibeg,' imax= ',imax
        igiret = igiret + 1
        return
      endif
      if (iend < 1) then
        print *,'!!! ERROR in get_ij_bounds, iend < 1, iend = ',iend
        igiret = igiret + 1
        return
      endif
      if (ibeg < 1) ibeg = 1
      if (iend > imax) iend = imax 
c
      return
      end
c
c-----------------------------------------------------------------------
c
c-----------------------------------------------------------------------
      subroutine calcdist(rlonb,rlatb,rlonc,rlatc,xdist)


c
c     ABSTRACT: This subroutine computes the distance between two 
c               lat/lon points by using spherical coordinates to 
c               calculate the great circle distance between the points.
c                       Figure out the angle (a) between pt.B and pt.C,
c             N. Pole   then figure out how much of a % of a great 
c               x       circle distance that angle represents.
c              / \
c            b/   \     cos(a) = (cos b)(cos c) + (sin b)(sin c)(cos A)
c            /     \
c        pt./<--A-->\c     NOTE: The latitude arguments passed to the
c        B /         \           subr are the actual lat vals, but in
c                     \          the calculation we use 90-lat.
c               a      \
c                       \pt.  NOTE: You may get strange results if you:
c                         C    (1) use positive values for SH lats AND
c                              you try computing distances across the 
c                              equator, or (2) use lon values of 0 to
c                              -180 for WH lons AND you try computing
c                              distances across the 180E meridian.
c    
c     NOTE: In the diagram above, (a) is the angle between pt. B and
c     pt. C (with pt. x as the vertex), and (A) is the difference in
c     longitude (in degrees, absolute value) between pt. B and pt. C.
c
c     !!! NOTE !!! -- THE PARAMETER ecircum IS DEFINED (AS OF THE 
c     ORIGINAL WRITING OF THIS SYSTEM) IN KM, NOT M, SO BE AWARE THAT
c     THE DISTANCE RETURNED FROM THIS SUBROUTINE IS ALSO IN KM.
c         
      USE gex
      USE trig_vals

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      if (rlatb < 0.0 .or. rlatc < 0.0) then
        pole = -90.
      else
        pole = 90.
      endif

      distlatb = (pole - rlatb) * dtr
      distlatc = (pole - rlatc) * dtr
      difflon  = abs( (rlonb - rlonc)*dtr )

      cosanga = ( cos(distlatb) * cos(distlatc) + 
     &            sin(distlatb) * sin(distlatc) * cos(difflon))
 
c     This next check of cosanga is needed since I have had ACOS crash
c     when calculating the distance between 2 identical points (should
c     = 0), but the input for ACOS was just slightly over 1
c     (e.g., 1.00000000007), due to (I'm guessing) rounding errors.

      if (cosanga > 1.0) then
        cosanga = 1.0
      endif

      degrees    = acos(cosanga) / dtr
      circ_fract = degrees / 360.
      xdist      = circ_fract * ecircum
c
c     NOTE: whether this subroutine returns the value of the distance
c           in km or m depends on the scale of the parameter ecircum. 
c           At the original writing of this subroutine (7/97), ecircum
c           was given in km.
c
      return
      end
c
c----------------------------------------------------------------------
c
c----------------------------------------------------------------------
      subroutine barnes(flon,flat,rlon,rlat,iimax,jjmax,iibeg,jjbeg
     &  ,iiend,jjend,fxy,defined_pt,bskip,re,ri,favg,icount,cparm,iret)

c
c     ABSTRACT: This routine performs a single-pass barnes anaylsis
c     of fxy at the point (flon,flat). The e-folding radius (km)
c     and influence radius (km) are re and ri, respectively.
c
c     NOTE:  The input grid that is searched in this subroutine is most
c     likely NOT the model's full, original grid.  Instead, a smaller
c     subgrid of the original grid is searched.  The upper left and 
c     lower right grid point indices are passed into this subroutine 
c     (iibeg, jjbeg, iiend, jjend) for this subgrid.  These indices are
c     determined in the subroutine  get_ij_bounds, and the purpose of 
c     doing it this way is to limit the number of points for which the
c     subroutine has to calculate distances (for a global 1 deg grid,
c     the number of loop iterations is reduced from 65160 to somewhere
c     around 600).
c
c     NOTE: This subroutine will immediately exit with a non-zero
c     return code if it tries to access a grid point that does not have
c     valid data.  This would happen in the case of a regional grid, if
c     you try to access a point near the edge of the grid (remember that
c     because of the interpolation for the regional grids, there will be
c     areas around the edges that have no valid data).
c
c     INPUT:
c     flon    Lon value for center point about which barnes anl is done
c     flat    Lat value for center point about which barnes anl is done
c     rlon    Array of lon values for each grid point
c     rlat    Array of lat values for each grid point
c     iimax   Max number of pts in x-direction on input grid
c     jjmax   Max number of pts in y-direction on input grid
c     iibeg   i index for grid point to start barnes anlysis (upp left)
c     jjbeg   j index for grid point to start barnes anlysis (upp left)
c     iiend   i index for last grid point in barnes anlysis (low right)
c     jjend   j index for last grid point in barnes anlysis (low right)
c     fxy     Real array of data on which to perform barnes analysis
c     defined_pt Logical; bitmap array used for regional grids
c     bskip   integer to indicate number of grid points to skip during 
c             a barnes loop, in order to speed processing
c     re      input e-folding radius for barnes analysis
c     ri      input influence radius for searching for min/max
c
c     OUTPUT:
c     favg    Average value about the point (flon,flat)
c     iret    Return code from this subroutine
c

      USE gex

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      real(kind=iGaKind) ::  fxy(iimax,jjmax), rlon(iimax), rlat(jjmax)
      integer   bskip
      logical(1) defined_pt(iimax,jjmax)
      character(*) cparm
c
c     --------------------------
c
      res = re*re
      wts = 0.0
      favg = 0.0

      icount = 0

      do j=jjbeg,jjend,bskip
        do i=iibeg,iiend,bskip

          icount = icount + 1

          call calcdist(flon,flat,rlon(i),rlat(j),dist)

          if (dist .gt. ri) cycle

          if (defined_pt(i,j)) then
            wt   = exp(-1.0*dist*dist/res)
            wts  = wts + wt
            favg = favg + wt*fxy(i,j)
          else
            print *,' '
            print *,'!!! UNDEFINED PT OUTSIDE OF GRID IN BARNES....'
            print *,'!!! i= ',i,' j= ',j
            print *,'!!! flon= ',flon,' flat= ',flat
            print *,'!!! rlon= ',rlon(i),' rlat= ',rlat(j)
            print *,'!!! EXITING BARNES....'
            print *,' '
            iret = 95
            return
          endif
 
        enddo
      enddo

      if (wts .gt. 1.0E-5) then
         favg = favg/wts
      else
        favg = 0.0
      endif
      iret = 0
c
      return
      end
