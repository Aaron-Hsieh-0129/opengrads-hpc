      subroutine ftnuv2trw(u, v, lat, lon, dum, undef, 
     $     ni, nj, centlat, centlon, 
     $     iout, rc)

      USE gex
      USE const

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      real(kind=iGaKind) ::  u(ni,nj),v(ni,nj)
      real(kind=iGaKind) ::  dum(ni,nj)
      real(kind=iGaKind) ::  lon(ni),lat(nj)
      
      integer rc

C         initialize dum to input field
C

      do i=1,ni
        xlon=lon(i)

        do j=1,nj

          xlat=lat(j)
          udat=u(i,j)
          vdat=v(i,j)

          call getvrvt (centlon,centlat,xlon,xlat,
     $         udat,vdat,
     $         vr,vt,uvr,vvr,uvt,vvt,
     $         igvtret)

          if(iout == 3) dum(i,j)=vvt
          if(iout == 2) dum(i,j)=uvt
          if(iout == 1) dum(i,j)=vt

          if(iout == -1) dum(i,j)=vr
          if(iout == -2) dum(i,j)=uvr
          if(iout == -3) dum(i,j)=vvr

        enddo
      enddo
      
      do i=1,ni
        do j=1,nj
          u(i,j)=dum(i,j)
        end do
      end do

      rc=0
      return
      end



c
c-----------------------------------------------------------------------
c
c-----------------------------------------------------------------------
      subroutine getvrvt (centlon,centlat,xlon,xlat,
     &     udat,vdat,
     &     vr,vt,uvr,vvr,uvt,vvt,
     &     igvtret)
c
c     ABSTRACT: This subroutine takes as input a u-wind and v-wind value
c     at an input (xlon,xlat) location and returns the tangential and
c     radial wind components relative to the input center lat/lon 
c     position (centlon,centlat).  The only trick to this whole 
c     subroutine is figuring out the angle from the center point to the
c     data point, and we do this by creating a triangle with the leg 
c     from the center point to the data point being the hypotenuse.
c
c     NOTE: All longitudes must be in positive degrees east (0-360) !!!
c
c     INPUT:
c     centlon  Longitude of center point
c     centlat  Latitude  of center point
c     xlon     Longitude of pt at which vr & vt will be computed
c     xlat     Latitude  of pt at which vr & vt will be computed
c     udat     u-value of wind at the point (xlon,xlat) 
c     vdat     v-value of wind at the point (xlon,xlat) 
c
c     OUTPUT:
c     vr      Radial wind component at (xlon,xlat) wrt (centlon,centlat)
c     vt      Tang   wind component at (xlon,xlat) wrt (centlon,centlat)
c     igvtret Return code from this subroutine

      USE const
      USE gex

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      logical verb

      verb=.false.
CCC      verb=.true.

      call calcdist1(centlon,centlat,xlon,xlat,hyp_dist)

      xlatdiff = abs(centlat - xlat)
      xlondiff = abs(centlon - xlon)

      
      if (xlondiff == 0 .and. xlatdiff > 0) then

        if (centlat > xlat) psi = 180   ! pt directly south of ctr
        if (centlat < xlat) psi = 0     ! pt directly north of ctr

      else if (xlondiff > 0 .and. xlatdiff == 0) then

        if (centlon > xlon) psi = 270   ! pt directly west of ctr
        if (centlon < xlon) psi = 90    ! pt directly east of ctr
C         
C         exactly on the center
C
      else if (hyp_dist ==  0) then

        vr=0.0
        vt=0.0
        uvr=0.0
        vvr=0.0
        uvt=0.0
        vvt=0.0
        return

      else

        ! This next part figures out the angle from the center point
        ! (centlon,centlat) to the data point (xlon,xlat).  It does 
        ! this by setting up a triangle and then using inverse trig
        ! functions to get the angle.  Since this is a kludgy way to
        ! do it that doesn't account for the curvature of the earth,
        ! we'll do it 2 ways, using asin and then acos, then take the
        ! average of those 2 for the angle.  hyp_dist, calculated just
        ! above, is the distance from the center pt to the data pt.

        opp_dist  = xlatdiff/360. * earthcircum
        sin_value = opp_dist / hyp_dist

        if (sin_value > 1.0) then

          sin_value=1.0

          if(verb) then
            print *,' '
            print *,'           !!! In getvrvt, sin_value > 1, setting to 1.'
            print *,'           !!! opp_dist= ',opp_dist,' hyp_dist= ',hyp_dist
            print *,'           !!! sin_value = ',sin_value
            print *,'           !!! centlon= ',centlon,' centlat= ',centlat
            print *,'           !!! xlon=    ',xlon,' xlat=    ',xlat
            print *,' '
          endif

        endif

        sin_angle = asin(sin_value) * rad2deg

        call calcdist1(centlon,centlat,xlon,centlat,adj_dist)
        cos_value = adj_dist / hyp_dist

        if (cos_value > 1.0) then

          cos_value = 1.0

          if(verb) then
            print *,' '
            print *,'           !!! In getvrvt, cos_value > 1, setting to 1.'
            print *,'           !!! adj_dist= ',adj_dist,' hyp_dist= ',hyp_dist
            print *,'           !!! cos_value = ',cos_value
            print *,'           !!! centlon= ',centlon,' centlat= ',centlat
            print *,'           !!! xlon=    ',xlon,' xlat=    ',xlat
            print *,' '
          endif

        endif

        cos_angle = acos(cos_value) * rad2deg

        theta = 0.5 * (sin_angle + cos_angle)

        ! The previous lines of code just calculated an angle between
        ! 0 and 90.  This next if structure adjusts that angle to 
        ! instead be between 0 and 360.

        if      (centlat <= xlat .and. centlon <= xlon) then
          psi = 90 - theta
        else if (centlat >  xlat .and. centlon <= xlon) then
          psi = 90 + theta
        else if (centlat >= xlat .and. centlon >= xlon) then
          psi = 270 - theta
        else if (centlat <  xlat .and. centlon >= xlon) then
          psi = 270 + theta
        endif

      endif

      psi=psi*deg2rad
      uvrcomp = udat * sin(psi)
      vvrcomp = vdat * cos(psi)
      vr      = uvrcomp + vvrcomp

      uvtcomp = (-udat) * cos(psi)
      vvtcomp = vdat    * sin(psi)
      vt      = uvtcomp + vvtcomp

      vvr = vr * cos (psi)
      uvr = vr * sin (psi)
    
      vvt = -vt * cos (psi + pi*0.5)
      uvt = -vt * sin (psi + pi*0.5)

      return 
      end


      subroutine calcdist1(rlonb,rlatb,rlonc,rlatc,xdist)

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
c     !!! NOTE !!! -- THE PARAMETER earthcircum IS DEFINED (AS OF THE 
c     ORIGINAL WRITING OF THIS SYSTEM) IN KM, NOT M, SO BE AWARE THAT
c     THE DISTANCE RETURNED FROM THIS SUBROUTINE IS ALSO IN KM.
c         
      USE gex
      USE const
      

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      if (rlatb < 0.0 .or. rlatc < 0.0) then
        pole = -90.
      else
        pole = 90.
      endif

      distlatb = (pole - rlatb) * deg2rad
      distlatc = (pole - rlatc) * deg2rad
      difflon  = abs( (rlonb - rlonc)*deg2rad )

      cosanga = ( cos(distlatb) * cos(distlatc) + 
     &            sin(distlatb) * sin(distlatc) * cos(difflon))
 
c     This next check of cosanga is needed since I have had ACOS crash
c     when calculating the distance between 2 identical points (should
c     = 0), but the input for ACOS was just slightly over 1
c     (e.g., 1.00000000007), due to (I'm guessing) rounding errors.

      if (cosanga > 1.0) then
        cosanga = 1.0
      endif

      degrees    = acos(cosanga) / deg2rad
      circ_fract = degrees / 360.
      xdist      = circ_fract * earthcircum
c
c     NOTE: whether this subroutine returns the value of the distance
c           in km or m depends on the scale of the parameter earthcircum. 
c           At the original writing of this subroutine (7/97), earthcircum
c           was given in km.
c
      return
      end
