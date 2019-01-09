      subroutine ftnsmth2d(fld, undef, ni, nj, 
     $     npass,nnu,anu,dum,rc)

      USE gex

      real(kind=iGaKind) :: fld(ni,nj)
      real(kind=iGaKind) :: undef
      real(kind=iGaKind) :: dum(ni,nj)
      real(kind=iGaKind) :: anu(nnu)

      logical ioresp,io

      integer rc
C         
C         initialize
C

      ib=0
      ie=0
      jb=0
      je=0
      iskip=2
      dx=1.0

      ioresp=.true.
      io=.true.

      ioresp=.false.
      io=.false.

C        
C         initialize dum to input field
C
c
      do i=1,ni
        do j=1,nj
          dum(i,j)=fld(i,j)
        enddo
      enddo

C         
C         put undef check in smth2d
C

      ib=2
      ie=ni-1
      jb=2
      je=nj-1

      call smth2d(fld,ni,nj,ib,ie,jb,je,
     $     anu,npass,nnu,ioresp,io,iskip,dx,dum,undef)

      return
      end


      subroutine smth2d(a,ni,nj,ib,ie,jb,je,
     1     anu,npass,nnu,ioresp,io,iskip,dx,b,undef)
      
      USE gex
C
C...      routine to smooth a 2-d field at subsection of interior points
C...      using a noncomplex shuman (1957) smoother-desmoother 
C
      real (kind=iGaKind) a(ni,nj),b(ni,nj),anu(nnu)
      real (kind=iGaKind) pi,rlambda,dx,undef

      logical ioresp,io
      character qtitle*24
      
C
C...     output unsmoothed field if io.ne.0
C

      if(io) then
        call stat2(a,ni,nj,amin,amax,amean,avar,asd)
        write(6,12) amean,amin,amax,avar,asd
12      format(' ',/,' ',' input field mean = ',1pe13.4,/
     $       ' ','             amin = ',1pe13.4,/
     $       ' ','             amax = ',1pe13.4,/
     $       ' ','         variance = ',1pe13.4,/
     $       ' ','         stnd dev = ',1pe13.4,//)
        qtitle='raw field                '
        call qprntn(a,qtitle,1,1,ni,nj,iskip,6)

      end if

C         
C         mmmmmmmmmmmmmmmmm main loops, npass, the nus 
C

      do nn=1,npass

        do l=1,nnu

          do i=ib,ie
            do j=jb,je

              if(
     $             a(i,j).eq.undef.or.
     $             a(i+1,j).eq.undef.or.
     $             a(i-1,j).eq.undef.or.
     $             a(i,j-1).eq.undef.or.
     $             a(i,j+1).eq.undef.or.
     $             a(i+1,j+1).eq.undef.or.
     $             a(i+1,j-1).eq.undef.or.
     $             a(i-1,j-1).eq.undef.or.
     $             a(i-1,j+1).eq.undef
     $             ) then
                b(i,j)=a(i,j)

              else

                b(i,j)=a(i,j)*(1.0-anu(l))**2
     $               + 0.5*anu(l)*(1.0-anu(l))*
     $               (a(i+1,j)+a(i-1,j)+a(i,j+1)+a(i,j-1))
     $               + 0.25*(anu(l)**2)*
     $               (a(i-1,j-1)+a(i-1,j+1)+a(i+1,j-1)+a(i+1,j+1))
              endif
            end do
          end do

          do i=ib,ie
            do j=jb,je
              a(i,j)=b(i,j)
            end do
          end do

        end do 

      end do

C
C...     output smoothed field if io ne 0
C
      if(io) then

        call stat2(a,ni,nj,amin,amax,amean,avar,asd)
        write(6,14) amean,amin,amax,avar,asd
14      format(' ',/,' ','output field mean = ',1pe13.4,/
     $       ' ','             amin = ',1pe13.4,/
     $       ' ','             amax = ',1pe13.4,/
     $       ' ','         variance = ',1pe13.4,/
     $       ' ','         stnd dev = ',1pe13.4,//)
        qtitle='smoothed field          '
        call qprntn(a,qtitle,1,1,ni,nj,iskip,6)

      end if

C
C...     calculate response function assuming fourier basis
C

      if(ioresp) then

        write(6,200) npass,nnu 
200     format(' ',//,' ','smoothing function analysis'/
     1    ' ',5x,'number of passes = ',i2/
     2    ' ',5x,'number of elements per pass = ',i2)
        do k=1,nnu
          write(6,201) k,anu(k)
201       format(' ',7x,'k = ',i2,
     1  '  smoothing coefficient nu = ',f6.3)
        end do

        pi=4.0*atan(1.0)

        do i=2,ni
          b(i,1)=float(i)
          b(i,2)=1.0
          do mm=1,nnu
            b(i,2)=b(i,2)*(1.0-anu(mm)*(1.0-cos(2.0*pi/float(i))))
          end do

          b(i,2)=b(i,2)**npass
        end do

C
        write(6,222)
222     format(' ','response function as a function of wavelength ',
     1    'in grid units*dx',//,
     2    ' ','  lambda  response  ',//)
C
        do i=2,ni
          rlambda=dx*i
          write(6,225) rlambda,b(i,2)
225       format(' ',f7.1,3x,f6.3)
        end do

      end if 

      return
      end
C
C--------------------------------------------------------------------------
C         
C         program to test smth2d
C
c     program tests
c     dimension a(10,10),dum(10,10),anu(2)
c     character qtitle*24
c     do i=1,10
c       do j=1,10
c         a(i,j)=5.0**sin(3.14*0.5*(j-1))
c       end do
c     end do
c     anu(1)=0.5
c     anu(2)=-0.5
c
c     call smth2d(a,10,10,2,9,2,9,
c    $     anu,1,2,.true.,.true.,1.0,dum)  
c
c      stop
c      end
C
C--------------------------------------------------------------------------
C         

      subroutine stat2(a,m,n,amin,amax,amean,avar,asigma)
      USE gex
      real(kind=iGaKind) :: dimension a(m,n)

      amean = 0.0
      amin = 9.9e25
      amax = -9.9e25
      avar = 0.0
      asigma = 0.0
      do i=1,m
      do j=1,n
        if(a(i,j).lt.amin) amin=a(i,j)
        if(a(i,j).gt.amax) amax=a(i,j)
        amean=amean+a(i,j)
      end do
      end do
      amean = amean/m*n
      do i=1,m
      do j=1,n
        avar = avar + (a(i,j)-amean)**2
      end do
      end do
      avar = avar/(m*n-1)
      asigma=sqrt(avar)
      return
      end
