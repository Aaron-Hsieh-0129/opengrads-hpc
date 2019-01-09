      subroutine ftnlinreg(x,y,undef, nij, card, rc)

      USE gex

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      dimension x(nij),y(nij)

      character*120 card

      logical verb

      verb=.true.
      verb=.false.
c
c  do linear regression
c
      call fit (x,y,nij,a,b,undef)
c
c  fill in data values
c
      do i=1,nij
        if(y(i).ne.undef) then
          y(i) = a+x(i)*b
        end if
      end do

      if(verb) print*,'x,y,a,b: ',x(1),y(1),a,b
      
      write(card,'(a,g10.4,a,g10.4)') 'in y=mx+b m: ',a,' b: ',b
      rc=0

      return
      end
c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc 
ccccccc
c
      subroutine fit(x,y,ndata,a,b,undef)
      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

c
c	a is the intercept
c	b is the slope
c
       dimension x(ndata), y(ndata)
c
       sx = 0.
       sy = 0.
       st2 = 0.
       b = 0.
       ncnt=0
       do 12 i = 1, ndata
         if(y(i).ne.undef) then
           sx = sx + x(i)
           sy = sy + y(i)
           ncnt=ncnt+1
         endif
 12    continue
       if(ncnt.eq.0) return
       ss = float(ncnt)
       sxoss = sx/ss 

       do 14 i = 1, ndata
         if(y(i).ne.undef) then
           t = x(i) - sxoss
           st2 = st2 + t * t
           b = b + t * y(i)
         endif
14     continue
       b = b/st2
       a = (sy - sx * b)/ss
       return
       end
