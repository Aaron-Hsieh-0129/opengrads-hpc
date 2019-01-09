#include "orb.h"

       subroutine orb_getsat(sat,sat_name,rc)
       integer, intent(in)  :: sat
       integer, intent(out) :: rc
       character(len=*), intent(out) :: sat_name
          
             if ( sat == AQUA     ) then
                                         sat_name = "Aqua"
       else  if ( sat == CALIPSO  ) then
                                         sat_name = "Calipso"
       else  if ( sat == CLOUDSAT ) then
                                         sat_name = "CloudSat"
       else  if ( sat == AURA     ) then
                                         sat_name = "Aura"
       else  if ( sat == TERRA    ) then
                                         sat_name = "Terra"
       else
             rc = 99
             return
       end if
       rc = 0
       end

!...........................................................................

       subroutine ftntrack (lons, lats, nobs, nmax, sat, nymd, nhms, dt, rc )

       use orbits_mod

       implicit NONE
       include "gex.inc"

! !INPUT PARAMETERS:

       integer, intent(in) :: nmax    ! size of output arrays
       integer, intent(in) :: sat     ! Satellite name
       integer, intent(in) :: nymd(2) ! Beginning/ending date: YYYYMMDD
       integer, intent(in) :: nhms(2) ! Beginning/ending time: HHMMSS
       integer, intent(in) :: dt      ! deltat (in secs)

! !OUTPUT PARAMETERS:

       integer, intent(out) :: nobs     ! number of lons/lats
       real(kind=iGaKind), intent(out) :: lons(nmax), lats(nmax) ! track coordinates
       integer, intent(out)   :: rc    ! Error return code
                                       ! = 0  all is well
                                       ! = 1  nobs is too small
                                       ! = 3  memory allocation error

!                               ----

       INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)

!      Local workspace
!      ---------------
       character(len=32) :: sat_name

       integer i
       real(dp), pointer :: tlons(:), tlats(:) ! local work space


       call orb_getsat(sat,sat_name,rc)
       if ( rc /= 0 ) return

!       Compute tracks
!       --------------
        call orbits_track(tlons,tlats, sat_name, nymd, nhms, dt, rc)
        if ( rc /= 0 ) then
             rc = -rc
             return
        end if

        nobs = min(size(tlons),size(tlats))
        if ( nobs > nmax ) then
!!!             print *, 'nobs, nmax: ', nobs, nmax
             rc = 1
        else
           do i = 1, nobs
              lons(i) = tlons(i)
              lats(i) = tlats(i)
           end do
        endif
        deallocate(tlons,tlats)

!      All done
!      --------
       end

!...........................................................................

       subroutine ftnmasking (field, im, jm, lons, lats, undef, 
     &                        sat, nymd, nhms, dt, swath, 
     &                        ihalo, jhalo, rc )

       use orbits_mod

       implicit NONE
       include "gex.inc"

! !INPUT PARAMETERS:

       integer, intent(in)            :: im, jm
       integer, intent(in)            :: ihalo(2), jhalo(2)
       real(kind=iGaKind), intent(in) :: lons(im)
       real(kind=iGaKind), intent(in) :: lats(jm)
       real(kind=iGaKind), intent(in) :: swath(3)
       real(kind=iGaKind), intent(in) :: undef

       integer, intent(in) :: sat     ! Satellite name
       integer, intent(in) :: dt      ! delta t in secs
       integer, intent(in) :: nymd(2) ! Beginning/ending date: YYYYMMDD
       integer, intent(in) :: nhms(2) ! Beginning/ending time: HHMMSS

! !OUTPUT PARAMETERS:

       real(kind=iGaKind), intent(inout) :: field(im,jm)
       integer, intent(out)   :: rc    ! Error return code
                                       ! = 0  all is well
                                       ! = 3  memory allocation error

!                               ----

       INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)

!      Local workspace
!      ---------------
       character(len=32) :: sat_name
       integer :: isegs, jsegs, segs, mask(im,jm)  
       real(dp)::  V_mean, xlons(im), xlats(jm)
       real(dp) :: SwathWidth(2) = (/ 0.0, 0.0 /)

       integer i, nobs
       real(dp), pointer :: tlons(:)   => null()
       real(dp), pointer :: tlats(:)   => null()
       real(dp), pointer :: slons(:,:) => null()
       real(dp), pointer :: slats(:,:) => null()

       call orb_getsat(sat,sat_name,rc)

!      Convert to native floats
!      ------------------------
       xlons(:) = lons(:)       
       xlats(:) = lats(:)

       SwathWidth(1:2) = swath(1:2) ! type conversion

!      Interpoaltion parameters; swath(3) is the swath resolution, in km
!      -----------------------------------------------------------------
       V_mean = 7.5             ! Mean sat speed in km/s; assumes a 90 minute period
                                ! this should be a function of satellite.
       isegs = nint((swath(1)+swath(2)) / swath(3) ) ! segments across-track
       jsegs =  V_mean * dt / swath(3)               ! segments along-track

!      Simple masking without swath
!      ----------------------------
       if ( swath(1) .eq. 0 .AND. swath(2) .eq. 0 ) then

!         Compute tracks
!         --------------
          call orbits_track(tlons,tlats, sat_name, nymd, nhms, dt, rc)

!         Simple masking
!         --------------
          call orb_mask(mask,im,jm,lons,lats,tlons,tlats,size(tlons),jsegs)

          deallocate(tlons,tlats)

       else

!         Compute tracks
!         --------------
          call orbits_swath(slons,slats, sat_name, nymd, nhms, dt, 
     &                      SwathWidth, rc, wrap=.false.)

#ifdef DEBUG
          print *, 'isegs, jsegs, swath --> ', isegs, jsegs, swath
#endif

!         Swath masking
!         -------------
          nobs = size(slons)/3
          call orb_swath_mask(mask,im,jm,lons,lats,slons,slats,nobs,
     .                        isegs,jsegs)


          deallocate(slons,slats)

       end if

!      Add halo to the mask
!      --------------------
       if ( (sum(ihalo) + sum(jhalo)) > 0 ) then
          call orb_halo(mask,im,jm,lons,ihalo,jhalo)
       end if

!      Apply mask to field
!      -------------------
       where ( mask /= 1 ) field = undef

!      All done
!      --------
       end

!...........................................................................................

#if 1

      integer function ijsearch(coords,idim,value,periodic) ! fast bisection version
            implicit NONE
            INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)
            include "gex.inc"
            integer, intent(in) :: idim
            real(kind=iGaKind), intent(in) :: coords(idim)
            real(dp), intent(inout) :: value
            logical, intent(in)  :: periodic
            integer i, i1, i2, k
            if ( periodic ) then
                 if ( value>coords(idim) ) value = value - 360.
            endif
            if ( value .eq. coords(idim) ) then
                 ijsearch = idim
                 return
            endif
            ijsearch = -1
            i1 = 1
            i2 = idim 
            do k = 1, idim  ! it should never take take long
               i = (i1 + i2) / 2
               if ( (value .ge. coords(i)) ) then
                  if (value .lt. coords(i+1) ) then
                    ijsearch = i
                    exit
                  else
                    i1 = i
                  end if
               else
                    i2 = i
               endif
            end do
      end function

#else

      integer function ijsearch(coords,idim,value,periodic) ! simple, but slow version
           implicit NONE
            INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)
            include "gex.inc"
            integer, intent(in) :: idim
            real(kind=iGaKind), intent(in) :: coords(idim)
            real(dp), intent(inout) :: value
            logical, intent(in)  :: periodic
            integer i
            ijsearch = -1
            if ( periodic ) then
                 if ( value>coords(idim) ) value = value - 360.
            endif
            do i = 1, idim-1
               if ( (value .ge. coords(i)) .AND. (value .lt. coords(i+1)) ) then
                    ijsearch = i
                    exit
               endif
            end do
            if ( value .eq. coords(idim) ) ijsearch = idim
      end function

#endif

      subroutine orb_edges(ecoords,coords,idim)
      implicit NONE
      include "gex.inc"
      integer, intent(in) :: idim
      real(kind=iGaKind), intent(in)  :: coords(idim)
      real(kind=iGaKind), intent(out) :: ecoords(idim+1)
      ecoords(1)  = coords(1) - 0.5 * ( coords(2) - coords(1) ) 
      ecoords(2:idim) = 0.5 * ( coords(1:idim-1)+coords(2:idim) )
      ecoords(idim+1) = coords(idim) + 0.5 * (coords(idim) - coords(idim-1))      
      return
      end
!...........................................................................................

      subroutine orb_mask(mask,im,jm,lons,lats,tlons,tlats,nobs,jsegs)
      implicit NONE
      include "gex.inc"
      INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)

      integer, intent(in) :: im, jm, nobs, jsegs
      real(kind=iGaKind), intent(in) :: lons(im)
      real(kind=iGaKind), intent(in) :: lats(jm)


      real(kind=iGaKind) :: elons(im+1), elats(jm+1)
      real(dp) :: tlons(nobs), tlats(nobs) 
      real(dp) :: beta, d2r, lon, dx, dy, lat, tol = 0.1
      integer, intent(out) :: mask(im,jm)

     
      integer i, j, m, n, nfail, ijsearch
      logical periodic

!     Determine if grid is periodic
!     -----------------------------
      d2r = atan(1.0) / 45.
      dx = cos(d2r*lons(1))-cos(d2r*(2.0*lons(im)-lons(im-1)))
      dy = sin(d2r*lons(1))-sin(d2r*(2.0*lons(im)-lons(im-1)))
      if ( sqrt(dx**2 + dy**2) < tol ) then
           periodic = .true.
      else                             
           periodic = .false.
      endif

!     Build edge coords
!     -----------------
      call orb_edges(elons,lons,im)
      call orb_edges(elats,lats,jm)

      mask = 0
      nfail = 0
      do n = 2, nobs
         do m = 1, jsegs        ! along track refinement
            beta = (m - 1.0 ) / ( jsegs - 1.0 )
            if ( abs(tlons(n)-tlons(n-1))<90. ) then
               lon = (1.0-beta) * tlons(n-1) + beta * tlons(n)
            else if ( tlons(n) > tlons(n-1) ) then
               lon = (1.0-beta) * tlons(n-1) + beta * (tlons(n)-360.)
            else
               lon = (1.0-beta) * (tlons(n-1)-360.) + beta * tlons(n)
            end if
            lat = (1.0-beta) * tlats(n-1) + beta * tlats(n)
            if ( (lons(im) > 180.) .AND. (lon<0.) ) then
               lon = lon + 360.
            endif
            i = ijsearch(elons,im+1,lon,periodic)
            j = ijsearch(elats,jm+1,lat,.false.)
            if ( i>0 .and. j>0 ) then
               mask(i,j) = 1
            else 
#ifdef DEBUG
               print *, 
     .        'WARNING: Failed to assign (i,j) for these (lon,lat): ', 
     .              lon, lat
               nfail = nfail + 1
#endif
            end if
            !!! if ( n>1 ) call dist(d2r*tlons(n-1),d2r*tlats(n-1),d2r*tlons(n),d2r*tlats(n),dt)
            
         end do ! m
      end do ! n
#ifdef DEBUG
      if ( nfail > 0 ) print *, 
     .        'WARNING: Failed to assign (i,j) for ', nfail, ' points'
#endif
      end


!--
      subroutine dist(lon1,lat1,lon2,lat2,dt)
      INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)
      real(dp) :: lon1,lat1,lon2,lat2
      integer  :: dt
      real *8 dx, dy, dz, V, pi
      pi = 4.0 * atan(1.0)
      dx = cos(lat1) * sin(lon1) - cos(lat2) * sin(lon2)
      dy = cos(lat1) * cos(lon1) - cos(lat2) * cos(lon2)
      dz = sin(lat1) - sin(lat2)
      a = 6371.
      V = 2. * pi * a / ( 90. * 60 ) ! assumes a period of 90 minutes
      ds = a * sqrt(abs(dx*dx + dy*dy + dz*dz))
      
      print *, 'lon, lat, ds, ds* --> ', lon1, lat1, ds, V * dt
      end
 
!...........................................................................................

      subroutine orb_swath_mask(mask,im,jm,lons,lats,slons,slats,nobs,
     .                          isegs, jsegs)
      implicit NONE
      include "gex.inc"
      INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)

      integer, intent(in) :: im, jm, nobs, isegs, jsegs
      real(kind=iGaKind), intent(in) :: lons(im)
      real(kind=iGaKind), intent(in) :: lats(jm)


      real(kind=iGaKind) :: elons(im+1), elats(jm+1)
      real(dp) :: slons(3,nobs), slats(3,nobs) 
      real(dp) :: alpha, beta, d2r, lon, lon1, lon2,  lat, lat1, lat2
      real(dp) :: dx, dy, tol = 0.1
      integer, intent(out) :: mask(im,jm)

     
      integer i, j, k, m, n, nfail, ijsearch
      logical periodic

!     Determine if grid is periodic
!     -----------------------------
      d2r = atan(1.0) / 45.
      dx = cos(d2r*lons(1))-cos(d2r*(2.0*lons(im)-lons(im-1)))
      dy = sin(d2r*lons(1))-sin(d2r*(2.0*lons(im)-lons(im-1)))
      if ( sqrt(dx**2 + dy**2) < tol ) then
           periodic = .true.
      else                             
           periodic = .false.
      endif

!     Build edge coords
!     -----------------
      call orb_edges(elons,lons,im)
      call orb_edges(elats,lats,jm)

      mask = 0
      nfail = 0
       
      do k = 1, isegs   ! cros-track
         alpha = (k - 1.0 ) / ( isegs - 1.0 )
         do n = 2, nobs
            lon1 = (1.0-alpha) * slons(1,n-1) + alpha * slons(3,n-1)  
            lon2 = (1.0-alpha) * slons(1,n) + alpha * slons(3,n)  
            lat1 = (1.0-alpha) * slats(1,n-1) + alpha * slats(3,n-1)  
            lat2 = (1.0-alpha) * slats(1,n) + alpha * slats(3,n)  
            if ( abs(lon2-lon1) > 90. ) then
                 if ( lon2 > lon1 ) then
                      lon2 = lon2 - 360.
                 else
                      lon1 = lon1 - 360.
                 end if
            end if
            do m = 1, jsegs ! along track refinement
               beta = (m - 1.0 ) / ( jsegs - 1.0 )
               lon = (1.0-beta) * lon1 + beta * lon2
               lat = (1.0-beta) * lat1 + beta * lat2
               if ( (lons(im) > 180.) .AND. (lon<0.) ) then
                  lon = lon + 360.
               endif
               i = ijsearch(elons,im+1,lon,periodic)
               j = ijsearch(elats,jm+1,lat,.false.)
               if ( i>0 .and. i<=im .and. j>0 .and. j<=jm ) then
                  mask(i,j) = 1
               else 
#ifdef DEBUG
                  print *, 
     .                 'WARNING: Failed to assign (i,j) for ' // 
     .                 'lon ', slons(1,n), lon, slons(3,n), ' | lat ',
     .                 slats(1,n), lat, slats(3,n)
                  nfail = nfail + 1
#endif
               end if
            end do ! msegs
         end do    ! nobs
      end do       ! ksegs
#ifdef DEBUG
      if ( nfail > 0 ) print *, 'WARNING: Failed to assign (i,j) for ', nfail, ' points'
#endif
      end

!...........................................................................................

      subroutine orb_halo(mask,im,jm,lons,ihalo,jhalo)

      implicit NONE
      include "gex.inc"
      INTEGER, PARAMETER :: dp=SELECTED_REAL_KIND(15,307)

      integer, intent(in) :: im, jm, ihalo(2), jhalo(2)
      real(kind=iGaKind), intent(in) :: lons(im)

      integer, intent(inout) :: mask(im,jm)

      real(dp) :: d2r, dx, dy, tol = 0.1
      integer i, j, is_, is, js
      logical periodic
      integer :: tmask(im,jm)

!     Determine if grid is periodic
!     -----------------------------
      d2r = atan(1.0) / 45.
      dx = cos(d2r*lons(1))-cos(d2r*(2.0*lons(im)-lons(im-1)))
      dy = sin(d2r*lons(1))-sin(d2r*(2.0*lons(im)-lons(im-1)))
      if ( sqrt(dx**2 + dy**2) < tol ) then
           periodic = .true.
      else                             
           periodic = .false.
      endif

!     Build halo
!     ----------
#ifdef DEBUG
      print *, 'im, jm, halos: ', im, jm, ihalo, jhalo
#endif
      tmask = 0
      do j = 1, jm
         do i = 1, im
            if ( mask(i,j) .eq. 1 ) then
               do js = j-jhalo(1), j+jhalo(2)
                  if ((js.ge.1) .and. (js.le.jm) ) then
                     do is_ = i-ihalo(1), i+ihalo(2)
                        is = is_
                        if ( periodic ) then
                           if ( is < 1  ) is = im + is
                           if ( is > im ) is = is - im
                        endif
                        if ( (is.ge.1) .and. (is.le.im) ) tmask(is,js) = 1
                     end do     ! is-loop
                  end if        ! js is range
               end do           ! js-loop
            end if              ! (i,j) has mask=1
         end do                 ! i-loop
      end do                    ! j-loop
      mask = tmask
      end
      
