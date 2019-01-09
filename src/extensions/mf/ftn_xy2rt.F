      subroutine ftnxy2rt(fldxy,ni,nj,undef,
     $     xylat,xylon,
     $     clat,clon,
     $     drad,dphi,rmax,
     $     opoints,nr,nv,
     $     chrc,rc
     $     )

      USE gex
      USE const

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      dimension fldxy(ni,nj),xylat(nj),xylon(ni)

      character*1 incard(80),chrc(720)

C         
C         output
C         
      dimension  opoints(nv,nr)

      real(kind=iGaKind), save, allocatable :: r(:),rbm(:),rb2m(:)
      real(kind=iGaKind), save, allocatable :: rbp(:),rb2p(:)

      real(kind=iGaKind), save, allocatable :: phi(:),phibm(:),phibp(:)

      real(kind=iGaKind), save, allocatable :: fldsym(:),fldasym(:,:)

      real(kind=iGaKind), save, allocatable :: fldrt(:,:)
      real(kind=iGaKind), save, allocatable :: latrt(:,:),lonrt(:,:)

      nrad=nint(rmax/drad)+1
      nphi=nint(360.0/dphi)


      allocate (r(nrad),stat=iflag)

      allocate (rbm(nrad),stat=iflag)
      allocate (rb2m(nrad),stat=iflag)

      allocate (rbp(nrad),stat=iflag)
      allocate (rb2p(nrad),stat=iflag)

      allocate (phi(nphi),stat=iflag)

      allocate (phibm(nphi),stat=iflag)
      allocate (phibp(nphi),stat=iflag)

      allocate (fldsym(nrad),stat=iflag)
      allocate (fldasym(ni,nj),stat=iflag)

      allocate (fldrt(nrad,nphi),stat=iflag)
      allocate (latrt(nrad,nphi),stat=iflag)
      allocate (lonrt(nrad,nphi),stat=iflag)


Cxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
C         
C

C         
C         convert dr to nm for rhumb line 
C

C         
C         xy2rt         
C

      do i=1,nrad
        r(i)=(i-1)*drad
        rbm(i)=r(i)-drad*0.5
        rbp(i)=r(i)+drad*0.5
        if(rbm(i).lt.0.0) rbm(i)=0.0
        rb2m(i)=rbm(i)*rbm(i)
        rb2p(i)=rbp(i)*rbp(i)
      enddo

      do j=1,nphi
        phi(j)=(j-1)*dphi
        phibm(j)=phi(j)-dphi*0.5
        phibp(j)=phi(j)+dphi*0.5
      enddo

      rminrti=+1e20
      rmaxrti=-1e20
      rminrtj=+1e20
      rmaxrtj=-1e20

      do i=1,nrad

        fb=0.0
        nb=0

        fldsym(i)=undef

        do j=1,nphi

          call rumlatlon (phi(j),r(i)*km2nm,clat,clon,rlat1,rlon1)
          call lonlat2ij(rlon1,rlat1,xylon,xylat,ni,nj,undef,cxi,cyj)

          if( 
     $         ( (cxi.ge.1.0) .and. (cxi.le.float(ni)) ).and.
     $         ( (cyj.ge.1.0) .and. (cyj.le.float(nj)) )
     $         ) then


            if(cxi.lt.rminrti) rminrti=cxi
            if(cyj.lt.rminrtj) rminrtj=cyj
            if(cxi.gt.rmaxrti) rmaxrti=cxi
            if(cyj.gt.rmaxrtj) rmaxrtj=cyj

            call bssl5(cxi,cyj,fldxy,ni,nj,cvalb,undef)

          else
            cvalb=undef
          endif

          latrt(i,j)=rlat1
          lonrt(i,j)=rlon1
          fldrt(i,j)=cvalb

          if(phi(j).eq.0.0) then
            write(*,'(a,2(i3,1x),2(f8.1,2x),3x,4(f7.2,1x),3f10.2)')
     $           'kkkk ',i,j,r(i),phi(j),rlat1,rlon1,cxi,cyj,cvalb,
     $           fldxy(nint(cxi),nint(cyj))
          endif

          if(cvalb.ne.undef) then
            fb=fb+cvalb
            nb=nb+1
          endif

        end do

        if(nb.gt.0) then
          fldsym(i)=fb/float(nb)
        endif

      end do

      print*,'MMMMM ',rminrti,rmaxrti,rminrtj,rmaxrtj
      print*,'MMMMM ',nrad,nphi,r(nrad),phi(nphi)

      ib=int(rminrti-0.5)
      ie=int(rmaxrti+0.5)
      jb=int(rminrtj-0.5)
      je=int(rmaxrtj+0.5)

      print*,'MMMMM ',ib,ie,jb,je

C         
C   remove symmetric component         
C         

      do i=1,ni
        do j=1,nj
          fldasym(i,j)=fldxy(i,j)
        enddo
      enddo
      

      do i=ib,ie
        rlon=xylon(i)
        do j=jb,je
          rlat=xylat(j)
          call rumdirdist (clat,clon,rlat,rlon,head,dist)
          distkm=dist*rnm2km
          ri=distkm/drad+1.0
          if(distkm.le.r(nrad)) then
CCC            print*,'AAA ',i,j,distkm,int(ri),fldsym(int(ri))
            call bssl1(ri,fldsym,nrad,f1,undef)
          else
            f1=0
          endif
          fldasym(i,j)=fldasym(i,j)-f1
        enddo
      enddo

      do i=1,ni
        do j=1,nj
          fldxy(i,j)=fldasym(i,j)
        enddo
      enddo
      
      
      deallocate (r,stat=iflag)
      deallocate (rbm,stat=iflag)
      deallocate (rb2m,stat=iflag)
      deallocate (rbp,stat=iflag)
      deallocate (rb2p,stat=iflag)
      deallocate (phi,stat=iflag)
      deallocate (phibm,stat=iflag)
      deallocate (phibp,stat=iflag)

      deallocate (fldrt,stat=iflag)
      deallocate (fldsym,stat=iflag)
      deallocate (fldasym,stat=iflag)

      return 
      end


      subroutine crt2ij(r,phi,nrad,nphi,crad,cphi,crti,crtj)

      implicit integer(i-n)
      implicit real*8(a-h,o-z)

      dimension r(nrad),phi(nphi)

      crti=-999.0
      crtj=-999.0

      i=nrad
      do while(i.ge.1)

        if(crad.ge.r(i)) then

          ip1=i+1
          if(ip1.gt.nrad) then
            return
          endif

          radp1=r(ip1)
          dp=(crad-r(i))/(radp1-r(i))
          crti=i*1.0+dp
cccc          print*, 'iiiii ',i,r(i),crad,r(ip1),dp,crti
          i=0
        else
          i=i-1
        endif
      enddo

      j=nphi
      do while (j.ge.1) 
        if(cphi.ge.phi(j)) then
          jp1=j+1
          if(jp1.ge.nphi) jp1=1

          phip1=phi(jp1)
          if(phi(j).gt.180.0.and.phip1.eq.0.0) phip1=360.0

          dp=(cphi-phi(j))/(phip1-phi(j))
          crtj=j*1.0+dp
ccc          print*, 'jjjjj ',j,phi(j),cphi,phip1,dp,crtj
          j=0
        else
          j=j-1
        endif
      enddo

      return
      end

      subroutine bilinrt(fldrt,latrt,lonrt,rlat,rlon,
     $     nrad,nphi,undef,crti,crtj,cval)

      USE gex

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      real(kind=iGaKind) latrt(nrad,nphi),lonrt(nrad,nphi),
     $     lg11,lg21,lg12,lg22,
     $     lt11,lt21,lt12,lt22

      dimension fldrt(nrad,nphi)

      if(crti.eq.-999.or.crtj.eq.-999) then
        cval=undef
        return
      endif

      i1=int(crti)
      i2=i1+1
      
      j1=int(crtj)
      j2=j1+1

      lg11=lonrt(i1,j1)
      lg21=lonrt(i2,j1)

      lg12=lonrt(i1,j2)
      lg22=lonrt(i2,j2)

      lt11=latrt(i1,j1)
      lt21=latrt(i2,j1)

      lt12=latrt(i1,j2)
      lt22=latrt(i2,j2)

      di1=sqrt((lt11-lt21)*(lt11-lt21) + (lg11-lg21)*(lg11-lg21))
      di2=sqrt((lt12-lt22)*(lt12-lt22) + (lg12-lg22)*(lg12-lg22))

      dj1=sqrt((lt11-lt12)*(lt11-lt12) + (lg11-lg12)*(lg11-lg12))
      dj2=sqrt((lt21-lt22)*(lt21-lt22) + (lg21-lg22)*(lg21-lg22))


      dlon1=lg11-lg12
      dlon2=lg21-lg22

C         
C         cyclic continuity in theta
C         

      j2i=j2
      if(j2.gt.nphi) then
        j2i=j2-nphi
        print*,'MMMMMMMMMMmm ',crtj,j2,j2i
      endif


      v11=fldrt(i1,j1)
      v12=fldrt(i1,j2i)
      v21=fldrt(i2,j1)
      v22=fldrt(i2,j2i)

      rfact=crti-float(i1)
      sfact=crtj-float(j1)

      cval=
     $     (1.0-sfact)*( (1.0-rfact)*v11 + rfact*v21 )
     $    +      sfact*( (1.0-rfact)*v12 + rfact*v22 )


      write(*,'(a,4(i3,1x),2(f8.3,1x),8(f6.2,1x),4(f10.2,1x),3(f10.2))') 
      write(*,'(a,4(i3,1x),2(f8.3,1x),4(f10.2,1x),3(f10.2))') 
ccc     $     'BBBB ',crti,crtj,v11,v12,v21,v22,rfact,sfact,cval
     $     'BBBB ',i1,i2,j1,j2,
     $     crti,crtj,
ccc     $     lt11,lg11,lt12,lg12,lt21,lg21,lt22,lg22,
     $     v11,v12,v21,v22,
     $     rfact,sfact,cval

      return
      end












      
