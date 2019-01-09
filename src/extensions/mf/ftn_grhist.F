      subroutine ftngrhist( fld, np, undef,
     $     opoints,nr,nv)


      USE gex

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

C         
C         input from libmf.c
C         
      real(kind=iGaKind) :: fld(np)
      real(kind=iGaKind) :: undef

      integer n,ni,nj,nr,nv
      integer i,j,jj

      integer ndxr(np),ndxo(np)

      integer verb

C         
C         output
C         
      real(kind=iGaKind) ::  opoints(nv,nr)

      call indexx(np,fld,ndxo)

      do  n=1,np
        print*,n,ndxo(n),fld(ndxo(n))
      end do

      npm1=np-1
      nc=npm1

      dcutp=npm1/float(nc)
      print*,'cccc ',np,nc,dcutp

      do i=1,nc
        ptile=(i-1)*dcutp
        dum=ptile-int(ptile)
        ip1=int(ptile)+1
        ip2=ip1+1;
        if(ip2.ge.npm1) ip2=npm1
ccc        cutp=dum*fld(ndxo(ip2)) + (1.0-dum)*fld(ndxo(ip1))
        print*,'iiiiiiiiii ',i,ip1,ip2,npm1,dum,ndxo(ip1),ndxo(ip2)

C    cutp=dum*(*(gr+p2))+(1-dum)*(*(gr+p1));
C         *(gh+ioff+i) = cutp;

      end do

      return
      end

