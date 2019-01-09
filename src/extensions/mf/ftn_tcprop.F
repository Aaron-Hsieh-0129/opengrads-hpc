      subroutine ftntcprop( 
     $     latc,lonc,radinf,bearing,
     $     fld, undef,
     $     xi, yi, ni, nj,
     $     opoints,nr,nv
     $     )


      USE gex
      USE trig_vals
      USE grid_bounds

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

C         
C         input from libmf.c
C         
      real(kind=iGaKind) :: fld(ni,nj), xi(ni), yi(nj)
      real(kind=iGaKind) :: undef

C         
C         gettrk_main.f vars
C

      parameter(idtheta=10,ntheta=(360/idtheta)+1)
      real(kind=iGaKind) :: lonc,latc,cvalb,dval,xvalb,dist
      real(kind=iGaKind) :: cxi,cyj
      real(kind=iGaKind) :: head,dhead,radinf,radinfnm,elat,elon
      real(kind=iGaKind) :: maxcgrad,mincgrad,meancgrad
      real(kind=iGaKind) :: fldmean
      real(kind=iGaKind) :: cntrval(ntheta),cntrdval(ntheta),ocntrvals(nr)

      integer nradii

      integer, parameter :: iunit_diag=6

      integer n,ni,nj,nr,nv
      integer i,j,jj

      integer verb

C         output
C         
      real(kind=iGaKind) ::  opoints(nr,nv)

      pi = 4. * atan(1.)   ! Both pi and dtr were declared in module
      dtr = pi/180.0       ! trig_vals, but were not yet defined.


      verb=0

C         
C         convert radinf to nm for rhumb line
C         use nm vice km
C
ccc       radinfnm=(radinf/dtk)*60.0
      

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
      dhead=idtheta*1.0
      nradii=0
      ibmin=+99999999
      iemax=-99999999
      jbmin=+99999999
      jemax=-99999999

      nt=1
      do while(head <= 360.0)
        call rcaltln (latc,lonc,head,radinf,elat,elon)
        call lonlat2ij(elon,elat,xi,yi,ni,nj,undef,cxi,cyj)

        if(int(cxi).lt.ibmin) ibmin=int(cxi)
        if(int(cxi).gt.iemax) iemax=int(cxi)

        if(int(cyj).lt.jbmin) jbmin=int(cyj)
        if(int(cyj).gt.jemax) jemax=int(cyj)

        call bssl5(cxi,cyj,fld,ni,nj,xvalb,undef)
        dval=(xvalb-cvalb)/radinf
        if(dval > maxcgrad) maxcgrad=dval
        if(dval <= mincgrad) mincgrad=dval
        meancgrad=meancgrad+dval
        head=head+idtheta
        nradii=nradii+1
        cntrval(nt)=xvalb
        cntrdval(nt)=dval/radinf
        nt=nt+1
      end do
      
      if(nradii > 0) then
        meancgrad=meancgrad/nradii
      endif

C         
C         return the max/min/mean gradient          
C

      opoints(1,1)=maxcgrad/radinf
      opoints(2,1)=mincgrad/radinf
      opoints(3,1)=meancgrad/radinf
      
C         
C         field mean inside radinf
C         

      fldmean=0.0

      fldhemi1=0.0
      fldhemi2=0.0

      fldquadne=0.0
      fldquadse=0.0
      fldquadsw=0.0
      fldquadnw=0.0


C         
C         relative quads 0-90
C

      n=0
      nb1=0
      nb2=0
      nbne=0
      nbse=0
      nbsw=0
      nbnw=0

      ib=ibmin-1
      if(ib.le.0) ib=1

      jb=jbmin-1
      if(jb.le.0) jb=1

      ie=iemax+1
      if(ie.gt.ni) ie=ni

      je=jemax+1
      if(je.gt.nj) je=nj

      do i=ib,ie
        do j=jb,je
          call rumdirdist (latc,lonc,yi(j),xi(i),head,dist)
          if(dist <= radinf) then
            fldmean=fldmean+fld(i,j)
            n=n+1

            call whichquad(head,bearing,iquad)

C         
C         hemis
C         

            if(iquad.eq.1.or.iqaud.eq.2) then
              fldhemi1=fldhemi1+fld(i,j)
              nb1=nb1+1
            else
              fldhemi2=fldhemi2+fld(i,j)
              nb2=nb2+1
            endif
C         
C         quandrants
C         

            if(iquad.eq.1) then
              fldquadne=fldquadne+fld(i,j)
              nbne=nbne+1
            elseif(iquad.eq.2) then
              fldquadse=fldquadse+fld(i,j)
              nbse=nbse+1
            elseif(iquad.eq.3) then
              fldquadsw=fldquadsw+fld(i,j)
              nbsw=nbsw+1
            elseif(iquad.eq.4) then
              fldquadnw=fldquadnw+fld(i,j)
              nbnw=nbnw+1
            endif
          endif

        enddo
      enddo

      if(n>0) then
        fldmean=fldmean/float(n)
      else
        fldmean=undef
      endif

      if(nb1>0) then
        fldhemi1=fldhemi1/float(nb1)
      else
        fldhemi1=undef
      endif

      if(nb2>0) then
        fldhemi2=fldhemi2/float(nb2)
      else
        fldhemi2=undef
      endif

      if(nbne>0) then
        fldquadne=fldquadne/float(nbne)
      else
        fldquadne=undef
      endif

      if(nbse>0) then
        fldquadse=fldquadse/float(nbse)
      else
        fldquadse=undef
      endif

      if(nbsw>0) then
        fldquadsw=fldquadsw/float(nbsw)
      else
        fldquadsw=undef
      endif

      if(nbnw>0) then
        fldquadnw=fldquadnw/float(nbnw)
      else
        fldquadnw=undef
      endif

      nbq=nbne+nbse+nbsw+nbnw
      nbh=nb1+nb2

      if(verb.eq.1) then
        print*,'ttttttttttttttttt ',n,nb1,nb2,nbh,nbq
        print*,'HHHHHHHHHH ',fldhemi1,nb1,fldhemi2,nb2
        print*,'QQQQQQ ne: ',fldquadne,nbne
        print*,'QQQQQQ se: ',fldquadse,nbse
        print*,'QQQQQQ sw: ',fldquadsw,nbsw
        print*,'QQQQQQ nw: ',fldquadnw,nbnw
      endif

      opoints(1,2)=fldmean
      opoints(2,2)=fldhemi1
      opoints(3,2)=fldhemi2
      opoints(4,2)=fldquadne
      opoints(5,2)=fldquadse
      opoints(6,2)=fldquadsw
      opoints(7,2)=fldquadnw

      nvb=3
      call quadcntrprop(cntrval,ntheta,dhead,bearing,opoints,nr,nv,nvb)

      nvb=nvb+3
      call quadcntrprop(cntrdval,ntheta,dhead,bearing,opoints,nr,nv,nvb)

      return
      end


      subroutine quadcntrprop(cntrval,nt,dhead,bearing,opoints,nr,nv,nvb)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      dimension cntrval(nt)

      dimension opoints(nr,nv)

      integer iquad,verb

      verb=0
C         
C         relative quads 0-90
C
      b1ne=0
      b2ne=b1ne+90.0

      b1se=b2ne
      b2se=b1se+90.0

      b1sw=b2se
      b2sw=b1sw+90.0

      b1nw=b2sw
      b2nw=b1nw+90.0

      cntrmean=0.0
      cntrmax=-9.0e20
      cntrmin=+9.0e20

      cntrmaxqne=-9.0e20
      cntrmaxqse=-9.0e20
      cntrmaxqsw=-9.0e20
      cntrmaxqnw=-9.0e20

      cntrminqne=+9.0e20
      cntrminqse=+9.0e20
      cntrminqsw=+9.0e20
      cntrminqnw=+9.0e20

      cntrmeanqne=0.0
      cntrmeanqse=0.0
      cntrmeanqsw=0.0
      cntrmeanqnw=0.0

      nall=0
      nne=0
      nse=0
      nsw=0
      nnw=0

C         
C         quandrants
C         
      do n=1,nt-1

        head=(n-1)*dhead
        nall=nall+1
        cntrmean=cntrmean+cntrval(n)

        if(cntrval(n).gt.cntrmax) cntrmax=cntrval(n)
        if(cntrval(n).lt.cntrmin) cntrmin=cntrval(n)

        call whichquad(head,bearing,iquad)

        if(iquad.eq.1) then
          nne=nne+1
          cntrmeanqne=cntrmeanqne+cntrval(n)
          if(cntrval(n).gt.cntrmaxqne) cntrmaxqne=cntrval(n)
          if(cntrval(n).lt.cntrminqne) cntrminqne=cntrval(n)

        elseif(iquad.eq.2) then
          nse=nse+1
          cntrmeanqse=cntrmeanqse+cntrval(n)
          if(cntrval(n).gt.cntrmaxqse) cntrmaxqse=cntrval(n)
          if(cntrval(n).lt.cntrminqse) cntrminqse=cntrval(n)

        elseif(iquad.eq.3) then
          nsw=nsw+1
          cntrmeanqsw=cntrmeanqsw+cntrval(n)
          if(cntrval(n).gt.cntrmaxqsw) cntrmaxqsw=cntrval(n)
          if(cntrval(n).lt.cntrminqsw) cntrminqsw=cntrval(n)

        elseif(iquad.eq.4) then
          nnw=nnw+1
          cntrmeanqnw=cntrmeanqnw+cntrval(n)
          if(cntrval(n).gt.cntrmaxqnw) cntrmaxqnw=cntrval(n)
          if(cntrval(n).lt.cntrminqnw) cntrminqnw=cntrval(n)

        else
          print*,'EEE invalid quad for head: ',head
          stop 'quad prob'

        endif
        
      enddo

      if(nall.gt.0) cntrmean=cntrmean/nall
      if(nne.gt.0) cntrmeanqne=cntrmeanqne/nne
      if(nse.gt.0) cntrmeanqse=cntrmeanqse/nse
      if(nsw.gt.0) cntrmeanqsw=cntrmeanqsw/nsw
      if(nnw.gt.0) cntrmeanqnw=cntrmeanqnw/nnw

      if(verb.eq.1) then

        print*,'ne ',nne,cntrmeanqne,cntrmaxqne,cntrminqne
        print*,'se ',nse,cntrmeanqse,cntrmaxqse,cntrminqse
        print*,'sw ',nsw,cntrmeanqsw,cntrmaxqsw,cntrminqsw
        print*,'nw ',nnw,cntrmeanqnw,cntrmaxqnw,cntrminqnw
        
        print*,'mmmmmnmean ',cntrmean,nall
        print*,'mmmmmmmmax ',cntrmax
        print*,'mmmmmmmmin ',cntrmin

      endif

      nvo=nvb
      opoints(1,nvo)=cntrmean
      opoints(4,nvo)=cntrmeanqne
      opoints(5,nvo)=cntrmeanqse
      opoints(6,nvo)=cntrmeanqsw
      opoints(7,nvo)=cntrmeanqnw

      nvo=nvo+1
      opoints(1,nvo)=cntrmax
      opoints(4,nvo)=cntrmaxqne
      opoints(5,nvo)=cntrmaxqse
      opoints(6,nvo)=cntrmaxqsw
      opoints(7,nvo)=cntrmaxqnw

      nvo=nvo+1
      opoints(1,nvo)=cntrmin
      opoints(4,nvo)=cntrminqne
      opoints(5,nvo)=cntrminqse
      opoints(6,nvo)=cntrminqsw
      opoints(7,nvo)=cntrminqnw

      return
      end

      
      
      subroutine whichquad(head,bearing,irc)

      use GEX

      real(kind=iGaKind) 
     $     head,bearing,headchk,
     $     b1ne,b2ne,b1se,b2se
     $     b1sw,b2sw,b1nw,b2nw

      integer :: irc
      
      irc=-999
      headchk=head
      if(headchk.lt.bearing) headchk=headchk+360.0

      b1ne=bearing
      b2ne=b1ne+90.0

      b1se=b2ne
      b2se=b1se+90.0

      b1sw=b2se
      b2sw=b1sw+90.0
      
      b1nw=b2sw
      b2nw=b1nw+90.0

      if(headchk.ge.b1ne.and.headchk.lt.b2ne) then
        irc=1
        
      elseif(headchk.ge.b1se.and.headchk.lt.b2se) then
        irc=2
        
      elseif(headchk.ge.b1sw.and.headchk.lt.b2sw) then
        irc=3
        
      elseif(headchk.ge.b1nw.and.headchk.lt.b2nw) then
        irc=4

      endif

      return
      end 

