      subroutine ftnclhilo( fld, undef, xi, yi, ni, nj, 
     $     maxmin, radinf, cintinf,
     $     latc,lonc,
     $     opoints,
     $     npoints,nv,nrr,rc)
C
C         npoints = critical point index
C         nv = critical point variables
C
C         dimension order is reversed in fortran
C
C         clhilo.c:
C         float opoints[npoints][nv]
C         
C         .f:
C         real(kind=8) ::  opoints(nv,npoints)
C         
C

C         
C         bug in original code from steve -- implemented my version of
C         sterling's formula in grhilo/grhilo_proc.f and in ngtrk.x
C         original code assumed dlat=dlon=1.0
C         in the y interp
C

      USE gex

      parameter (nmax=2000,nmin=nmax)

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)


CCC      dimension xmax(nmax),ymax(nmax),xmin(nmin),ymin(nmin),
CCC     1     valmax(nmax),valmin(nmin),ihisrt(nmax),ilosrt(nmin)

      real(kind=iGaKind) ::
     $     xmax(nmax),ymax(nmax),xmin(nmin),ymin(nmin),
     $     valmax(nmax),valmin(nmin)

      integer :: ihisrt(nmax),ilosrt(nmin)

      real(kind=iGaKind) ::  fld(ni,nj), xi(ni), yi(nj)
      real(kind=iGaKind) ::  opoints(nv,npoints)
      real(kind=iGaKind) ::  lonc,latc

      real(kind=iGaKind) :: distmax(nmax),distmin(nmin)

      real(kind=iGaKind) :: radinf,cintinf,rmis

      integer maxmin,rc

      character fmt*6,cout*10
      data rmis/-99999999.9/

      logical verb

      verb=.false.

      fmt='i4'
      rc=0

      nhi=0
      nlo=0
      nhimax=0
      nlomax=0


      if(maxmin .ge. 0)  then
        do j=2,nj-1
          do i=2,ni-1

            if(
     $           fld(i,j) .ge. fld(i+1,j  ) .and. 
     1           fld(i,j) .ge. fld(i+1,j+1) .and.
     2           fld(i,j) .ge. fld(i  ,j+1) .and.
     3           fld(i,j) .ge. fld(i-1,j+1) .and.
     4           fld(i,j) .gt. fld(i-1,j  ) .and.
     5           fld(i,j) .gt. fld(i-1,j-1) .and.
     6           fld(i,j) .gt. fld(i  ,j-1) .and.
     7           fld(i,j) .gt. fld(i+1,j-1) .and.
     $           fld(i,j)     .ne. undef  .and. 
     $           fld(i+1,j  ) .ne. undef  .and. 
     1           fld(i+1,j+1) .ne. undef  .and.
     2           fld(i  ,j+1) .ne. undef  .and.
     3           fld(i-1,j+1) .ne. undef  .and.
     4           fld(i-1,j  ) .ne. undef  .and.
     5           fld(i-1,j-1) .ne. undef  .and.
     6           fld(i  ,j-1) .ne. undef  .and.
     7           fld(i+1,j-1) .ne. undef
     $           ) 
     $           then

              if(nhi .lt. nmax)  then
                nhi=nhi+1
C         
C         x proc
C
                dxinumc=fld(i+1,j)-fld(i-1,j)
                dxidenc=2.0*(fld(i+1,j)-2.0*fld(i,j)+fld(i-1,j))
                if(abs(dxinumc) .gt. 0.5*abs(dxidenc))  then
                  dxi=0.0
                  ximax=xi(i)
                else
                  dxi=-dxinumc/dxidenc
                  if(dxi.ge.0.0) then
                    ximax=xi(i)+dxi*(xi(i+1)-xi(i))
                  else
                    ximax=xi(i)+dxi*(xi(i)-xi(i-1))
                  endif
                endif
C         
C         y proc
C
                dyjnumc=fld(i,j+1)-fld(i,j-1)
                dyjdenc=2.0*(fld(i,j+1)-2.0*fld(i,j)+fld(i,j-1))
                if(abs(dyjnumc) .gt. 0.5*abs(dyjdenc))  then
                  dyj=0.0
                  yjmax=yi(j)
                else
                  dyj=-dyjnumc/dyjdenc
                  if(dyj.ge.0.0) then
                    yjmax=yi(j)+dyj*(yi(j+1)-yi(j))
                  else
                    yjmax=yi(j)+dyj*(yi(j)-yi(j-1))
                  endif
                endif

                dxinumu=fld(i+1,j+1)-fld(i-1,j+1)
                dxinumd=fld(i+1,j-1)-fld(i-1,j-1)
                dxidenu=2.0*(fld(i+1,j+1)-2.0*fld(i,j+1)+fld(i-1,j+1))
                dxidend=2.0*(fld(i+1,j-1)-2.0*fld(i,j-1)+fld(i-1,j-1))
                xintc=fld(i,j)+0.5*dxi*(dxinumc+dxi*dxidenc)
                xintu=fld(i,j+1)+0.5*dxi*(dxinumu+dxi*dxidenu)
                xintd=fld(i,j-1)+0.5*dxi*(dxinumd+dxi*dxidend)
                valij=xintc+0.5*dyj*(xintu-xintd+dyj*(xintu-
     1               2.0*xintc+xintd))
                
c         write(6,1211)  fld(i-1,j+1),fld(i,j+1),fld(i+1,j+1)
c         1211          format(' ...fld(i-1,j+1),fld(i,j+1),fld(i+1,j+1)=',3f12.4)
c         write(6,1212)  fld(i-1,j),fld(i,j),fld(i+1,j)
c         1212          format(' ...fld(i-1,j),fld(i,j),fld(i+1,j)=',6x,3f12.4)
c         write(6,1213)  fld(i-1,j-1),fld(i,j-1),fld(i+1,j-1)
c         1213          format(' ...fld(i-1,j-1),fld(i,j-1),fld(i+1,j-1)=',3f12.4)
c         write(6,1214)  ximax,yjmax,valij
c         1214          format(' ...ximax,yjmax,valij=',3f12.3)

                if(valij.lt.undef*0.1) then
                  xmax(nhi)=ximax
                  ymax(nhi)=yjmax
                  valmax(nhi)=valij

                  distmax(nhi)=distsp(yjmax,ximax,latc,lonc)
cc                  distmax(nhi)=sqrt( (ximax-lonc)*(ximax-lonc) 
cc     &                             + (yjmax-latc)*(yjmax-latc) )
                endif


c         write(6,1316)  nhi,xmax(nhi),ymax(nhi),valmax(nhi)
c         1316          format(' ...nhi,xmax,ymax,valmax=',f12.3)
              else
                write(6,1315)  nmax
 1315           format(' ******nhi exceeds nmax=',i4)
              endif
            endif
            
          enddo
        enddo
        nhimax=nhi

c         sort max values 

        call sortrl(valmax,ihisrt,nmax,nhimax)

        if(verb) then
          write(6,*)  '  #    maxval      index       x       y'
          do nhi=nhimax,1,-1
            write(6,5649)  nhi,valmax(nhi),
     $           ihisrt(nhi),xmax(ihisrt(nhi)),
     $           ymax(ihisrt(nhi))
 5649       format(' ',i4,f12.3,i4,2f12.3)
          enddo
        endif

        do nhi=nhimax,1,-1
          if(valmax(nhi) .ne. rmis)  then
            if(verb)  write(6,5678)  ihisrt(nhi),valmax(nhi)
 5678       format(' ...sorted max, ihisrt,valmax=',i4,f12.3)
            do ntest=nhi-1,1,-1
              if(valmax(ntest) .ne. rmis)  then
                dis=distsp(ymax(ihisrt(nhi)),xmax(ihisrt(nhi)),
     1               ymax(ihisrt(ntest)),xmax(ihisrt(ntest)))
                if(dis .lt. radinf .and. abs(valmax(nhi)-
     1               valmax(ntest)) .le. cintinf)  then
                  valmax(ntest)=rmis
                  if(verb) write(6,5683)  ntest
 5683             format(' ......valmax for ntest=',i5,' is removed.')
                endif
              endif
            enddo
          endif
        enddo
      endif

      if(maxmin .le. 0)  then
        do j=2,nj-1
          do i=2,ni-1

c         if(fld(i,j) .le. fld(i+1,j  ) .and.
c         1       fld(i,j) .le. fld(i+1,j+1) .and.
c         2       fld(i,j) .le. fld(i  ,j+1) .and.
c         3       fld(i,j) .le. fld(i-1,j+1) .and.
c         4       fld(i,j) .lt. fld(i-1,j  ) .and.
c         5       fld(i,j) .lt. fld(i-1,j-1) .and.
c         6       fld(i,j) .lt. fld(i  ,j-1) .and.
c         7       fld(i,j) .lt. fld(i+1,j-1))  then

            if(
     $           fld(i,j) .le. fld(i+1,j  ) .and. 
     1           fld(i,j) .le. fld(i+1,j+1) .and.
     2           fld(i,j) .le. fld(i  ,j+1) .and.
     3           fld(i,j) .le. fld(i-1,j+1) .and.
     4           fld(i,j) .lt. fld(i-1,j  ) .and.
     5           fld(i,j) .lt. fld(i-1,j-1) .and.
     6           fld(i,j) .lt. fld(i  ,j-1) .and.
     7           fld(i,j) .lt. fld(i+1,j-1) .and.
     $           fld(i,j)     .ne. undef  .and. 
     $           fld(i+1,j  ) .ne. undef  .and. 
     1           fld(i+1,j+1) .ne. undef  .and.
     2           fld(i  ,j+1) .ne. undef  .and.
     3           fld(i-1,j+1) .ne. undef  .and.
     4           fld(i-1,j  ) .ne. undef  .and.
     5           fld(i-1,j-1) .ne. undef  .and.
     6           fld(i  ,j-1) .ne. undef  .and.
     7           fld(i+1,j-1) .ne. undef
     $           ) 
     $           then


c         write(6,21)  i,j,fld(i,j),xi(i),yi(j),fmt,fmt(1:1)
c         21        format(' ...min: i,j,fld(i,j)=',2i4,e12.3,2f9.3,' fmt=',a,
c         1              '...fmt(1:1)=',a)

              if(nlo .lt. nmax)  then
                nlo=nlo+1
C         
C         x proc
C
                dxinumc=fld(i+1,j)-fld(i-1,j)
                dxidenc=2.0*(fld(i+1,j)-2.0*fld(i,j)+fld(i-1,j))
                if(abs(dxinumc) .gt. 0.5*abs(dxidenc))  then
                  dxi=0.0
                  ximin=xi(i)
                else
                  dxi=-dxinumc/dxidenc
                  if(dxi.ge.0.0) then
                    ximin=xi(i)+dxi*(xi(i+1)-xi(i))
                  else
                    ximin=xi(i)+dxi*(xi(i)-xi(i-1))
                  endif
                endif

C         
C         y proc
C
                dyjnumc=fld(i,j+1)-fld(i,j-1)
                dyjdenc=2.0*(fld(i,j+1)-2.0*fld(i,j)+fld(i,j-1))
                if(abs(dyjnumc) .gt. 0.5*abs(dyjdenc))  then
                  dyj=0.0
                  yjmin=yi(j)
                else
C         
C         bug in original code from steve -- implemented my version of
C         sterling's formula in grhilo/grhilo_proc.f and in ntrker
C
                  dyj=-dyjnumc/dyjdenc
                  if(dyj.ge.0.0) then
                    yjmin=yi(j)+dyj*(yi(j+1)-yi(j))
                  else
                    yjmin=yi(j)+dyj*(yi(j)-yi(j-1))
                  endif
                endif

                dxinumu=fld(i+1,j+1)-fld(i-1,j+1)
                dxinumd=fld(i+1,j-1)-fld(i-1,j-1)
                dxidenu=2.0*(fld(i+1,j+1)-2.0*fld(i,j+1)+fld(i-1,j+1))
                dxidend=2.0*(fld(i+1,j-1)-2.0*fld(i,j-1)+fld(i-1,j-1))
                xintc=fld(i,j)+0.5*dxi*(dxinumc+dxi*dxidenc)
                xintu=fld(i,j+1)+0.5*dxi*(dxinumu+dxi*dxidenu)
                xintd=fld(i,j-1)+0.5*dxi*(dxinumd+dxi*dxidend)
                valij=xintc+0.5*dyj*(xintu-xintd+dyj*(xintu-
     1               2.0*xintc+xintd))
c         write(6,1211)  fld(i-1,j+1),fld(i,j+1),fld(i+1,j+1)
c         write(6,1212)  fld(i-1,j),fld(i,j),fld(i+1,j)
c         write(6,1213)  fld(i-1,j-1),fld(i,j-1),fld(i+1,j-1)
c         write(6,1214)  ximin,yjmin,valij
                
CCCCCCCCCCCC                print*, 'vvvvvvvvvvvvvvvlllllllllllllllllll ',valij,nlo
                if(valij.lt.undef*0.1) then
                  xmin(nlo)=ximin
                  ymin(nlo)=yjmin
                  valmin(nlo)=valij
                  distmin(nlo)=distsp(yjmin,ximin,latc,lonc)
ccc                  distmin(nlo)=sqrt( (ximin-lonc)*(ximin-lonc) +
ccc     &                             (yjmin-latc)*(yjmin-latc) )
                endif
              else
                write(6,1317)  nmin
 1317           format(' ******nlo exceeds nmin=',i4)
              endif 
            endif
            
          enddo
        enddo

        nlomax=nlo
        call sortrl(valmin,ilosrt,nmin,nlomax)

        if(verb) then
          write(6,*)  '  #    minval      index       x       y'
          do nlo=1,nlomax
            write(6,6649)  nlo,valmin(ilosrt(nlo)),
     $           ilosrt(nlo),xmin(ilosrt(nlo)),
     $           ymin(ilosrt(nlo))
 6649       format(' ',i4,f12.3,i4,2f12.3)
          enddo
        endif
        do nlo=1,nlomax
          if(valmin(nlo) .ne. rmis)  then
            if(verb) write(6,6678)  nlo,ilosrt(nlo),valmin(nlo)
 6678       format(' ...sorted min, nlo,ilosrt,valmin=',2i4,f12.3)
            do ntest=nlo+1,nlomax
              if(valmin(ntest) .ne. rmis)  then
                dis=distsp(ymin(ilosrt(nlo)),xmin(ilosrt(nlo)),
     1               ymin(ilosrt(ntest)),xmin(ilosrt(ntest)))
                if(dis .lt. radinf .and. abs(valmin(nlo)-
     1               valmin(ntest)) .le. cintinf)  then
                  valmin(ntest)=rmis
                  if(verb) write(6,6683)  ntest
 6683             format(' ......valmin for ntest=',i5,' is removed.')
                endif
              endif
            enddo
          endif
        enddo
      endif

c         for proximate highs and lows, we decide in favor of the lows

      do nhi=1,nhimax
        if(valmax(nhi) .ne. rmis)  then
          if(verb) write(6,7678)  ihisrt(nhi),valmax(nhi)
 7678     format(' ...sorted max, ihisrt,valmax=',i4,f12.3)
          do ntest=1,nlomax
            if(valmin(ntest) .ne. rmis)  then
              dis=distsp(ymax(ihisrt(nhi)),xmax(ihisrt(nhi)),
     1             ymin(ilosrt(ntest)),xmin(ilosrt(ntest)))
              if(dis .lt. radinf)  then
                valmax(nhi)=rmis
                if(verb) write(6,7683)  nhi
 7683           format(' ......valmax for nhi=',i5,' is removed.')
              endif
            endif
          enddo
        endif
      enddo


      nrr=0
      do nhi=nhimax,1,-1
        if(valmax(nhi) .ne. rmis)  then
          if(fmt(1:1) .eq. "f" .or. fmt(1:1) .eq. 'f')  then
            write(cout,'('//fmt//')')  valmax(nhi)
          else if (fmt(1:1) .eq. "i" .or. fmt(1:1) .eq. 'i')  then
            write(cout,'('//fmt//')')  nint(valmax(nhi))
          else
            if(verb) write(6,19)  fmt,fmt(1:1)
 19         format('  ...format not allowed, fmt,fmt(1:1)=',
     1           2(a,'...'))
          endif 

          if(maxmin.ge.0) then
            nrr=nrr+1
            if(nrr.ge.npoints) then
              print*,'EEEE too many points in ftn_clhilo;',
     $            ' nrr, npoints: ',
     $             nrr,npoints
              rc=1
              return
            endif

            opoints(1,nrr)=1.0
            opoints(2,nrr)=valmax(nhi)
            opoints(3,nrr)=ymax(ihisrt(nhi))
            opoints(4,nrr)=xmax(ihisrt(nhi))
            opoints(5,nrr)=distmax(nhi)
          endif

        endif
      enddo


      do nlo=1,nlomax
        if(valmin(nlo) .ne. rmis)  then
          if(fmt(1:1) .eq. "f" .or. fmt(1:1) .eq. 'f')  then
            write(cout,'('//fmt//')')  valmin(nlo)
          else if (fmt(1:1) .eq. "i" .or. fmt(1:1) .eq. 'i')  then
            write(cout,'('//fmt//')')  nint(valmin(nlo))
          else
            if(verb) write(6,29)  fmt,fmt(1:1)
 29         format('  ...format not allowed, fmt,fmt(1:1)=',
     1           2(a,'...'))
          endif 

          if(maxmin.le.0) then
            nrr=nrr+1
            if(nrr.ge.npoints) then
              print*,'EEEE too many points in ftn_clhilo;',
     $            ' nrr, npoints: ',
     $             nrr,npoints
              rc=1
              return
            endif

            opoints(1,nrr)=-1.0
            opoints(2,nrr)=valmin(nlo)
            opoints(3,nrr)=ymin(ilosrt(nlo))
            opoints(4,nrr)=xmin(ilosrt(nlo))
            opoints(5,nrr)=distmin(nlo)
          endif
          
        endif
      enddo

      return
      end
