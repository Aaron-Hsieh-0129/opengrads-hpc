      subroutine ftnre2(fldin,xin,yin,nii,nji,
     $     rvals,nargs,args,incard,
     $     fldout,xout,yout,nio,njo,njog,
     $     ovals,iflag,
     $     chrc,rc)

      USE gex

      implicit integer(i-n)
      implicit real(kind=iGaKind) (a-h,o-z)

      parameter(n1max=10000,
     $     niopt=4,nvopt=8)

      dimension fldin(nii,nji),xin(nii),yin(nji),
     $     xinb(nii+1),yinb(nji+1)

      dimension gxout(nio+1),gyout(njo+1)
      dimension glat(njog+1),glatb(njog+1)

      dimension dumx(n1max)

      dimension fldout(nio,njo),xout(nio+1),yout(njo+1)

      dimension dum1(nii,nji),
     $     area_in(nii,nji),area_out(nio,njo)

      integer nijo,ier
      integer rc,nargs,nargsl
      integer did_glat_allocate

      dimension rvals(20),ovals(20),args(10)

      character*10 carg(niopt)
      character*2 copt(nvopt)
      character*1 incard(80),chrc(720)

      character card*80,qtitle*24,regrid_method*40,chcard*120

      logical cyclicxi,wrapxi
      logical gauss_out,
     $     diag_out,vote,bessel,
     $     npole_point,spole_point

      logical verb,dowrapfix


      data copt/'ig','ba','bl','vt','bs','p1','gg','ma'/

C         
C         print diagnostics
C 
      iunit_diag=6
      verb=.true.
      verb=.false.
      diag_out=.true.
      diag_out=.false.


C         
C         constants
C
      pi=3.141592654
      deg2rad=pi/180.0
      rad2deg=180.0/pi
C         
C         defaults
C

      dowrapfix=.false.
      dowrapfix=.true.

C         
C         user defined beginning lat/lon
C
      rlatbeg_set=-999.0
      rlonbeg_set=-999.0
C         
C         pole points
C         
      npole_point=.false.
      spole_point=.false.
C         
C         initialize flag for using calculated gauss lat weights
C
      jsglat=-888
C         
C         the default method is box averaging without voting on
C         a unform grid
C         
      iregrid_type=1
      iregrid_method=1
      ilinearo=1
      jlinearo=1
      bessel=.false.
      vote=.false.
      gauss_out=.false.
C         
C         minimum fractional area covered by data in box averaging
C         to have a defined point
C         
      area_min=0.5
C         
C         voting defaults --
C         50% of grid box must be covered regardless of the number
C         of candidates
C         
      rmin_vote_max=0.5
      rmin_vote_min=0.5
C         
C         length of output card
C
      njcard=120


C         
C         grid parameters
C
      undef=rvals(1)
      itype=nint(real(rvals(2)))
      jtype=nint(real(rvals(3)))
      nii=nint(real(rvals(4)))
      nji=nint(real(rvals(5)))
      ilineari=nint(real(rvals(6)))
      jlineari=nint(real(rvals(7)))
      xbeg=rvals(8)



Caaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
C         
C  allocate dummy arrays         
C         

C         
C         check the input grid type
C         
C         abort if not 2-d x,y
C         
      if(itype.ne.0.or.jtype.ne.1) go to 830
C         
C         get grid increments if uniform
C         
      if(ilineari.eq.1) then
        dx=rvals(9)
      else
        dx=-999
      endif
      ybeg=rvals(10)

      if(jlineari.eq.1) then
        dy=rvals(11)
      else
        dy=-999
      endif
C         
C         only get time values if a one of the dimensions is time
C     
      if(itype.eq.3.or.jtype.eq.3) then
        yrbeg=rvals(12)
        mobeg=rvals(13)
        dabeg=rvals(14)
        hrbeg=rvals(15)
        mnbeg=rvals(16)
        dmn=rvals(17)
        dmo=rvals(18)
      endif
C         
      if(diag_out) then
        write(iunit_diag,102) undef,itype,jtype,nii,nji,
     $       ilineari,jlineari,xbeg,dx,ybeg,dy
 102    format(' ',
     $       /,10x,'          undefined value = ',e13.5,
     $       /,10x,'         x dimension type = ',i3,
     $       /,10x,'         y dimension type = ',i3,
     $       /,10x,'         x dimension size = ',i5,
     $       /,10x,'         y dimension size = ',i5,
     $       /,10x,' uniform grid spacing in x ? ',i2,
     $       /,10x,' uniform grid spacing in y ? ',i2,
     $       /,10x,'   x world coor beginning  = ',e12.4,
     $       /,10x,'                        dx = ',e12.4,
     $       /,10x,'   y world coor beginning  = ',e12.4,
     $       /,10x,'                        dy = ',e12.4)
        
        
        if(itype.eq.3.or.jtype.eq.3) then
          write(iunit_diag,104) 
     $         yrbeg,mobeg,dabeg,hrbeg,mnbeg,dmn,dmo
 104      format(' ',
     $         /,10x,'                year start = ',e12.4,
     $         /,10x,'               month start = ',e12.4,
     $         /,10x,'                 day start = ',e12.4,
     $         /,10x,'                hour start = ',e12.4,
     $         /,10x,'              minute start = ',e12.4,
     $         /,10x,'           minute increment = ',e12.4,
     $         /,10x,'            month increment = ',e12.4)
        endif
        
        
        if(verb.and.iflag.eq.1) then
          write(iunit_diag,*) 'input grid x boundaries'
          write(iunit_diag,*) (xin(i),i=1,nii)
          write(iunit_diag,*) 'input grid y boundaries'
          write(iunit_diag,*) (yin(j),j=1,nji)

          qtitle='fldin test            '
          call qprntu(fldin,qtitle,undef,1,1,nii,nji,1,iunit_diag)
        endif
      endif

C         
C------------------------------------------------------------
C
C         read arguments after the grid -- two floats
C         
C         #1 - dx or # gaussian lats
C         #2 - dy or # gaussian lons
C         
C         if only 1 then dx=dy
C         if only 2 then method = box averaging
C         
C------------------------------------------------------------
C         
C         decrement ngarg by one for the grid
C         
      nargsl=nargs
      nargsl=nargsl-1
      if(nargsl.ge.2) then
        nargrd=2
      else
        nargrd=1
      endif

C         
C         if one argument, then assume uniform lat lon with equal dx
C         
      if(nargrd.eq.1) then
        iout_grid_type=1
        iregrid_method=1
        dxout=args(1)
        dyout=args(1)

C         
C         base default regrid method, for very short form, 1 parameter call
C         based on the size of the output grid v intput grid
C
C         if output > intput, best to use box averaging
C         if input < output, use bilinear
C         

        if (dxout*dyout.ge.(dx*dy)) then
          iregrid_method = 1
        else
          iregrid_method = 2
          bessel=.false.
        endif


      else 
C         
C         if two arguments, dx & dy are specified the
C         we run box ave by default
C         
        iout_grid_type=1
        iregrid_method=1
        dxout=args(1)
        dyout=args(2)
C
C         method
C         
        iregrid_type=1
        iregrid_method=1
      endif

C         
C------------------------------------------------------------
C         
C         read the third argument -- the char option string
C         
C         this specifies:
C
C         1)  we are telling regrid that the input grid
C         is gaussian and the resolution (number of gaussian latitudes)
C         
C         2)  interp method
C         
C         3)  starting lon,lat of the (1,1) of the output grid
C
C------------------------------------------------------------
C         
C         decrement nargsl based on the last reads
C         

      nargsl=nargsl-nargrd
C         
C         number of char options
C
      nca=0
C         
C         we have char args...
C         
      do i=1,80
        card(i:i)=incard(i)
      end do
      
      if(nargsl.ge.1) then

        lenic=ichar_len(card,80)
C         
C         parse it and check for proper lengths
C         
        nca=1
        ib=1
        do ii=1,lenic
          carg(nca)='          '
          if(incard(ii).eq.'_') then
            ie=ii-1
            ilen=ie-ib+1
            carg(nca)(1:ilen)=card(ib:ie)
            if(carg(nca)(1:2).eq.'ig'.or.carg(nca)(1:2).eq.'ma') then
              if(ilen.eq.2) go to 812
            else
              if(ilen.gt.2) go to 812
            endif

            ib=ie+2
            nca=nca+1

          endif

          if(ii.eq.lenic) then
            ie=lenic
            ilen=ie-ib+1
            carg(nca)(1:ilen)=card(ib:ie)
            if(carg(nca)(1:2).eq.'ig'.or.carg(nca)(1:2).eq.'ma') then
              if(ilen.eq.2) go to 812
            else
              if(ilen.gt.2) go to 812
            endif

          endif

        enddo
C         
C         too many args
C
        if(nca.gt.niopt) go to 814 
C         
C***      process the args
C         
        do ii=1,nca
          iok=0
          do jj=1,nvopt
            if(carg(ii)(1:2).eq.copt(jj)) then
              iok=1
            endif
          enddo
C         
C***      invalid carg
C         
          if(iok.ne.1) then
            nca=ii
            go to 812
          endif
C         
C***      min area for defined data
C
          if(carg(ii)(1:2).eq.'ma') then
            read(carg(ii)(3:5),'(i3)') i1
            area_min=i1*0.01
C         
C         decrease area_min slightly for 100%
C         because of numerical differences between
C         sum of intersections and the actual sfc area
C         of the output grid boxes
C
            if(area_min.ge.1.0) area_min=0.96
          endif
C         
C***      number of gaussian lats in input grid
C***      to invoke the more precise calculation 
C***      of the box boundaries
C
          if(carg(ii)(1:2).eq.'ig') then

            eps_glat=0.01
            read(carg(ii)(3:5),'(i3)') njglat
C         
C         make sure this IS a valid gaussian grid 
C         and can be calculated by the routine
C         
            if(mod(njglat,4).ne.0.or.njglat.lt.8) then
              write(*,'(/,a,/,a,/)') 
     $             'number of input gaussian lats invalid',
     $             '(not a factor of 4) bypass precise weighting'
            else
C         
C         get the latitudes and boundaries using the pcmdi routine
C         

              call gauss_lat_pcmdi(glat,glatb,njglat)

              do j=1,njglat
                jp1=j+1
                if(j.eq.njglat) jp1=njglat
                write(*,'(i3,3(2x,f7.3))') 
     $               j,glat(j),glatb(j),glatb(jp1)
              end do
              
              do j=1,nji
                write(*,'(i3,2x,f7.3)') j,yin(j)
              end do
C         
C         now see if we can match the input grid to the assumed
C         gaussian grid
C         
              jstest=1
              if(yin(jstest).lt.-90.0) jstest=2

              do j=1,njglat
                if(abs(yin(jstest)-glat(j)).le.eps_glat) then
                  if(diag_out) then
                    write(iunit_diag,*) 
     $                   ' We have a near match use the calcuated grid'
                    write(iunit_diag,*) 
     $                   ' jsglat = ',j,' jstest = ',jstest
                    write(iunit_diag,*) 
     $                   ' abs(yin(jstest) =  ',yin(jstest)
                    write(iunit_diag,*) 
     $                   ' glat(j) = ',glat(j)
                  endif
                  jsglat=j
C         
C         quick exit
C         
                  go to 9001
                endif
              end do

 9001         continue

            endif

          endif


        enddo

      endif

C         
C         
C------------------------------------------------------------
C         
C         input grid parameters
C         check for cyclic continuity in x 
C         
C------------------------------------------------------------
C

      cyclicxi=.false.
      wrapxi=.false.
C
C         make sure the x dimension is longitude and is uniform
C         
      if(itype.eq.0.and.ilineari.eq.1) then

C         
C         does input grid have the wrap point in x?
C
        xlen=(nii-1)*dx
        if(xlen.eq.360.0) then
          wrapxi=.true.
          cyclicxi=.true.
          niifix=360.0/dx
          xlen=360.0
        endif


      endif

      if(diag_out) 
     $     write(iunit_diag,*) 'cyclicxi: ',cyclicxi,wrapxi
C         
C------------------------------------------------------------
C
C         if wrapped in x, then trim the grid
C
C         dump the input data into a dummy array
C         and then load the trimmed grid into the original field array
C
C------------------------------------------------------------
C         
      if(wrapxi.and.dowrapfix.and.iflag.eq.1) then
        !!!print*,'WWW--------------- wrapxi=1  dowrapfix1',niifix
        call vload2(dum1,fldin,nii,nji)
        call trim_grid(dum1,nii,niifix,nji,fldin)
C         
C         set the input size to the new trimmed x dimension
C         
        nii=niifix
      endif

C         
C------------------------------------------------------------
C         
C         set up the input grid
C         
C------------------------------------------------------------
C
C         boundaries of input grid boxes
C         
      rlonbegi=xin(1)
      rlonendi=xin(nii)
      if(cyclicxi) rlonedi=xin(1)+360.0

      rlatbegi=yin(1)
      rlatendi=yin(nji)

      niip1=nii+1
      njip1=nji+1

C         
C         x input grid box boundaries
C         
      do i=2,nii
        dumx(i)=0.5*(xin(i-1)+xin(i))
      end do

      dumx(1)=xin(1)-0.5*(xin(2)-xin(1))
      dumx(niip1)=xin(nii)+0.5*(xin(nii)-xin(nii-1))

      do i=1,niip1
        xinb(i)=dumx(i)
      end do
C         
C         y input grid box boundaries
C         
      if(jsglat.ne.-888) then
C         
C         use the calculated gaussian boundaries
C         
        dumx(1)=yin(1)-0.5*(yin(2)-yin(1))
        dumx(njip1)=yin(nji)+0.5*(yin(nji)-yin(nji-1))

        jend=njip1
        if(jend.gt.njglat+1) jend=njglat+1

        if(jstest.eq.1) then

          do j=1,jend
            jj=jsglat+(j-1)
            if(j.ne.1.and.j.ne.njip1) 
     $           dumx(j)=0.5*(yin(j-1)+yin(j))
            if(diag_out) 
     $           write(iunit_diag,*) 'jstest 1 ',j,jj,dumx(j),glatb(jj)
            dumx(j)=glatb(jj)
          end do

        else if(jstest.eq.2) then

          write(iunit_diag,*) 'jstest 2 1 = ',dumx(1)
          do j=2,jend
            jj=jsglat+(j-jstest)
            if(j.ne.1.and.j.ne.njip1) 
     $           dumx(j)=0.5*(yin(j-1)+yin(j))
            if(diag_out) 
     $           write(iunit_diag,*) 'jstest 2 ',j,jj,dumx(j),glatb(jj)
            dumx(j)=glatb(jj)
          end do

        end if


      else
C         
C         use the input lat -> j map for the boundaries
C
        do j=2,nji
          dumx(j)=0.5*(yin(j-1)+yin(j))
        end do

        dumx(1)=yin(1)-0.5*(yin(2)-yin(1))
        dumx(njip1)=yin(nji)+0.5*(yin(nji)-yin(nji-1))

      endif
C         
C         make sure the input grid does not extend beyond the poles
C
C mf 20090329 ::  !!!no -- messes up interpolation at pole and 
C
      do j=1,njip1
        yinb(j)=dumx(j)
c        if(yinb(j).lt.-90.0) yinb(j)=-90.0
c        if(yinb(j).gt.90.0) yinb(j)=90.0
      end do

      if(diag_out.and.verb) then
        write(iunit_diag,*) 'input grid x boundaries'
        write(iunit_diag,*) (xinb(i),i=1,niip1)
        write(iunit_diag,*) 'input grid y boundaries'
        write(iunit_diag,*) (yinb(j),j=1,njip1)
      endif
C         
C------------------------------------------------------------
C
C         read arguments after the char options
C         
C         the first is the grid and the second in the char options
C
C------------------------------------------------------------
C         
C         decrement nargsl if there were char opts
C
      if(nca.gt.0) nargsl=nargsl-1

      istrvt=999
      istrp1=999
      
      if(nca.ne.0) then
         
        do ii=1,nca

          if(carg(ii)(1:2).eq.'gg') then
      
C         
C         Gaussian Output Grid
C         
            niog=nint(args(1))
            njog=nint(args(2))
            nwaves=njog/2
            gauss_out=.true.
            
C         
C         calculate the gaussian latitudes and 
C         the grid spacing for the gaussian longitudes
C         
            call gauss_lat_nmc(dumx,njog)
            
            ilinearo=1
            jlinearo=0
            dxout=360.0/float(niog)
            dyout=180.0/float(njog)
            dyoutg=dyout
            iout_grid_type=2
            
          endif
C         
C*****         method
C
          if(carg(ii)(1:2).eq.'vt') then
             iregrid_method=1
            vote=.true.
            if(nargsl.gt.0) istrvt=ii
          else if(carg(ii)(1:2).eq.'ba') then

             iregrid_method=1
C         
C         change the type of map if gauss out grid
C
             if(gauss_out) then
                ilinearo=1
                jlinearo=0
             else
                dxout=args(1)
                dyout=args(2)
             endif
             vote=.false.

          else if(carg(ii)(1:2).eq.'bl') then
            iregrid_method=2
            bessel=.false.
          else if(carg(ii)(1:2).eq.'bs') then
            iregrid_method=2
            bessel=.true.
          end if
C         
C         starting lon/lat of the (1,1) point
C         
          if(carg(ii)(1:2).eq.'p1') then
            istrp1=ii
          endif

        end do
C         
C         SPECIAL OPTIONS
C         
C         case 1 - voting options
C         case 2 - user-defined beginning lat/lon
C         
        if(nargsl.eq.4) then
          
          if(istrvt.eq.999.or.istrp1.eq.999) go to 838 

          if(istrvt.lt.istrp1) then

            rmin_vote_max=args(3)
            rmin_vote_min=args(4)
            rlonbeg_set=args(5)
            rlatbeg_set=args(6)

          else

            rlonbeg_set=args(3)
            rlatbeg_set=args(4)
            rmin_vote_max=args(5)
            rmin_vote_min=args(6)

          endif


        else if(nargsl.eq.2) then
          if(istrvt.ne.999.and.istrp1.ne.999) then
C         
C         both vt and p1 have been set, then use defaults for
C         vt and set the start lon,loat
C
            rlonbeg_set=args(3)
            rlatbeg_set=args(4)
          else if(istrvt.ne.999) then
            rmin_vote_max=args(3)
            rmin_vote_min=args(4)
          else if(istrp1.ne.999) then
            rlonbeg_set=args(3)
            rlatbeg_set=args(4)
          else 
            go to 839
          endif

        else if(nargsl.ne.0) then
          
          go to 839

        endif


      endif
C         
C         QC the input
C         
C         #1 - setting the (1,1) point for a gaussian output grid NOT!
C 
      if(iout_grid_type.eq.2.and.rlonbeg_set.ne.-999.0) then
        rlonbeg_set=-999.0
        rlatbeg_set=-999.0
      endif

      if(diag_out) then
      write(iunit_diag,*) 'iout_grid_type = ',iout_grid_type
      write(iunit_diag,*) 'iregrid_method = ',iregrid_method
      write(iunit_diag,*) '         dxout = ',dxout
      write(iunit_diag,*) '         dyout = ',dyout
      write(iunit_diag,*) '          vote = ',vote
      write(iunit_diag,*) '        bessel = ',bessel
      write(iunit_diag,*) ' rmin_vote_max = ',rmin_vote_max
      write(iunit_diag,*) ' rmin_vote_min = ',rmin_vote_min
      write(iunit_diag,*) '   rlonbeg_set = ',rlonbeg_set
      write(iunit_diag,*) '   rlatbeg_set = ',rlatbeg_set
      endif


C
C------------------------------------------------------------
C         
C         set up the output grid based on the arguments
C
C------------------------------------------------------------

C111111111111111111111111111111111111111111111111111111111111111111111111111111111111111
C
C         iout_grid_type 1 - uniform lat/lon in both directions
C
      if(iout_grid_type.eq.1) then

        rlonbego=int(rlonbegi/dxout)*dxout
        rlonendo=int(rlonendi/dxout)*dxout
        if(cyclicxi) rlonendo=rlonbego+(360.0-dxout)
        rlatbego=int(rlatbegi/dyout)*dyout
        rlatendo=int(rlatendi/dyout)*dyout
C         
C         check for a global input gaussian grid
C         
        yleni=yinb(njip1)-yinb(1)
        if(yleni+dyout.ge.180.0) then
          rlatbego=-90.0
          rlatendo=90.0
        endif

        nio=nint((rlonendo-rlonbego)/dxout)+1
        njo=nint((rlatendo-rlatbego)/dyout)+1

C         
C         make sure we have at least a 2x2 output grid
C 
        if(nio.lt.2) then
          nio=2
          rlonendo=rlonbego+2.0*dxout
        endif
        if(njo.lt.2) then
          njo=2
          rlatendo=rlonbego+2.0*dyout
        endif
C         
C         user defined beginning longitude
C         
        if(rlonbeg_set.ne.-999.0) then
          rlonbego=rlonbeg_set
C         
C         cylic continuity check
C
          if(cyclicxi) then
            nio=nint(360.0/dxout)
          else
            nio=nint((rlonendi-rlonbego)/dxout)+1
          endif
          if(nio.lt.2) nio=2
          rlonendo=rlonbego+(nio-1)*dxout
        endif

        if(rlatbeg_set.ne.-999.0) then
          rlatbego=rlatbeg_set
          njo=nint((rlatendi-rlatbego)/dyout)+1
          if(njo.lt.2) njo=2
          rlatendo=rlatbego+(njo-1)*dyout
C         
C         bounds check
C         
          if(rlatendo.gt.90.0) then
            rlatendo=rlatendo-dyout
            njo=njo-1
          endif

        endif

Coooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
C
C     linear grid -- output dimensions for second call
C
        if(iflag.eq.-999) then
          iflag=1
          rc=1
          return
        endif
        if(diag_out) then
          write(iunit_diag,*) ' '
          write(iunit_diag,*) 'iout_grid_type=1'
          write(iunit_diag,*) 
     $         'i= ',rlonbegi,rlonendi,rlatbegi,rlatendi
          write(iunit_diag,*) 
     $         'o= ',rlonbego,rlonendo,rlatbego,rlatendo
          write(iunit_diag,*) 'nio,njo = ',nio,njo
        endif


C222222222222222222222222222222222222222222222222222222222222222222222222222222222222222
C         
C         type 2 - gaussian in latitude
C
      else if(iout_grid_type.eq.2) then

        rlonbego=int(rlonbegi/dxout)*dxout

        if(cyclicxi) then
          rlonendo=rlonbego+(360.0-dxout)
        else
          rlonendo=int((rlonendi/dxout))*dxout
        endif

        nio=nint((rlonendo-rlonbego)/dxout)+1

C         
C         limits of the input grid (handles gaussian --> gaussian)
C         
        xbegi=yinb(1)
        xendi=yinb(njip1)

Coooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
C
C     GAUSSIAN grid -- output dimensions for second call
C
        
        if(iflag.eq.-999) then
          jj=0
          do j=1,njog
            if(dumx(j).ge.xbegi.and.dumx(j).le.xendi) then
              jj=jj+1
            endif
          end do

          njo=jj
          rlatbego=dumx(1)
          rlatendo=dumx(njo)
          njog=njo
          iflag=1
          rc=1
          return

        endif

        if(iflag.eq.1) then
          jj=0
          do j=1,njog
            if(dumx(j).ge.xbegi.and.dumx(j).le.xendi) then
              jj=jj+1
              yout(jj)=dumx(j)
            endif
          end do

          njo=jj
          rlatbego=yout(1)
          rlatendo=yout(njo)
          

          if(diag_out) then
            write(iunit_diag,*) ' '
            write(iunit_diag,*) 'iout_grid_type=2'
            write(iunit_diag,*) 'dxout = ',dxout
            write(iunit_diag,*)
     $           'i= ',rlonbegi,rlonendi,rlatbegi,rlatendi
            write(iunit_diag,*) 
     $           'o= ',rlonbego,rlonendo,rlatbego,rlatendo
            write(iunit_diag,*) 'nio, njo = ',nio,njo
          endif
        endif
 
      else

C         
C         invalid grid type; abort
C
        go to 834

      endif
C         
C         output grid characteristics to GrADS
C         

      if(iregrid_method.eq.1.and..not.vote) 
     $     regrid_method='box averaging                  '
      if(iregrid_method.eq.1.and.vote)
     $     regrid_method='box averaging with VOTING      '
      if(iregrid_method.eq.2.and..not.bessel)
     $     regrid_method='bilinear interpolation         '
      if(iregrid_method.eq.2.and.bessel)
     $     regrid_method='bessel interpolation           '


      if(iout_grid_type.eq.1.and.iflag.eq.1) then

        if(verb) then
          write(*,'(a)')  ' '
          write(*,'(a)')
     $         'the output grid is UNIFORM lat/lon:'
          write(*,'(a,f5.2,a,f5.2,a)')
     $         'dx = ',dxout,' deg and dy = ',dyout,' deg'
          write(*,'(a,i4,a,i4)')
     $         '# points in i(lon) = ',nio,
     $         '  # points j(lat) = ',njo
          write(*,'(a,f7.2,a,f7.2,a,f6.2,a,f6.2)') 
     $         'lon extent = ',rlonbego,' to ',rlonendo,
     $         ' lat extend = ',rlatbego,' to ',rlatendo
        endif

        jcard=0
        do i=1,njcard; chcard(i:i)=' ' ; enddo
        write(chcard,'(a)')
     $       'the output grid is UNIFORM lat/lon:'
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

        jcard=jcard+1
        do i=1,njcard ; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,f5.2,a,f5.2,a)')
     $       'dx = ',dxout,' deg and dy = ',dyout,' deg'
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

        jcard=jcard+1
        do i=1,njcard ; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,i4,a,i4)')
     $       '# points in i(lon) = ',nio,
     $       '  # points j(lat) = ',njo
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

        jcard=jcard+1
        do i=1,njcard ; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,f7.2,a,f7.2,a,f6.2,a,f6.2)') 
     $       'lon extent = ',rlonbego,' to ',rlonendo,
     $       ' lat extend = ',rlatbego,' to ',rlatendo
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

      endif

      if(iout_grid_type.eq.2.and.iflag.eq.1) then
        if(verb) then
          write(*,'(a)')  ' '
          write(*,'(a,i3,a)')
     $         'the output grid is  N# (points EQ-Pole): ',
     $          nwaves,' GAUSSIAN'
          write(*,'(a,f7.3,a,f6.3,a)')
     $         'dx = ',dxout,' deg and dy ~ ',dyoutg,' deg'
          write(*,'(a,i4,a,i4)')
     $         '# points in i(lon) = ',nio,'  # points j(lat) = ',njo
          write(*,'(a,f7.2,a,f7.2,a,f6.2,a,f6.2)') 
     $         'lon extent = ',rlonbego,' to ',rlonendo,
     $         '   lat extend = ',rlatbego,' to ',rlatendo
        endif

        jcard=0
        do i=1,njcard; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,i3,a)')
     $         'the output grid is  N# (points EQ-Pole): ',
     $         nwaves,' GAUSSIAN'
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

        jcard=jcard+1
        do i=1,njcard ; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,f7.3,a,f6.3,a)')
     $       'dx = ',dxout,' deg and dy ~ ',dyoutg,' deg'
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

        jcard=jcard+1
        do i=1,njcard ; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,i4,a,i4)')
     $       '# points in i(lon) = ',nio,'  # points j(lat) = ',njo
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

        jcard=jcard+1
        do i=1,njcard ; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,f7.2,a,f7.2,a,f6.2,a,f6.2)') 
     $       'lon extent = ',rlonbego,' to ',rlonendo,
     $       '   lat extend = ',rlatbego,' to ',rlatendo
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

      
        if(verb) then
          write(*,'(a,a)') 
     $       'regrid method is: ',regrid_method
          if(vote) then
            write(*,'(a,f4.2,a,f4.2)')
     $           'vote parameters:  max fract area = ',rmin_vote_max,
     $           '  min frac area = ',rmin_vote_min
          endif
          write(*,'(a)')  ' '
        endif

        jcard=jcard+1
        do i=1,njcard ; chcard(i:i)=' ' ; enddo
        write(chcard,'(a,a)') 
     $         'regrid method is: ',regrid_method
        do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo

        if(vote) then
         jcard=jcard+1
         do i=1,njcard ; chcard(i:i)=' ' ; enddo
         write(chcard,'(a,f4.2,a,f4.2)')
     $       'vote parameters:  max fract area = ',rmin_vote_max,
     $       '  min frac area = ',rmin_vote_min
         do i=1,njcard ; chrc(i+jcard*njcard)=chcard(i:i) ; enddo
        endif

      endif

C
C         boundaries of the output boxes
C         
      niop1=nio+1
      njop1=njo+1

C
C         X
C         

      if(iflag.eq.1) then
        do i=1,niop1
          xout(i)=rlonbego+(i-1)*dxout-0.5*dxout
        end do
      endif
C         
C         y
C         
C         make sure the output grid does not extend beyond the poles
C         

      if(iflag.eq.1 ) then

        if(iout_grid_type.eq.1) then
          do j=1,njop1
            yout(j)=rlatbego+(j-1)*dyout-0.5*dyout
            if(yout(j).lt.-90.0) yout(j)=-90.0
            if(yout(j).gt.90.0) yout(j)=90.0
          end do
C         
C         check for pole points
C         
          if(rlatendo.eq.90.0) npole_point=.true.
          if(rlatbego.eq.-90.0) spole_point=.true.
          
        else if(iout_grid_type.eq.2) then
          
          dumx(1)=yout(1)-0.5*(yout(2)-yout(1))
          dumx(njop1)=yout(njo)+0.5*(yout(njo)-yout(njo-1))
          do j=2,njo
            dumx(j)=0.5*(yout(j-1)+yout(j))
          end do
          
          do j=1,njop1
            yout(j)=dumx(j)
          end do
        endif

      endif

C         
C------------------------------------------------------------
C
C         input-output grid box relationship 
C
C------------------------------------------------------------
C         
C         make sure longitudes of the input/output grids
C         are in positive deg
C         
      if(xinb(1).lt.0.0) then
        do i=1,niip1
          xinb(i)=xinb(i)+360.0
        enddo
      endif
         
      if(iflag.eq.1 ) then
        if(xout(1).lt.0.0) then
          do i=1,niop1
            xout(i)=xout(i)+360.0
          enddo
        endif
      endif
C         
C         calculate the location of the output grid box boundaries 
C         w.r.t. the input grid box boundaries
C

      call in_out_boundaries(xinb,yinb,nii,nji,
     $     xout,yout,nio,njo,cyclicxi,
     $     niip1,njip1,niop1,njop1,
     $     iunit_diag,
     $     gxout,gyout)
      
      if(verb.and.iflag.eq.1) then
        write(iunit_diag,*) 'output grid x boundaries'
        do i=1,niop1
          write(iunit_diag,*) ' i = ',i,' xout = ',xout(i),
     $         ' gxout = ',gxout(i)
        end do
        write(iunit_diag,*) 'output grid y boundaries'
        do j=1,njop1
          write(iunit_diag,*) ' j = ',j,' yout = ',yout(j),
     $         ' gyout = ',gyout(j)
        end do
      endif

C         
C         calculate sfc area of each grid box of input grid
C         

      if(iflag.eq.1) then

        call sfc_area(fldin,xinb,yinb,undef,nii,nji,
     $       area_in,iunit_diag)

        call sfc_area(fldout,xout,yout,undef,nio,njo,
     $       area_out,iunit_diag)

        if(verb) then
          qtitle='output sfc_area unit s '
          call qprntu(area_out,qtitle,undef,1,1,nio,njo,4,iunit_diag)
        endif

      endif


C         
C------------------------------------------------------------
C         
C         do the regrid
C
C------------------------------------------------------------
C
C         box averaging or "clumping" with a "voting" option
C         where the output grid equals the value of the
C         input grid which accounts for the most area.  voting
C         is used for discontinuos data such as soil type
C

      if(iregrid_method.eq.1) then

        call box_ave(fldin,area_in,area_out,area_min,
     $       undef,gxout,gyout,
     $       nii,nji,nio,njo,fldout,iunit_diag,vote,istat,
     $       rmin_vote_max,rmin_vote_min)
C         
C         FNOC bilinear/bessel interpolation
C         
      else if(iregrid_method.eq.2) then

       call bssl_interp(fldin,undef,
     $       xout,yout,
     $       gxout,gyout,
     $       nii,nji,nio,njo,fldout,iunit_diag,
     $       cyclicxi,spole_point,npole_point,bessel,istat)

      endif
C         
C         check for pole points
C         
      if(spole_point.or.npole_point) then
        call fix_poles(fldout,nio,njo,undef,
     $       spole_point,npole_point)
      endif

      if(verb) then
        qtitle='fldin                '
        call qprntu(fldin,qtitle,undef,1,1,nii,nji,1,iunit_diag)
        qtitle='fldout                '
        call qprntu(fldout,qtitle,undef,1,1,nio,njo,1,iunit_diag)
      endif
        
      do i=1,20
        ovals(i)=rvals(i)
      enddo

C         
C         modify the appropriate input grid parameters for 
C         defining the output grid for GrADS
C

      ovals(4)=float(nio)
      ovals(5)=float(njo)

      ovals(6)=float(ilinearo)
      ovals(7)=float(jlinearo)

      ovals(8)=rlonbego
      ovals(9)=dxout
      if(iout_grid_type.eq.1) then
        ovals(10)=rlatbego
        ovals(11)=dyout
      else if(iout_grid_type.eq.2) then
        ovals(10)=0.0
        ovals(11)=0.0
      endif

C         
C         define the lat/lon of the output grid points
C         
      if(ilinearo.eq.0) then
        do i=1,nio
          xout(i)=rlonbego+(i-1)*dxout
        end do
      endif

      if(iout_grid_type.eq.1) then

        do j=1,njo
          dumx(j)=(yout(j)+yout(j+1))*0.5
          yout(j)=rlatbego+(j-1)*dyout
        end do

      else if(iout_grid_type.eq.2) then

        do j=1,njo
          dumx(j)=(yout(j)+yout(j+1))*0.5
        end do
        do j=1,njo
          yout(j)=dumx(j)
        end do

      endif

C         
C reverse order for C         
C

      call vload2(area_out,fldout,nio,njo)

C      do j=1,njo
C        do i=1,nio
C          fldout(j,i)=area_out(i,j)
C        enddo
C      enddo

      go to 999
C         
C         error conditions 
C
 810  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 810'
      write(*,*) 'unable to open the GrADS input file to regrid'
      write(*,*) ' '
      go to 888

 812  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 812'
      write(*,'(a,a,a)') 
     $     'invalid character option in arg #4 = (',
     $     carg(nca)(1:2),') try again'
      write(*,*) ' '
      go to 888

 814  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 814'
      write(*,'(a,i1,a)') 
     $     'too many options in character arg #4 = (',
     $     nca,') try again'
      write(*,*) ' '
      go to 888

 820  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 820'
      write(*,*) 'the first arguement to regrid is not a grid'
      write(*,*) ' '
      go to 888
      
 830  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 830'
      write(*,*) 'the grid must be 2-d and x-y (lon-lat)' 
      write(*,*) ' '
      go to 888

 832  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 832'
      write(*,*) 'arguments must be numbers' 
      write(*,*) ' '
      go to 888

 834  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 834'
      write(*,*) 'invalid output grid iout_grid_type  = ',
     $     iout_grid_type 
      write(*,*) ' '
      go to 888

 836  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 836'
      write(*,*) 'invalid interpolation option',
     $     ' iregrid_type = ',iregrid_type 
      write(*,*) ' '
      go to 888

 838  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 838'
      write(*,*) '4 or more special case arguments, not a special case'
      write(*,*) ' '
      go to 888

 839  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 839'
      write(*,*) 'too few or wrong number of options for special cases'
      write(*,*) ' '
      go to 888

 840  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 840'
      write(*,*) 'rmin_vote outside the interval [1,0]'
      write(*,*) ' '
      return
      go to 888

 842  continue
      write(*,*) ' '
      write(*,*) 'regrid ERROR 842'
      write(*,*) 'dlon/dlat greater than zero when attempting'
      write(*,*) 'to set beginning lat/lon of the output grid'
      write(*,*) ' '
      go to 888

C
C     got here because it worked
C
 999  continue
      rc=0
      return

C
C     the was a problem set return code to failure
C
 888  continue
      rc=1
      return

      end

