      subroutine ftntcxy2rt(fldxy,ni,nj,undef,
     $     xylat,xylon,
     $     clat,clon,
     $     drad,dtheta,rmax,
     $     opoints,olats,olons,nr,nt,
     $     rc
     $     )

      USE gex
      USE const

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      dimension fldxy(ni,nj),xylat(nj),xylon(ni)

C         
C         output
C         
      dimension  opoints(nr,nt),olats(nr,nt),olons(nr,nt)

      real(kind=iGaKind), save, allocatable :: r(:),rbm(:),rb2m(:)
      real(kind=iGaKind), save, allocatable :: rbp(:),rb2p(:)

      real(kind=iGaKind), save, allocatable :: phi(:),phibm(:),phibp(:)


      allocate (r(nr),stat=iflag)

      allocate (rbm(nr),stat=iflag)
      allocate (rb2m(nr),stat=iflag)

      allocate (rbp(nr),stat=iflag)
      allocate (rb2p(nr),stat=iflag)

      allocate (phi(nt),stat=iflag)

      allocate (phibm(nt),stat=iflag)
      allocate (phibp(nt),stat=iflag)




Cxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
C
C         xy2rt         

      do i=1,nr
        r(i)=(i-1)*drad
        rbm(i)=r(i)-drad*0.5
        rbp(i)=r(i)+drad*0.5
        if(rbm(i).lt.0.0) rbm(i)=0.0
        rb2m(i)=rbm(i)*rbm(i)
        rb2p(i)=rbp(i)*rbp(i)
      enddo

      do j=1,nt
        phi(j)=(j-1)*dtheta
        phibm(j)=phi(j)-dtheta*0.5
        phibp(j)=phi(j)+dtheta*0.5
      enddo

      rminrti=+1e20
      rmaxrti=-1e20
      rminrtj=+1e20
      rmaxrtj=-1e20

      do i=1,nr

        fb=0.0
        nb=0

        do j=1,nt

          call rumlatlon (phi(j),r(i),clat,clon,rlat1,rlon1)
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

          opoints(i,j)=cvalb
          olats(i,j)=rlat1
          olons(i,j)=rlon1

c          if(j.eq.1) then
c           print*,'i: ',i,'j: ',j,' r: ',r(i),' t: ',phi(j),' opoint: ',opoints(i,j)
c          endif

        end do

      end do

      
      deallocate (r,stat=iflag)
      deallocate (rbm,stat=iflag)
      deallocate (rb2m,stat=iflag)
      deallocate (rbp,stat=iflag)
      deallocate (rb2p,stat=iflag)

      deallocate (phi,stat=iflag)
      deallocate (phibm,stat=iflag)
      deallocate (phibp,stat=iflag)

      return 
      end


