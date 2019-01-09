      subroutine ftnesmrf(fld, undef, nij, rc)
      USE gex
      USE esmrf_lut

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)


      dimension fld(nij)
      
      logical verb
      logical calcesmrflut

      verb=.true.
      verb=.false.

      calcesmrflut=.false.

C         
C         here is how the esmrf lut is calculated
C         
      if(calcesmrflut) then
        call calc_esat_lut()
      endif

C
C         Calculate esat from input temperature.  Assume there are no
C         undefined valuesm, use the look up table in the esmrf_lut module
C

      do ntz=1,nij

        if(fld(ntz).ne.undef)  then

          ntt=int(dtpvi*(fld(ntz)-float(nxmin)))+1
          ntt1=ntt+1
          tntt=float(nxmin)+dtpv*(ntt-1)

          fld2=pvs2(ntt)+(pvs2(ntt1)-pvs2(ntt))*
     1         dtpvi*(fld(ntz)-tntt)
          fld(ntz)=fld2

        endif

      end do

      rc=0
      return
      end


      subroutine calc_esat_lut()
      USE esmrf_lut

      implicit integer(i-n)
      implicit real*8(a-h,o-z)

      dimension pvs(nxpv)

C         
C         constants
C
      CP= 1.0046E+3
      RD= 2.8705E+2
      RV= 4.6150E+2
      TTP= 2.7316E+2
      HVAP= 2.5000E+6
      PSATK= 6.1078E-1
      CLIQ= 4.1855E+3
      CVAP= 1.8460E+3
      DLDT=CVAP-CLIQ
      dtpvi=float(ndxpv)
      dtpv =1.0/float(ndxpv)

      XA=-DLDT/RV
      XB=XA+HVAP/(RV*TTP)
      XMIN=float(nxmin)
      XMAX=float(nxmax)
      NX=nxpv
      XINC=(XMAX-XMIN)/(NX-1)
      C1XPVS=1.-XMIN/XINC
      C2XPVS=1./XINC
      ANXPVS=NX-0.01
      DO JX=1,NX
        X=XMIN+(JX-1)*XINC
        T=X
        TR=TTP/T
        TBPVS=PSATK*(TR**XA)*EXP(XB*(1.-TR))
        pvs(jx)=tbpvs*10
        print*,jx,pvs(jx)
      enddo
      
      return
      end
