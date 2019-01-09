!
!    Copyright (C) 2007 by Arlindo da Silva <dasilva@opengrads.org>
!    All Rights Reserved.
!
!    This program is free software; you can redistribute it and/or modify
!    it under the terms of the GNU General Public License as published by
!    the Free Software Foundation; using version 2 of the License.
!
!    This program is distributed in the hope that it will be useful,
!    but WITHOUT ANY WARRANTY; without even the implied warranty of
!    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
!    GNU General Public License for more details.
!
!    You should have received a copy of the GNU General Public License
!    along with this program; if not, please consult  
!              
!              http://www.gnu.org/licenses/licenses.html
!
!    or write to the Free Software Foundation, Inc., 59 Temple Place,
!    Suite 330, Boston, MA 02111-1307 USA
!
!-------------------------------------------------------------------------
!
!
!...........................................................................

!BOP
!
! !INTERFACE:

       subroutine ftnFish ( lons, lats, im, jm, mbdcnd, amiss, div, 
     .                      velp, rc )

       implicit NONE

! !INPUT PARAMETERS:

       integer   im, jm       ! zona/latitudinal dimensions
       real*8    lons(im)     ! min/max longitude in degrees
       real*8    lats(jm)     ! min/max latitude  in degrees
       integer   mbdcnd       ! meridional baoundary condition, see fishpak:
                              !  = 1  solution specified at both poles 
                              !  = 5  solution specified at TF (South Pole) and
                              !  = 7  solution specified at TS (North Pole) and
                              !  = 9  solution unspecified at both poles
                              !  if unsure, specify "9".
       real*8    amiss        ! missing value
       real*8    div(im,jm)   ! vorticity or divervence

! !OUTPUT PARAMETERS:

       real*8   velp(im,jm)   ! When input is vorticity, velp is the 
                              !  streamfunction; when input is divergence,
                              !  velp is the velocity potential.
       integer rc             ! error code; 0 means all is well
!
! !DESCRIPTION:
!
! Interface to fishpak. Given vorticity or stream function on am A-grid,
! this function returns velocity potential or streamfunction, respectively.
! It is assumed a global periodic lat/lon grid. On input, div and velp
! may share storage.
!
! !REVISION HISTORY:
!
!  26Jun2007  da Silva  Loosely based on classic UDF from GEOS functions.
!
!EOP
!--------------------------------------------------------------------------

!     Automatic arrays
!     ----------------
      real*8 vp(jm*(im+1))
      real*8 w(11*jm+6*(im+1))
      real*8 bdtf(im+1), bdts(im+1)
      real*8 bdps(jm), bdpf(jm)

      integer i, j, jj, imp, iwk, k, nundef
      real*8 dlon, rlon
      real*8 pi, d2r, tol
      logical xwrap ! whether first longitude point is already repeated

      integer intl, m, n, nbdcnd, idimf
      real*8 ts, tf, ps, pf, rad, elmbda, pertrb

      integer spnum, npnum
      real*8  spval, npval, accum


!     Statement function
!     ------------------
      real*8 x
      logical UNDEF
              UNDEF(x) = abs(x-amiss) .le. abs(amiss)*tol


!     For now, requires 
!       1) periodic longitudes
!       2) latitudes: better if in [-90,+90], otherwise
!          solution set to zero at lats(1) and lats(jm).
!     -----------------------------------------------------
      pi   = 3.14159267
      d2r  = pi / 180.
      dlon = (lons(im) - lons(1))/float(im)
      rlon = lons(im) - lons(1)
      tol  = 0.01

#ifdef DEBUG
      print *, 'ftnFish: im, jm ', im, jm
      print *, 'ftnFish: dlon  = ', dlon
      print *, 'ftnFish: mbdcnd = ', mbdcnd
      print *, 'ftnFish: rlon  = ', rlon, rlon+dlon
      print *, 'ftnFish: amiss = ', amiss
      print *, 'ftnFish: lons  = ', (lons(i), i=1,im)
      print *, 'ftnFish: lats  = ', (lats(j), j=1,jm)
#endif

!     Are longitudes periodic? If so, are they wrapped?
!     -------------------------------------------------
      if ( abs(rlon-360.) .lt. dlon/2 ) then
           xwrap = .true.  ! periodic, but first longitude point 
                           ! is already repeated at the end

      else if ( abs(rlon+dlon-360.) .lt. dlon/2 ) then
           xwrap = .false.  ! periodic with first point NOT repeated

      else
            rc = 1000       ! not periodic: we cannot handle it.
            return
      endif


#ifdef DEBUG
      print *, 'ftnFish: xwrap = ', xwrap
#endif

!     Previously hardwired parameters
!     -------------------------------
      imp = im + 1
      iwk = 11 * jm + 6 * (im+1)

!     Initialize work space to zero, just in case
!     --------------------------------------------
      do i = 1, iwk
         w(i) = 0.0
      end do
      do j = 1, jm
         bdps(j) = 0.0
         bdpf(j) = 0.0
      end do
      do i = 1, im+1
         bdtf(i) = 0.0
         bdts(i) = 0.0
      end do

#ifdef DEBUG
      nundef = 0
      do j = 1, jm
         do i = 1, im
            if (undef(div(i,j))) nundef = nundef + 1
         end do
      end do
      print *, 'ftnFish: # undefs BEFORE fix = ', nundef
#endif

!      Cope with undef at the poles: replace with zonal mean of nearest
!      latitude band if pole has undef
!      ----------------------------------------------------------------
       call fixpole ( div, lats, im, jm, 1,     2, amiss ) ! South pole
       call fixpole ( div, lats, im, jm, jm, jm-1, amiss ) ! North pole

!     Try to cope with GrADS undefs coming from hcurl and hdivg
!      at the boundaries
!     ---------------------------------------------------------
       do j = 1, jm             ! remove undefs at extreme longitudes
          if( UNDEF ( div(1,j)  ) ) div(1 ,j)  = div(2,j)
          if ( xwrap ) then
             if( UNDEF ( div(im,j) ) ) div(im,j)  = div(2,j) ! keep it periodic
          else
             if( UNDEF ( div(im,j) ) ) div(im,j)  = div(im-1,j)
          end if
       end do

#ifdef DEBUG
      nundef = 0
      do j = 1, jm
         do i = 1, im
            if (undef(div(i,j))) nundef = nundef + 1
         end do
      end do
      print *, 'ftnFish: # undefs AFTER  fix = ', nundef
#endif

!     Transform the input array to the storage scheme expected
!      by Fishpak: arrays are transposed and latitudes are flipped.
!      Notice that undefs are replaced by zeros: how strange! 
!      TO DO: consider some form of filling in instead,
!      like a Cressman/Barnes' scheme.
!     --------------------------------------------------------
      do j = 1, jm
         jj = jm - j + 1        ! flip latitudes (fishpak uses co-latitude)
         do i = 1, im
            k = jj + (i-1) * jm
            if( .not. UNDEF ( div(i,j) ) ) then
               vp(k) = div(i,j)
            else
               vp(k) = 0.0
            end if
         end do
         vp(jj+im*jm) = vp(jj)  ! add an extra longitude column
      end do
    
!     ----------------------------------------------------------------
!     NOTE: no need to repeat first longitude when it is xwrapped, but 
!           there is no harm either, so we don't bother
!     ----------------------------------------------------------------

!     Prepare other inputs for Fishpak
!       co-latitude = 90 - latitude
!     --------------------------------
      RAD    = 6371000.0 ! radius of the earth
      INTL   = 0         ! tell Fishpak to initialize itself
                         ! Range of co-latitudes
      TS     = d2r*(90-lats(jm)) !  start co-latitude, 0 is north pole
      TF     = d2r*(90-lats(1))  !  end   co-latitude, pi is south pole
      M      = JM-1      ! number or co-latitude "layers"

      PS     = 0.0       !  first longitude
      PF     = 2. * pi   !  last one: notice that "vp" repeats firs/last
                         !   longitude point
      if ( xwrap ) then
         N      = IM-1   !  number of longiude "layers" - already x wrapped
      else
         N      = IM     !  number of longiude "layers" 
      endif

      NBDCND = 0         ! indicates that the solution is periodic 
                         !  in longitude 
      ELMBDA = 0         ! Reduces the general Helmholtz equation to Poisson's
      IDIMF  = M+1

      PERTRB = 0.0       ! this is really output

!     Solve the poisson equation
!     --------------------------
      CALL PWSSSP ( INTL,TS,TF,M,MBDCND,BDTS,BDTF,PS,PF,N,NBDCND,BDPS,
     .              BDPF,ELMBDA,VP,IDIMF,PERTRB,rc,W)

#ifdef DEBUG
      print *, 'ftnFish: PERTRB = ', PERTRB
#endif

      if ( rc .ne. 0 ) return

!     Transform the output array
!     --------------------------
      do j = 1, jm
         jj = jm - j + 1 ! flip latitudes (fishpak uses co-latitude)
         do i = 1, im
            velp(i,j) = vp(jj+(i-1)*jm) * rad * rad
         end do
      end do

      return
      end

      subroutine fixpole ( div, lats, im, jm, jp, jn, amiss)
      implicit NONE
      integer im, jm
      integer jp  ! pole index
      integer jn  ! next to pole index
      real*8 div(im,jm), lats(jm)
      real*8 amiss
      real*8 tol

      integer i, j, npts
      real*8 accum
      logical need_fix

      real*8 x
      logical UNDEF
              UNDEF(x) = abs(x-amiss) .le. abs(amiss)*tol

      tol  = 0.01
      need_fix = .false.
      do i = 1, im
         if ( UNDEF(div(i,jp)) ) need_fix = .true.
      end do

      if ( need_fix ) then

!        True pole
!        ---------
         if ( abs((abs(lats(jp))-90.)) .le. tol ) then
            accum = 0.0
            npts = 0 
            do i = 1, im
               if ( .not. UNDEF(div(i,jn) ) ) then
                  accum = accum + div(i,jn)
                  npts = npts + 1
               end if
            end do
            if ( npts .gt. 0 ) then
               accum = accum / npts
            else            
               accum = amiss
            end if
            do i = 1, im
               div(i,jp) = accum
            end do

!        Not really the pole, nonetheless...
!        -----------------------------------
         else
            do i = 1, jm
               if ( UNDEF(div(i,jp)) ) div(i,jp) =  div(i,jn)
            end do
         end if

      end if

      return
      end
