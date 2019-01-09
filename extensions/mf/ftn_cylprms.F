      subroutine ftncylprms( 
     $     lonc,latc,radinf
     $     ,fld, undef, xi, yi, ni, nj 
     $     ,opoints,nr,nv
     $     )


      USE trig_vals
      USE grid_bounds

      implicit none

C         
C         input from libmf.c
C         
      real(kind=8) ::  fld(ni,nj), xi(ni), yi(nj)
      real(kind=8) :: undef

C         
C         gettrk_main.f vars
C

      real(kind=8) :: lonc,latc,cvalb,dval,xvalb,dist
      real(kind=8) :: cxi,cyj
      real(kind=8) :: head,dhead,radinf,radinfnm,elat,elon
      real(kind=8) :: maxcgrad,mincgrad,meancgrad
      real(kind=8) :: fldmean

      integer nradii

      integer, parameter :: iunit_diag=6
      integer n,ni,nj,nr,nv
      integer i,j,jj

C         
C         output
C         
      real(kind=8) ::  opoints(nv,nr)

      pi = 4. * atan(1.)   ! Both pi and dtr were declared in module
      dtr = pi/180.0       ! trig_vals, but were not yet defined.

C         
C         convert radinf to nm for rhumb line
C
      
      radinfnm=(radinf/dtk)*60.0

C         
C         get field value at center point lonc/latc using bessel interpolation         
C

      call lonlat2ij(lonc,latc,xi,yi,ni,nj,undef,cxi,cyj)
      call bssl5(cxi,cyj,fld,ni,nj,cvalb,undef)

      maxcgrad=-9e20
      mincgrad=9e20
      meancgrad=0.0
      
C         
C         find gradient from radinf -> center (dr) 
C         every dhead deg
C         

      head=0.0
      dhead=30.0
      nradii=0
      do while(head <= 360.0)
        call rcaltln (latc,lonc,head,radinfnm,elat,elon)
        call lonlat2ij(elon,elat,xi,yi,ni,nj,undef,cxi,cyj)
        call bssl5(cxi,cyj,fld,ni,nj,xvalb,undef)
        dval=(xvalb-cvalb)/radinf
        if(dval > maxcgrad) maxcgrad=dval
        if(dval <= mincgrad) mincgrad=dval
        meancgrad=meancgrad+dval
cccc        print*,head,elat,elon,xvalb,dval
        head=head+dhead
        nradii=nradii+1
      end do
      
      if(nradii > 0) then
        meancgrad=meancgrad/nradii
      endif

C         
C         return the max/min/mean gradient          
C

      opoints(1,1)=maxcgrad
      opoints(2,1)=mincgrad
      opoints(3,1)=meancgrad
      
C         
C         field mean inside radinf
C         

      fldmean=0.0
      n=0
      do i=1,ni
        do j=1,nj
          call calcdist(lonc,latc,xi(i),yi(j),dist)
          if(dist <= radinf) then
            fldmean=fldmean+fld(i,j)
            n=n+1
cccc            print*,'qqqq ',i,j,dist,fld(i,j)
          endif

        enddo
      enddo

      if(n>0) then
        fldmean=fldmean/float(n)
      else
        fldmean=undef
      endif

      opoints(1,2)=fldmean

      return
      end

