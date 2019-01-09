      subroutine ftngrhilo( fld, undef, xi, yi, ni, nj, 
     $     maxmin, ipcntile,sortby,nsigdig,
     $     latc,lonc,radinf,
     $     opoints,
     $     npoints,nvars,nrr,
     $     rc)

      USE gex
      USE trig_vals

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)


c         
c         routine to find all critical points in a 2-d scalar grid (fld) 
C
C         assumes lon (xi) and lat (yi) are univariate (can deal with gaussian grids though)
C         
C         uses stirling's formula for locating extrema between points - 
C         returns value of gradient and laplacian at critical point
C         3rd order bessel interpolator to get the value at extrema
C         
C         find distance in nm between the extreme and latc,lonc
C         and sorts the output by value distance, grad, lapl and percentile
C         
C


C         local work arrays
C
      real(kind=iGaKind) ::  acl(npoints),xcl(npoints),ycl(npoints),
     $     gcl(npoints),lcl(npoints),dcl(npoints)
      real(kind=iGaKind) ::  ach(npoints),xch(npoints),ych(npoints),
     $     gch(npoints),lch(npoints),dch(npoints)
      real(kind=iGaKind) ::  lonc,latc,radinf

      character*1 hll(npoints)
      character*1 hlh(npoints)

      integer ndxl(npoints),odxl(npoints)
      integer ndxh(npoints),odxh(npoints)

      integer maxmin,rc

C
C         input arrays
C
      real(kind=iGaKind) ::  fld(ni,nj), xi(ni), yi(nj)

C
C         output array
C
      real(kind=iGaKind) ::  opoints(nvars,npoints)

      real(kind=iGaKind) :: laplout,laploutval,gradout,gradoutval
      real(kind=iGaKind) :: gchmax,gclmax,lhlmax,lclmax
      real(kind=iGaKind) :: ipcntile,pcntile

      real(kind=iGaKind) :: eps

      real(kind=iGaKind) :: p0,p1,p2,p3,p4,p5,p6,p7,p8,p0l,p0h
      real(kind=iGaKind) :: r0,s0,dr,ds
      real(kind=iGaKind) :: rihl,rjhl
      real(kind=iGaKind) :: ahl,xhl,yhl,ghl,lhl,dhl,distout

      character hl*1
      character sortby*1
      character oformat*24,cval*24

      logical io_tc,hi,lo,tough,
     $     hi_closed1,lo_closed1,hi_closed2,lo_closed2

      logical doprecfilt

C         
C         settings for precision filter...
C         only if nsigdig is 2-7
C         
      sigdigfact=5.0
      maxcycle=10
      if(nsigdig.gt.1.and.nsigdig.lt.8) then
        doprecfilt=.true.
      else
        doprecfilt=.false.
      endif

      doprecfilt=.false.

      rc=0

      iunit=6
      oformat='g10.4'

      if(ipcntile.gt.100.0) ipcntile=100.0
      if(ipcntile.lt.0.0) ipcntile=0.0

      pcntile=ipcntile*0.01


C         
C         control over output H/L 
C         

      tough=.true.
      eps=1e-7

      gclmax=-9e20
      ghlmax=-9e20
      lhlmax=-9e20
      lclmax=-9e20

      iallundefined=1
C         
C check if undefined         
C         
      do i=1,ni
        do j=1,nj
          if(fld(i,j).ne.undef) then
            iallundefined=0
            exit
          endif
        enddo 
      enddo

      if(iallundefined.eq.1) then
        print*,'EEEE all points undefined, sayoonara.. '
        rc=2
        return
      endif



C         
C***      search for critical fld
C         
      nhl=0
      nh=0
      nl=0

      fldmax=-1e20
      fldmin=1e20

C         
C ... precision filter -- knocks down noise from grib, etc...         
C
      if(doprecfilt) then

        do j=2,nj-1
          do i=2,ni-1
            if(fld(i,j).gt.fldmax) fldmax=fld(i,j)
            if(fld(i,j).lt.fldmin) fldmin=fld(i,j)
          end do
        end do

        fldrange=fldmax-fldmin
        if(fldrange.eq.0.0) then
          fldrange=1e-5
        endif

C         
C taken from qprntn code in ftn_libmf.F         
C
        if(abs(fldmax).lt.1.0e-32.or.fldmax.eq.0.0) xm=99.0

        xm=log10((10**(nsigdig-1)-1.0)/fldmax)
        kp=int(xm)
        if(xm.lt.0.0) kp=kp-1
        fk=10.0**kp
        
CC        print*, 'MMMMMMM --------------- ',nsigdigo,fldmax,xm,kp,fk
CC        print*, 'MMMMM ',fldmax,fldmax*fk
        
      endif
Ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
C         
C         cycling loop
C
      ncycle=1

 200  continue

      nall=0
      do j=2,nj-1
        do i=2,ni-1
          nall=nall+1

c         
c****     check for a critical point
c         
          i0=i
          j0=j
          i0p1=i0+1
          i0m1=i0-1
          j0p1=j0+1
          j0m1=j0-1

          if(i0p1.gt.ni) i0p1=1
          if(i0m1.lt.1) i0m1=ni
          
          p0=fld(i0,j0)
C         
C         check for undefined point, bomb out
C         
          if(p0.eq.undef) go to 100 

          p0l=p0-p0*eps
          p0h=p0+p0*eps
          
          p3=fld(i0,j0p1)
          p7=fld(i0,j0m1)
          p1=fld(i0p1,j0)
          p5=fld(i0m1,j0)
          
          if(
     $         (p1.eq.undef).or.
     $         (p3.eq.undef).or.
     $         (p5.eq.undef).or.
     $         (p7.eq.undef)
     $         )
     $         go to 100 

ccc       write(*,'(2i5,5(1pe20.7))') i,j,p0,p3,p7,p1,p5

          p2=fld(i0p1,j0p1)
          p4=fld(i0p1,j0m1)
          p6=fld(i0m1,j0p1)
          p8=fld(i0m1,j0m1)

          if(
     $         (p2.eq.undef).or.
     $         (p4.eq.undef).or.
     $         (p6.eq.undef).or.
     $         (p8.eq.undef)
     $         )
     $         go to 100 
c         
c         test if a relative hi or lo using two levels of "closedness"
c         
          lo_closed1=.false.
          lo_closed2=.false.
          hi_closed1=.false.
          hi_closed2=.false.

          lo_closed1=p0l.lt.p1.and.p0l.lt.p3.and.
     1         p0l.lt.p5.and.p0l.lt.p7.and.
     2         p0l.lt.p2.and.p0l.lt.p4.and.
     3         p0l.lt.p6.and.p0l.lt.p8

          hi_closed1=p0h.gt.p1.and.p0h.gt.p3.and.
     1         p0h.gt.p5.and.p0h.gt.p7.and.
     2         p0h.gt.p2.and.p0h.gt.p4.and.
     3         p0h.gt.p6.and.p0h.gt.p8

          if(.not.tough) then
            lo_closed2=p0l.lt.p1.and.p0l.lt.p3.and.
     1           p0l.lt.p5.and.p0l.lt.p7

            hi_closed2=p0h.gt.p1.and.p0h.gt.p3.and.
     1           p0h.gt.p5.and.p0h.gt.p7
          endif


          if(tough) then 
            hi=hi_closed1
            lo=lo_closed1
          else
            hi=hi_closed2
            lo=lo_closed2
          endif

          if(hi.or.lo) then
            
            ghl=sqrt((p1-p5)*(p1-p5)+(p3-p7)*(p3-p7))

            r0=0.5*(p1-p5)
            dr = -(p1-2.0*p0+p5)
            if(dr.ne.0.0) then        
              r0=r0/dr
            else
              r0 = 0.0
            endif

            s0=0.5*(p3-p7)
            ds= -(p3-2.0*p0+p7)
            if(ds.ne.0.0) then
              s0=s0/ds
            else
              s0 = 0.0
            endif
            
            lhl=sqrt(dr*dr+ds*ds)

C         
C         precsion filter
C

            if(doprecfilt) then

              p0diff=abs(4.0*p0l-p1-p5-p3-p7)
              sizdiff=p0diff*fk
              if(sizdiff.lt.sigdigfact)  then

CCC       print*,'LLLLLLLLLLLLLLLL ghl lhl',sghl,slhl,p0diff,fk,sizdiff,nhl,nall
C         
C         cycle to next point
C
                cycle     
              endif

            endif

c         
c****     use sterling's formula to locate the critical
c****     point between grid points
c         
c         dr and ds are the 2nd derivatives in x and y
c         
c         ghl is the mag of the gradient
c
ccc       write(*,'(a,4i5,5(1pe20.7))') cc
ccc     $           'qqqq ',nh,nl,i,j,p0,p3,p7,p1,p5

            rihl=i0+r0
            rjhl=j0+s0

ccc            print*,'xi...',xi(i0m1),xi(i0),xi(i0p1)
ccc            print*,'i0 ',i0m1,i0,i0p1,r0,p5,p0,p1
ccc            print*,'yi...',yi(j0m1),yi(j0),yi(j0p1)
ccc            print*,'j0 ',j0m1,j0,j0p1,s0,p7,p0,p3
           
            if(r0.ge.0.0) then
              xhl= xi(i0) + r0*(xi(i0p1)-xi(i0))
            else
              xhl= xi(i0) + r0*(xi(i0)-xi(i0m1))
            endif

            if(s0.ge.0.0) then
              yhl= yi(j0) + s0*(yi(j0p1)-yi(j0))
            else
              yhl= yi(j0) + s0*(yi(j0)-yi(j0m1))
            endif
            
c         
c****     find the value of the max using 2-d bessel order interp
c         

            call bssl5(rihl,rjhl,fld,ni,nj,ahl,undef)

C--       distance from center point (e.g., TC)
C         

            dhl=distsp(yhl,xhl,latc,lonc)

C--       check if inside radinf
C
            if((radinf.gt.0.0).and.(dhl > radinf)) go to 100

C--       if passes radinf test increment counters
C
            if(hi) nh=nh+1
            if(lo) nl=nl+1
            nhl=nhl+1  

            if(nhl.ge.npoints) then
              sigdigfact=sigdigfact*2.0
              ncycle=ncycle+1
              print*,'doubling sigdigfact to ',sigdigfact,' ncycle: ',ncycle
              if(ncycle.gt.maxcycle) then
                print*,'EEEEEE too many points stop, npoints: ',
     $               nhl,npoints,nsigdig,sigdigfact
                rc=1
                return
              endif
              go to 200
            endif



            if(hi) hlh(nh)='H'
            if(lo) hll(nl)='L'

            if(hi) then
              if(ghl.gt.ghlmax) ghlmax=ghl
              if(lhl.gt.lhlmax) lhlmax=lhl
              ach(nh)=ahl
              xch(nh)=xhl
              ych(nh)=yhl
              gch(nh)=ghl
              lch(nh)=lhl
              dch(nh)=dhl
            endif

            if(lo) then
              if(ghl.gt.gclmax) gclmax=ghl
              if(lhl.gt.lclmax) lclmax=lhl
              acl(nl)=ahl
              xcl(nl)=xhl
              ycl(nl)=yhl
              gcl(nl)=ghl
              lcl(nl)=lhl
              dcl(nl)=dhl
            endif

c         
          end if

          

 100      continue

        end do        

      end do 
      

      ie=ichar_len(oformat,24)

CCCC      write(iunit,'(a,i3,1x,i3,1x,4(g10.4,1x))') 'GGG ',nh,nl,rmgh,rmlh,rmgl,rmll

      nrr=0

      if(gclmax.le.0.0) gchmax=-9e-10
      if(ghlmax.le.0.0) ghlmax=-9e-10

      if(lclmax.le.0.0) lchmax=-9e-10
      if(lhlmax.le.0.0) lhlmax=-9e-10

      if(nh.ge.1) then

        if(nh.gt.1) then
          if(sortby.eq.'m') call indexx(nh,ach,ndxh)
          if(sortby.eq.'g') call indexx(nh,gch,ndxh)
          if(sortby.eq.'l') call indexx(nh,lch,ndxh)
          if(sortby.eq.'t') call indexx(nh,ych,ndxh)
          if(sortby.eq.'n') call indexx(nh,xch,ndxh)
          if(sortby.eq.'d') then
            call indexx(nh,dch,odxh)
            call ireverse(nh,odxh,ndxh)
          endif


        else
          ndxh(1)=1
        endif
      
        nn=nint(float(nh)*pcntile)+1
        if(nn.le.0) nn=1
C         
C         force output of one point; always cycle through loop once
C
        if(nn.ge.nh) nn=nh

        do ii=nh,nn,-1
          cval='                        '
          valout=ach(ndxh(ii))
          xout=xch(ndxh(ii))
          yout=ych(ndxh(ii))
          gradout=(gch(ndxh(ii))/ghlmax)*100.0
          laplout=(lch(ndxh(ii))/lhlmax)*100.0
          gradoutval=gch(ndxh(ii))
          laploutval=lch(ndxh(ii))
          distout=dch(ndxh(ii))

          if((oformat(1:1).eq.'i').or.(oformat(1:1).eq.'I')) then
            ival=nint(valout)
            write(cval,'('//oformat(1:ie)//')') ival
          else
            write(cval,'('//oformat(1:ie)//')') valout
          endif

CC          write(iunit,
CC     $         '(a,a1,1x,a,1x,2(f10.2,1x),1x,2(g10.4,1x))') 
CC     $         'GGG ',hlh(ndxh(ii)),cval,xout,yout,
CC     $         gradout,laplout

          
          if(maxmin.ge.0) then

            nrr=nrr+1
            opoints(1,nrr)=1.0
            opoints(2,nrr)=valout
            opoints(3,nrr)=yout
            opoints(4,nrr)=xout
            opoints(5,nrr)=gradout
            opoints(6,nrr)=laplout
            opoints(7,nrr)=distout
            opoints(8,nrr)=gradoutval
            opoints(9,nrr)=laploutval

          endif

        end do
      endif

      if(nl.ge.1) then

        if(nl.gt.1) then

          if(sortby.eq.'m') then
            call indexx(nl,acl,odxl)
            call ireverse(nl,odxl,ndxl)
          endif
          if(sortby.eq.'g') call indexx(nl,gcl,ndxl)
          if(sortby.eq.'l') call indexx(nl,lcl,ndxl)
          if(sortby.eq.'t') call indexx(nl,ycl,ndxl)
          if(sortby.eq.'n') call indexx(nl,xcl,ndxl)
          if(sortby.eq.'d') then
            call indexx(nl,dcl,odxl)
            call ireverse(nl,odxl,ndxl)
          endif

        else
          ndxl(1)=1
        endif

        nn=nint(float(nl)*pcntile)
        if(nn.le.0) nn=1
C         
C         force output of one point
C         
        if(nn.gt.nl) nn=nl-1
        
        do ii=nl,nn,-1

          val=acl(ndxl(ii))
          valout=acl(ndxl(ii))
          xout=xcl(ndxl(ii))
          yout=ycl(ndxl(ii))
          gradout=(gcl(ndxl(ii))/gclmax)*100.0
          laplout=(lcl(ndxl(ii))/lclmax)*100.0
          gradoutval=gcl(ndxl(ii))
          laploutval=lcl(ndxl(ii))
          distout=dcl(ndxl(ii))

          if((oformat(1:1).eq.'i').or.(oformat(1:1).eq.'I')) then
            ival=nint(val)
            write(cval,'('//oformat(1:ie)//')') ival
          else
            write(cval,'('//oformat(1:ie)//')') val
          endif

CC          write(iunit,
CC     $         '(a,a1,1x,a,1x,2(f10.2,1x),1x,2(g10.4,1x))') 
CC     $         'GGG ',hll(ndxl(ii)),cval,xcl(ndxl(ii)),ycl(ndxl(ii)),
CC     $         gcl(ndxl(ii)),lcl(ndxl(ii))


          if(maxmin.le.0) then

            nrr=nrr+1
            opoints(1,nrr)=-1.0
            opoints(2,nrr)=valout
            opoints(3,nrr)=yout
            opoints(4,nrr)=xout
            opoints(5,nrr)=gradout
            opoints(6,nrr)=laplout
            opoints(7,nrr)=distout
            opoints(8,nrr)=gradoutval
            opoints(9,nrr)=laploutval

          endif
            
        end do

      endif
C         
C         return the # of points found
C         
      

      return
      end

