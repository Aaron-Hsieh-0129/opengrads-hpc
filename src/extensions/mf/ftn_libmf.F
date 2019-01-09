      subroutine box_ave(fldin,area_in,area_out,area_min,
     $     undef,gxout,gyout,
     $     nii,nji,nio,njo,fldout,iunit_diag,vote,istat,
     $     rmin_vote_max,rmin_vote_min)

      USE gex

      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      parameter (na=300)
      dimension fldin(nii,nji),area_in(nii,nji),
     $     fldout(nio,njo),area_out(nio,njo),
     $     gxout(nio+1),gyout(njo+1)


      dimension area_box(na),fld_box(na),dxdy_box(na),ifld_rank(na),
     $     fld_cand(na),area_cand(na),dxdy_cand(na),rmin_dxdy_vote(3)

      logical cyclicx,diag_out,vote
      
      diag_out=.false.
ccc      diag_out=.true.
      istat=1

            
C         
C         tolerance for checking whether grid is undefined (zero sfc area)
C
      eps=1.0e-12
C         
C         minimum fractional area of the output grid box
C         in order to have a winner in the voting
C
      rmin_dxdy_vote(1)=rmin_vote_max
      rmin_dxdy_vote(2)=(rmin_vote_max+rmin_vote_min)*0.5
      rmin_dxdy_vote(3)=rmin_vote_min

      ibb=1
      iee=nio
      jbb=1
      jee=njo

      do j=jbb,jee
        do i=ibb,iee
          
          ib=int(gxout(i))+1
          ie=int(gxout(i+1))+1
          jb=int(gyout(j))+1
          je=int(gyout(j+1))+1
C         
C         check for exceeding n pole
C         
          if(je.gt.nji) je=nji
          if(jb.lt.1) jb=1

C         
C         cyclic continuity in x
C         
          cyclicx=.false.
          if(ie.lt.ib) then
            ie=ie+nii
            cyclicx=.true.
          endif
          
          if(diag_out) then
            write(iunit_diag,*) ' '
            write(iunit_diag,*)
     $           'i,j,ib,ie,jb,je = ',i,j,ib,ie,jb,je
          endif
C         
C         initialize the counter for intersecting grid boxes
C         
          icnt=0
C         
C         CASE 1:  only one input grid box in output grid box
C         
          if(ib.eq.ie.and.jb.eq.je) then
            
            icnt=1
            dxdy_box(icnt)=1.0
            area_box(icnt)=area_in(ib,jb)
            if(area_box(icnt).eq.0.0) dxdy_box(icnt)=0.0
            fld_box(icnt)=fldin(ib,jb)
            
            if(diag_out) then
              write(iunit_diag,*)
     $             'ib=ie and jb=je ',area_box(icnt),dxdy_box(icnt),
     $             fld_box(icnt)
            endif
            
          else if(ib.eq.ie) then
C         
C         CASE 2:  intersecting boxes in y only
C         
            ii=ib
            dx=gxout(i+1)-gxout(i)
            dxout=1.0

            do jj=jb,je
              
              icnt=icnt+1
              
              if(icnt.gt.na) then
                istat=0
                go to 999
              endif
              
              if(jj.eq.jb) then
                dy=float(jj)-gyout(j)
              else if(jj.eq.je) then
                dy=gyout(j+1)-float(jj-1)
              else
                dy=1.0
              endif
              
              if(jj.eq.jb.or.jj.eq.je) then
                dyout=dy/(gyout(j+1)-gyout(j))
              else
                dyout=1.0
              endif

              dxdy_box(icnt)=dxout*dyout
              area_box(icnt)=dx*dy*area_in(ii,jj)
              if(area_in(ii,jj).eq.0.0) dxdy_box(icnt)=0.0
              fld_box(icnt)=fldin(ii,jj)
              
              if(diag_out) then
                write(iunit_diag,*)
     $               'ib=ie ',ii,jj,dx,dy,area_in(ii,jj)
                write(iunit_diag,*)
     $               'area_box ... ',area_box(icnt),fld_box(icnt)
                write(iunit_diag,*)
     $               'dx dyout ... ',dxout,dyout,dxdy_box(icnt)
              endif
              
            end do
            
          else if(jb.eq.je) then
C         
C         CASE 3:  intersecting boxes in x only
C         
            jj=jb
            dy=gyout(j+1)-gyout(j)
            dyout=1.0

            do ii=ib,ie

              icnt=icnt+1
              if(icnt.gt.na) then
                istat=0
                go to 999
              endif

              ii0=ii
              if(cyclicx.and.ii0.gt.nii) ii0=ii-nii
              if(ii.eq.ib) then
                dx=float(ii)-gxout(i)
              else if(ii.eq.ie) then
                x0=float(ii-1)
                if(cyclicx) x0=float(ii0)-1.0
                dx=gxout(i+1)-x0
              else
                dx=1.0
              endif

              if(ii.eq.ib.or.ii.eq.ie) then
                dxout=dx/(gxout(i+1)-gxout(i))
              else
                dxout=1.0
              endif

              dxdy_box(icnt)=dxout*dyout

              area_box(icnt)=dx*dy*area_in(ii0,jj)
              if(area_in(ii0,jj).eq.0.0) dxdy_box(icnt)=0.0
              fld_box(icnt)=fldin(ii0,jj)
              
              if(diag_out) then
                write(iunit_diag,*)
     $               'jb=je ',ii,ii0,jj,dx,dy,area_in(ii0,jj)
                write(iunit_diag,*)
     $               'area_box ... ',area_box(icnt),fld_box(icnt)
                write(iunit_diag,*)
     $               'dx dyout ... ',dxout,dyout,dxdy_box(icnt)
              endif
              
            end do
            
          else
C         
C         CASE 4:  intersecting boxes in both directions
C         
            do jj=jb,je

              if(jj.eq.jb) then
                dy=float(jj)-gyout(j)
              else if(jj.eq.je) then
                dy=gyout(j+1)-float(jj-1)
              else
                dy=1.0
              endif

              if(jj.eq.jb.or.jj.eq.je) then
                dyout=dy/(gyout(j+1)-gyout(j))
              else
                dyout=1.0
              endif

              do ii=ib,ie
                icnt=icnt+1
                if(icnt.gt.na) then
                  istat=0
                  go to 999
                endif
                
                ii0=ii
                if(cyclicx.and.ii0.gt.nii) ii0=ii-nii
                if(ii.eq.ib) then
                  dx=float(ii)-gxout(i)
                else if(ii.eq.ie) then
                  x0=float(ii-1)
                  if(cyclicx) x0=float(ii0)-1.0
                  dx=gxout(i+1)-x0
                else
                  dx=1.0
                endif

                if(ii.eq.ib.or.ii.eq.ie) then
                  dxout=dx/(gxout(i+1)-gxout(i))
                else
                  dxout=1.0
                endif

                dxdy_box(icnt)=dxout*dyout
                area_box(icnt)=dx*dy*area_in(ii0,jj)
                if(area_in(ii0,jj).eq.0.0) dxdy_box(icnt)=0.0
                fld_box(icnt)=fldin(ii0,jj)
                

                if(diag_out) then
                  write(iunit_diag,*) 'ib.ne.ib.and.jb.ne.je ',
     $                 ii,ii0,jj,dx,dy,area_in(ii0,jj)
                  write(iunit_diag,*)
     $                 'area_box ... ',area_box(icnt),fld_box(icnt)
                  write(iunit_diag,*)
     $                 'dx dyout ... ',dxout,dyout,dxdy_box(icnt)
                endif
                
              end do
              
            enddo
            
          endif
C         
C         integrate or vote for the average value
C         
          if(vote) then
C         
C         voting routine; first get total area
C         
            
            tot_area=0.0
            do ii=1,icnt
              tot_area=tot_area+area_box(ii)
            end do
            
            if(diag_out) then
              write(iunit_diag,*) 'i,j,icnt,tot_area = ',
     $             i,j,icnt,tot_area
            endif
            
            
            if(tot_area.le.eps) then
              fldout(i,j)=undef
              go to 100
            endif
            
            if(icnt.eq.1) then
C         
C         USSR election -- only one "candidate" to vote for
C         
C         check if the the total area is greater
C         than the minimum required to hold the election (e.g., 0.5)
C         
              if(diag_out) then
                write(iunit_diag,*)
     $               'USSR:  ',dxdy_box(1),rmin_dxdy_vote(1),fld_box(1)
              endif

              if(dxdy_box(1).lt.rmin_dxdy_vote(1)) then
                fldout(i,j)=undef
              else
                fldout(i,j)=fld_box(1)
              endif
              
              go to 100
              
            else if(icnt.eq.2) then
C         
C         USA election -- two-party, two-candidate race; area wins
C         
              if(diag_out) then
                write(iunit_diag,*)'USA: ',dxdy_box(1),dxdy_box(2),
     $               rmin_vote
              endif

              if(dxdy_box(1).eq.0.0.or.dxdy_box(2).eq.0.0) then
                rmin_vote=rmin_dxdy_vote(1)
              else if(fld_box(1).eq.fld_box(2)) then
                rmin_vote=rmin_dxdy_vote(1)
              else
                rmin_vote=rmin_dxdy_vote(2)
              endif

              if((dxdy_box(1).ge.dxdy_box(2)).and.
     $             (dxdy_box(1).gt.rmin_vote)) then
                fldout(i,j)=fld_box(1)
              else if(dxdy_box(2).gt.rmin_vote) then
                fldout(i,j)=fld_box(2)
C         
C         case where both candidates are the same in the two-person race
C
              else if(fld_box(1).eq.fld_box(2).and.
     $               ((dxdy_box(1)+dxdy_box(2)).gt.rmin_vote)) then
                fldout(i,j)=fld_box(2)
              else
                fldout(i,j)=undef
              endif
              
            else
C         
C         a wide open election - three or more candidates
C         
C         sort the data by surface area using 
C         the numercial recipes routine --  indexx
C         
              call indexx(icnt,fld_box,ifld_rank)
              
              if(diag_out) then
                write(iunit_diag,*) 
     $               'fld_box = ',(fld_box(ipp),ipp=1,icnt)
                write(iunit_diag,*) 
     $               'area_box = ',(area_box(ipp),ipp=1,icnt)
                write(iunit_diag,*) 
     $               'ifld_rank = ',(ifld_rank(ipp),ipp=1,icnt)
                write(iunit_diag,*)
              endif
C         
C         the indexes are in reverse order, with the
C         biggest fld element in the last element of the array
C         first check if the biggest is in the majority
C         
C         set up the candidates
C         
              ncand=1
              it1=ifld_rank(1)
              
              area_cand(ncand)=area_box(it1)
              fld_cand(ncand)=fld_box(it1)
              dxdy_cand(ncand)=dxdy_box(it1)
              
              do ii=2,icnt
                
                i1=ii-1
                i2=ii
                it1=ifld_rank(i1)
                it2=ifld_rank(i2)
                
                if(fld_box(it1).eq.fld_box(it2)) then
                  area_cand(ncand)=area_cand(ncand)+area_box(it2)
                  dxdy_cand(ncand)=dxdy_cand(ncand)+dxdy_box(it2)
                else
                  ncand=ncand+1
                  area_cand(ncand)=area_box(it2)
                  dxdy_cand(ncand)=dxdy_box(it2)
                  fld_cand(ncand)=fld_box(it2)
                endif
                
              end do
              if(diag_out) then
                write(iunit_diag,*) 'ncand = ',ncand
                write(iunit_diag,*) 
     $               'fld_cand = ',(fld_cand(ipp),ipp=1,ncand)
                write(iunit_diag,*) 
     $               'area_cand = ',(area_cand(ipp),ipp=1,ncand)
                write(iunit_diag,*) 
     $               'dxdy_cand = ',(dxdy_cand(ipp),ipp=1,ncand)
              endif
C         
C         if one candidate, all done
C         
              if(ncand.eq.1) then
                if(dxdy_cand(1).gt.rmin_dxdy_vote(3)) then
                  fldout(i,j)=fld_cand(1)
                else
                  fldout(i,j)=undef
                endif
                go to 100
              else
                
C         
C         the candidate with the most area is the winner
C         
                area_max=0.0
                do ii=1,ncand
                  if(area_cand(ii).gt.area_max) then
                    iamax=ii
                    area_max=area_cand(ii)
                  endif
                end do
                
                if(ncand.le.2) then
                  rmin_vote=rmin_dxdy_vote(ncand)
                else
                  rmin_vote=rmin_dxdy_vote(3)
                endif

                if(dxdy_cand(iamax).gt.rmin_vote) then
                  fldout(i,j)=fld_cand(iamax)
                else
                  fldout(i,j)=undef
                endif
                
                if(diag_out) then
                  write(iunit_diag,*) ' '
                  write(iunit_diag,*) 'the winner is...',
     $                 iamax,area_max,dxdy_cand(iamax),
     $                 rmin_vote,fldout(i,j)
                endif
                
                go to 100
                
              endif
              
            endif
            
            
          else
C         
C         area integrate
C         
            tot_fld=0.0
            tot_area=0.0
C         
            do ii=1,icnt
              tot_fld=tot_fld+fld_box(ii)*area_box(ii)
              tot_area=tot_area+area_box(ii)
            end do
            
            if(tot_area.gt.area_out(i,j)*area_min) then
              fldout(i,j)=tot_fld/tot_area
            else
              fldout(i,j)=undef
            endif
            
            if(diag_out) then
              write(iunit_diag,*) 'qqq ',tot_area,' ',
     $             area_out(i,j)*area_min,' ',
     $             area_out(i,j),' ',fldout(i,j)
            endif

            
          endif
          
 100      continue
          
          if(diag_out) then
            write(iunit_diag,*)
     $           'i,j fldout(i,j) = ',i,j,fldout(i,j)  
          endif
          
        end do
      end do
      
 999  continue
      
      return
      end

      subroutine bssl_interp(fldin,undef,
     $     xout,yout,
     $     gxout,gyout,
     $     nii,nji,nio,njo,fldout,iunit_diag,
     $     cyclicxi,spole_point,npole_point,bessel,istat)


      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension fldin(nii,nji),fldout(nio,njo),
     $     xout(nio+1),yout(njo+1),
     $     gxout(nio+1),gyout(njo+1)

      dimension fr(4)

      logical cyclicxi,diag_out,bessel,spole_point,npole_point
      
      logical verb
      
      verb=.false.

      diag_out=.false.
ccc      diag_out=.true.

      jchk=2
      ichk=2

      istat=1
C         
C         convert the box boundaries to grid point center
C         

      do i=1,nio
C         
C         check for crossing the boundaries 
C         the only way for this to occur is if the field is 
C         cyclically continuous in x
C

        if(gxout(i+1).lt.gxout(i)) then
          gxout(i)=((gxout(i)-float(nii))+gxout(i+1))*0.5+0.5
          if(gxout(i).lt.1.0) gxout(i)=float(nii)+gxout(i)
        else
          gxout(i)=(gxout(i)+gxout(i+1))*0.5+0.5
          if(gxout(i).lt.1.0) gxout(i)=float(nii)+gxout(i)
        endif
      end do

      do j=1,njo

        y1=yout(j)
        y2=yout(j+1)
        ycenter=(y1+y2)*0.5

        gy1=gyout(j)
        gy2=gyout(j+1)
        gycenter=(gyout(j)+gyout(j+1))*0.5

        if(gycenter.lt.0.5) then
          gymid=gycenter+0.5
        else
          gymid=gycenter+0.5
        endif

C         
C         check if a pole points on the input grid
C         
        gyout(j)=gymid
        jc=ifix(real(gyout(j)))
        s=gyout(j)-jc

        if(spole_point.and.(y1.le.-90.and.y2.gt.y1)) then
          if(verb) write(iunit_diag,*) 'ddddddddd SSSSSS pole corr'
          gyout(j)=1.0
          jc=1
          s=gyout(j)-jc
        endif

        if(npole_point.and.(y2.ge.90.and.y1.lt.y2)) then
          if(verb) write(iunit_diag,*) 'dddddddddd NNNNNN pole corr'
          gyout(j)=real(nji)
          jc=nji
          s=gyout(j)-jc
        endif

        if(npole_point.and.gyout(j).gt.(real(nji)+0.5)) 
     $       gyout(j)=real(nji)

        
        if(verb) then
          write(iunit_diag,'(a,i3,10(f9.3),i3)') 
     $         'ppppppppppppppp ',j,
     $         y1,y2,ycenter,gy1,gy2,gycenter,gymid,yout(j),s,gyout(j),jc
        endif

      end do

      ibb=1
      iee=nio
      jbb=1
      jee=njo

      niim1=nii-1
      njim1=nji-1
      
      do j=jbb,jee
        do i=ibb,iee

          ic=ifix(real(gxout(i)))
          jc=ifix(real(gyout(j)))
          icp1=ic+1

          if(cyclicxi.and.icp1.gt.nii) icp1=icp1-nii 
          jcp1=jc+1
          
          if(diag_out.and.i.eq.ichk) write(iunit_diag,*)
     $         'i,j,gxout,gyout,ic,jc',
     $         i,j,gxout(i),gyout(j),ic,jc
          
          if((jc.lt.1.or.jc.gt.nji).or.
     $         (.not.cyclicxi.and.(ic.lt.1.or.ic.gt.nii))) then
            fldout(i,j)=undef
            if(diag_out.and.i.eq.ichk) 
     $           write(iunit_diag,*) 'out of bounds ' 
            go to 100
          end if
C         
C------------------------------------------------
C         
C         bilinear/bessel interpolation based on the FNOC routine
C         bssl5 by D. Hensen, FNOC
C         
C------------------------------------------------
C         
          r=gxout(i)-ic
          s=gyout(j)-jc

C         
C         interior check
C         
          if((jc.ge.2.and.jc.lt.njim1.and.cyclicxi).or.
     $         (ic.ge.2.and.jc.ge.2.and.
     $         ic.lt.niim1.and.jc.lt.njim1)) then
            go to 10
          endif
C         
C         border zone check
C         
          if((jc.lt.nji.and.cyclicxi).or.
     $         (ic.lt.nii.and.jc.lt.nji)) then
            go to 5 
          endif
C         
C------------------------------------------------
C         
C         top and right edge processing
C         
C------------------------------------------------
C         
          if(ic.eq.nii.and.jc.eq.nji) then
            
            fldout(i,j)=fldin(nii,nji)
            if(diag_out.and.i.eq.ichk) 
     $           write(iunit_diag,*) 'upper right hand corenr'
            
          else if(ic.eq.nii) then
            
            if(diag_out.and.i.eq.ichk) 
     $           write(iunit_diag,*) 'right edge'
            
            if(fldin(ic,jc).ne.undef.and.
     $           fldin(ic,jcp1).ne.undef) then
              fldout(i,j)=(1.0-s)*fldin(ic,jc)+
     $             s*fldin(ic,jcp1)
            else
              fldout(i,j)=undef
            endif
            
          else if(jc.eq.nji) then
            
            if(diag_out.and.i.eq.ichk) 
     $           write(iunit_diag,*) 'top edge'
            
            if(fldin(ic,jc).ne.undef.and.
     $           fldin(icp1,jc).ne.undef) then
              fldout(i,j)=(1.0-r)*fldin(ic,jc)+
     $             r*fldin(icp1,jc)
            else
              fldout(i,j)=undef
            endif

C         
C         bottom row
C
          else if(jc.eq.1) then
            if(diag_out.and.i.eq.ichk) 
     $           write(iunit_diag,*) 'bottom edge'
            
            if(fldin(ic,jc).ne.undef.and.
     $           fldin(icp1,jc).ne.undef) then
              fldout(i,j)=(1.0-r)*fldin(ic,jc)+
     $             r*fldin(icp1,jc)
            else
              fldout(i,j)=undef
            endif
            
          endif
          
          go to 100
          
 5        continue
C         
C------------------------------------------------
C         
C         border zone; bilinear
C         
C------------------------------------------------
C         
          iok_bilinear=1
          if(fldin(ic,jc).eq.undef.or.
     $         fldin(icp1,jc).eq.undef.or.
     $         fldin(ic,jcp1).eq.undef.or.
     $         fldin(icp1,jcp1).eq.undef) iok_bilinear=0
          
          if(diag_out.and.i.eq.ichk) 
     $         write(iunit_diag,*) 
     $         'border zone, iok_bilinear= ',iok_bilinear,ic,jc,icp1,jcp1
                    
          if(iok_bilinear.eq.0) then
            fldout(i,j)=undef
          else
            fldout(i,j)=(1.-s)*((1.-r)*fldin(ic,jc)
     $           +r*fldin(icp1,jc))
     $           +s*((1.-r)*fldin(ic,jcp1)
     $           +r*fldin(icp1,jcp1))
          endif

          go to 100
 
 10       continue
C         
C------------------------------------------------
C         
C         interior zone
C         
C------------------------------------------------
C         
C         first check if bilinear is OK
C         
          iok_bilinear=1
          if(fldin(ic,jc).eq.undef.or.
     $         fldin(icp1,jc).eq.undef.or.
     $         fldin(ic,jcp1).eq.undef.or.
     $         fldin(icp1,jcp1).eq.undef) iok_bilinear=0
          
          if(diag_out.and.i.eq.ichk) 
     $         write(iunit_diag,*) 
     $         'interior zone, iok_bilinear= ',iok_bilinear
          
          if(iok_bilinear.eq.0) then
            fldout(i,j)=undef
            go to 100
          else 
C         
C         bilinear value is the first guess
C         
            fldout(i,j)=(1.-s)*((1.-r)*fldin(ic,jc)
     $           +r*fldin(icp1,jc))
     $           +s*((1.-r)*fldin(ic,jcp1)
     $           +r*fldin(icp1,jcp1))
C         
C         exit if only doing bilinear
C         

            if(.not.bessel) go to 100
            
          endif
          
          
C         
C         interpolate 4 columns (i-1,i,i+1,i+2) to j+s and store in fr(1)
C         through fr(4)
C         
          r1=r-0.5
          r2=r*(r-1.)*0.5
          r3=r1*r2*0.3333333333334
          s1=s-0.5
          s2=s*(s-1.)*0.5
          s3=s1*s2*0.3333333333334
C         
          k=0
          im1=ic-1
          ip2=ic+2
          
          if(diag_out.and.j.eq.jck) write(iunit_diag,*) 'bessel interp'
          
C         
          do ii=im1,ip2
            
            k=k+1
            
            i1=ii

            if(cyclicxi.and.i1.lt.1) i1=nii-i1
            if(cyclicxi.and.i1.gt.nii) i1=i1-nii
            
            j1=jc
            
            j1p1=j1+1
            j1p2=j1+2
            j1m1=j1-1
            
            fij=fldin(i1,j1)
            fijp1=fldin(i1,j1p1)
            fijp2=fldin(i1,j1p2)
            fijm1=fldin(i1,j1m1)
            if(diag_out.and.i.eq.ichk) 
     $           write(iunit_diag,*) 'i1,j1 = ',i1,j1,
     $           fij,fijp1,fijp2,fijm1
C         
C         exit if any value undefined
C         
            if(fij.eq.undef.or.
     $           fijp1.eq.undef.or.
     $           fijp2.eq.undef.or.
     $           fijm1.eq.undef) go to 100 
            
            u=(fij+fijp1)*0.5
            del=fijp1-fij
            udel2=(fijp2-fijp1+fijm1-fij)*0.5
            del3=fijp2-fijp1-2.*del+fij-fijm1
            
            fr(k)=u+s1*del+s2*udel2+s3*del3
            
          end do
          
C         
C         interpolate the fr row to ii+r
C         
          u=(fr(2)+fr(3))*0.5
          del=fr(3)-fr(2)
          udel2=(fr(4)-fr(3)+fr(1)-fr(2))*0.5
          del3=fr(4)-fr(3)-2.*del+fr(2)-fr(1)
          
          fldout(i,j)=u+r1*del+r2*udel2+r3*del3
          
          
 100      continue

          if(diag_out.and.i.eq.ichk) 
     $         write(iunit_diag,*) 'interp value = ',i,j,ic,jc,
     $         fldin(ic,jc),fldin(ic,jcp1),s,fldout(i,j)
          
        end do
      end do
      
 999  continue
      
      return
      end
      
      subroutine indexx(n,arrin,indx)
      
      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension arrin(n),indx(n)
      do 11 j=1,n
        indx(j)=j
11    continue
      l=n/2+1
      ir=n
10    continue
        if(l.gt.1)then
          l=l-1
          indxt=indx(l)
          q=arrin(indxt)
        else
          indxt=indx(ir)
          q=arrin(indxt)
          indx(ir)=indx(1)
          ir=ir-1
          if(ir.eq.1)then
            indx(1)=indxt
            return
          endif
        endif
        i=l
        j=l+l
20      if(j.le.ir)then
          if(j.lt.ir)then
            if(arrin(indx(j)).lt.arrin(indx(j+1)))j=j+1
          endif
          if(q.lt.arrin(indx(j)))then
            indx(i)=indx(j)
            i=j
            j=j+j
          else
            j=ir+1
          endif
        go to 20
        endif
        indx(i)=indxt
      go to 10
      end

      subroutine in_out_boundaries(xin,yin,nii,nji,
     $     xout,yout,nio,njo,cyclicxi,
     $     niip1,njip1,niop1,njop1,
     $     iunit_diag,
     $     gxout,gyout)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

C         
C  Purpose:
C
C         calculate the location of grid box boundaries of
C         an "output" grid w.r.t. an "input" grid
C         
C         used in a 2-D regriding process, i.e.,
C         input --> output
C         
C  Input variables:
C         
C         xin - longitudes of the input grid
C         yin - latitudes of the input grid
C         xout - longtitudes of the output grid
C         yout - latitudes of the output grid
C         
C         cyclicxi - flag whether the input grid is cyclically continuous
C         in x
C         
C         nii - size of the x dimension of the input grid
C         nji - size of the y dimension of the input grid
C         nio - size of the x dimension of the output grid
C         njo - size of the y dimension of the output grid
C
C         niip1= nii+1, etc.
C
C         iunit_diag - unit number to write diagnositics
C
C  Output variables:         
C
C         gxout - x location of the output grid in input grid units
C         gyout - y location of the output grid in input grid units
C         

      dimension xin(*),yin(*),xout(*),yout(*),
     $     gxout(*),gyout(*)

      logical cyclicxi
      logical verb

      verb=.false.

C         
C         locate the output grid w.r.t. input grid in x
C         ALLOW FOR CYCLIC CONTINUITY IN X!!
C         
      ibeg=1
      iend=niip1
      
      do i=1,niop1
        
        x0=xout(i)
        
        do ii=ibeg,iend
C         
C         point is before start of input grid 
C         
          x1=xin(ii)
          x2=xin(niop1)

          if(x0.lt.x1) then
            if(cyclicxi) then
              ic=nii
              do while(x0.lt.x1)
                x2=x1
                x1=xin(ic)-360.0
                ic=ic-1
              enddo
              dx0=(x0-x1)/(x2-x1)
              gxout(i)=float(ic)+dx0
            else
              gxout(i)=0.0
            endif
            
            ibeg=1
            go to 50
            
C         
C         point is within the input grid
C         
          else if(ii.ge.1.and.ii.le.nii) then
            
            x2=xin(ii+1)
            
cccc            write(iunit_diag,*) ibeg,i,ii,x0,x1,x2
            
            if(x0.ge.x1.and.x0.lt.x2) then
              dx0=(x0-x1)/(x2-x1)
              gxout(i)=float(ii-1)+dx0
              ibeg=ii
              go to 50
            endif
           
C         
C         point is beyond input grid
C         
          else if(x0.ge.x2) then
            ic=2
            if(cyclicxi) then
              x1=xin(niip1)
              x2=xin(ic)+360.0

cccc              write(iunit_diag,*) 'after',i,ic,x0,x1,x2
              do while(x0.lt.x1.or.x0.ge.x2)
                ic=ic+1
                x1=x2
                x2=xin(ic)+360.0
              end do
              dx0=(x0-x1)/(x2-x1)
              gxout(i)=float(ic-2)+dx0
            else
              gxout(i)=float(nii)
            endif
            
            ibeg=niip1
            go to 50
            
          endif
          
        end do
        
 50     continue
        
      end do
C         
C         locate the output grid w.r.t. input grid in y
C         NO CYCLIC CONTINUITY!!
C         
      jbeg=1
      jend=njip1
      
      do j=1,njop1
        
        y0=yout(j)
        
        do jj=jbeg,jend
C         
C         point is before start of input grid 
C         
          y1=yin(jj)
          
          if(y0.lt.y1) then

            gyout(j)=0.0
            jbeg=1
            if(verb) write(iunit_diag,*)
     $           'before y ',j,jj,y0,y1,gyout(j)
            go to 60
C         
C         point is within the input grid
C         
          else if(jj.ge.1.and.jj.le.nji) then
            
            y2=yin(jj+1)
            
            if(y0.ge.y1.and.y0.lt.y2) then
              dy0=(y0-y1)/(y2-y1)
              gyout(j)=float(jj-1)+dy0
              jbeg=jj
              if(verb) write(iunit_diag,*)
     $             'jbeg... ',jbeg,j,jj,y1,y0,y2,gyout(j)
              go to 60
            endif
C         
C         point is beyond input grid
C         
          else if(y0.ge.y1) then
            gyout(j)=float(nji)
            jbeg=njip1
cccc            write(iunit_diag,*) 'after y ',j,jj,gyout(j)
            go to 60
          endif
          
        end do
        
 60     continue
        
      end do

      return
      end

      subroutine avg_pole(a,m,n)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

C         
Csss      routine to replace pole value with average 
Csss      at the penultimate point
C
      dimension a(m,n)
      ave_2=0.0
      ave_nm1=0.0
      do i=1,m
        ave_2=ave_2+a(i,2)
        ave_nm1=ave_nm1+a(i,n-1)
      end do
      ave_2=ave_2/m
      ave_nm1=ave_nm1/m
      do i=1,m
        a(i,1)=ave_2
        a(i,n)=ave_nm1
      end do
      return
      end

      subroutine bsslz1(bes,n)                                      

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension bes(n)                                              
      dimension bz(50)                                              
c                                                                   
      data pi/3.14159265358979d0/                                   
      data bz  / 2.4048255577d0, 5.5200781103d0,
     $  8.6537279129d0,11.7915344391d0,14.9309177086d0,18.0710639679d0, 
     $ 21.2116366299d0,24.3524715308d0,27.4934791320d0,30.6346064684d0,
     $ 33.7758202136d0,36.9170983537d0,40.0584257646d0,43.1997917132d0,
     $ 46.3411883717d0,49.4826098974d0,52.6240518411d0,55.7655107550d0,
     $ 58.9069839261d0,62.0484691902d0,65.1899648002d0,68.3314693299d0,
     $ 71.4729816036d0,74.6145006437d0,77.7560256304d0,80.8975558711d0,
     $ 84.0390907769d0,87.1806298436d0,90.3221726372d0,93.4637187819d0,
     $ 96.6052679510d0,99.7468198587d0,102.888374254d0,106.029930916d0,
     $ 109.171489649d0,112.313050280d0,115.454612653d0,118.596176630d0,
     $ 121.737742088d0,124.879308913d0,128.020877005d0,131.162446275d0,
     $ 134.304016638d0,137.445588020d0,140.587160352d0,143.728733573d0,
     $ 146.870307625d0,150.011882457d0,153.153458019d0,156.295034268d0/

      nn=n                                                          
      if(n.le.50) go to 12                                          
      bes(50)=bz(50)                                                
      do 5 j=51,n                                                   
    5 bes(j)=bes(j-1)+pi                                            
      nn=49                                                         
   12 do 15 j=1,nn                                                  
   15 bes(j)=bz(j)                                                  
      return                                                        
      end                                                           

      function esatw(t)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c     t is temperature of air in deg celcius.
c     ta is temperature in deg kelvin
c     p is pressure in mb
c     esatw is saturation vapor pressure in mb (over water)
c
      data ps/1013.246/,ts/373.16/
c
      ta = t
      if(t .lt. 100.0) then 
         ta = t + 273.16
      end if
      e1=11.344*(1.0-ta/ts)
      e2=-3.49149*(ts/ta-1.0)
      f1=-7.90298*(ts/ta-1.0)
      f2=5.02808*log10(ts/ta)
      f3=-1.3816*(10.0**e1-1.0)*1.e-7
      f4=8.1328*(10.0**e2-1.0)*1.e-3
      f5=log10(ps)
      f=f1+f2+f3+f4+f5
      esatw=10.0**f
      return
      end
c
      function esati(t)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c     t is temperature of air in deg celcius.
c     ta is temperature in deg kelvin
c     p is pressure in mb
c     esati is saturation vapor pressure with respect to ice in mb
c
      data p0/6.1071/,t0/273.16/
c
      t = ta
      if(t .lt. 100.0) then 
        ta = t + 273.16
      end if
      f1=-9.09718*(t0/ta-1.0)
      f2=-3.56654*log10(t0/ta)
      f3=0.876793*(1.0-ta/t0)
      f4=log10(p0)
      esati = 10.0**(f1+f2+f3+f4)
      return
      end


      subroutine fix_grid(a,ni,nj)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension a(ni,nj)
      do i=1,ni
        do j=1,nj
          a(i,j)=i+j
        end do
      end do
      return
      end


      subroutine fix_poles(fldout,nio,njo,undef,
     $     spole_point,npole_point)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension fldout(nio,njo)
      logical spole_point,npole_point

      rmeans=0.0
      rmeann=0.0
      icnts=0
      icntn=0

      do i=1,nio

        if(fldout(i,1).ne.undef) then
          rmeans=rmeans+fldout(i,1)
          icnts=icnts+1
        endif

        if(fldout(i,njo).ne.undef) then
          rmeann=rmeann+fldout(i,njo)
          icntn=icntn+1
        endif

      end do

      if(icnts.gt.0) then
        rmeans=rmeans/float(icnts)
      else
        rmeans=undef
      endif

      if(icntn.gt.0) then
        rmeann=rmeann/float(icntn)
      else
        rmeann=undef
      endif

      if(spole_point) then
        do i=1,nio
          fldout(i,1)=rmeans
        end do
      end if

      if(npole_point) then
        do i=1,nio
          fldout(i,njo)=rmeann
        end do
      end if

      return
      end

c
c$$$  subprogram documentation block
c                .      .    .                                       .
c subprogram:    gaulat      calculates gaussian grid latitudes
c   prgmmr: s. j. lord       org: w/nmc22    date: 91-06-06
c
c abstract: calculates gaussian grid latitudes
c
c program history log:
c   91-06-06  s. j. lord - copied from kanamitsu library
c   930921 m.fiorino - changed from colatitude to latitude
c   yy-mm-dd  modifier1   description of change
c   yy-mm-dd  modifier2   description of change
c
c usage:    call pgm-name(inarg1, inarg2, wrkarg, outarg1, ... )
c   input argument list:
c     inarg1   - generic description, including content, units,
c     inarg2   - type.  explain function if control variable.
c
c   output argument list:      (including work arrays)
c     wrkarg   - generic description, etc., as above.
c     outarg1  - explain completely if error return
c     errflag  - even if many lines are needed
c
c   input files:   (delete if no input files in subprogram)
c     ddname1  - generic name & content
c
c   output files:  (delete if no output files in subprogram)
c     ddname2  - generic name & content as above
c     ft06f001 - include if any printout
c
c remarks: list caveats, other helpful hints or information
c
c attributes:
c   language: indicate extensions, compiler options
c   machine:  nas, cyber, whatever
c
c$$$
      subroutine gauss_lat_nmc(gaul,k)                                     

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      parameter (ngaus=1000)
      dimension a(ngaus)                                              
      dimension gaul(k)                                                
c
      save
c                                                                   
      esp=1.d-14                                                    
      c=(1.d0-(2.d0/3.14159265358979d0)**2)*0.25d0                  
      fk=k                                                          
      kk=k/2                                                        
      call bsslz1(a,kk)                                             
      do 30 is=1,kk                                                 
      xz=cos(a(is)/sqrt((fk+0.5d0)**2+c))                           
      iter=0                                                        
   10 pkm2=1.d0                                                     
      pkm1=xz                                                       
      iter=iter+1                                                   
      if(iter.gt.10) go to 70                                       
      do 20 n=2,k                                                   
      fn=n                                                          
      pk=((2.d0*fn-1.d0)*xz*pkm1-(fn-1.d0)*pkm2)/fn                 
      pkm2=pkm1                                                     
   20 pkm1=pk                                                       
      pkm1=pkm2                                                     
      pkmrk=(fk*(pkm1-xz*pk))/(1.d0-xz**2)                          
      sp=pk/pkmrk                                                   
      xz=xz-sp                                                      
      avsp=abs(sp)                                                  
      if(avsp.gt.esp) go to 10                                      
      a(is)=xz                                                      
   30 continue                                                      
      if(k.eq.kk*2) go to 50                                        
      a(kk+1)=0.d0                                                  
      pk=2.d0/fk**2                                                 
      do 40 n=2,k,2                                                 
      fn=n                                                          
   40 pk=pk*fn**2/(fn-1.d0)**2                                      
   50 continue                                                      
      do 60 n=1,kk                                                  
      l=k+1-n                                                       
      a(l)=-a(n)                                                    
   60 continue                                                      
c                                                                   
      radi=180./(4.*atan(1.))                                       
      do 211 n=1,k                                                  
      gaul(n)=acos(a(n))*radi-90.0                                       
  211 continue                                                      
c     print *,'gaussian lat (deg) for jmax=',k                      
c     print *,(gaul(n),n=1,k)                                       
c                                                                   
      return                                                        
   70 write(6,6000)                                                 
 6000 format(//5x,14herror in gauaw//)
      stop
      end        
                                                   
      subroutine gauss_lat_pcmdi(gaul,gaulb,k)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)
      
      parameter (ngaus=1000)
      real(kind=8) pa(ngaus),pw(ngaus),pb(ngaus),pt(ngaus)
      dimension gaul(k),gaulb(k)

      save  

C         
C         get the gaussian latitudes and integration weights
C
CCC      call gauaw(pa,pw,k,nmax)
      call gauaw(pa,pw,k)
C         
C         reverse direction so j increase northward for output
C         
      do j=1,k
        jj=k-j+1
        pt(j)=pa(jj)
      end do
      do j=1,k
        pa(j)=pt(j)
      end do

      do j=1,k
        jj=k-j+1
        pt(j)=pw(jj)
      end do
      do j=1,k
        pw(j)=pt(j)
      end do

      pi = 4.*datan(1.d0)
      r2d = 180.0/pi
C         
C         integrate to get the latitude boundaries of the gauss grid boxes
C
      pb(1)=-pi*0.5
      do j=1,k
        pb(j+1) = dasin( pw(j) + dsin( pb(j) ) )
      end do

      do j=1,k+1
        gaulb(j)=r2d*pb(j)
      end do

      do j=1,k
        gaul(j)=r2d*dasin(pa(j))
      end do

      return
      end

      subroutine gauaw(pa,pw,k)

      USE gex
      implicit integer(i-n)
      implicit real(kind=8)(a-h,o-z)

c         
c****     *gauaw* - compute abscissas and weights for *gaussian integration.
c         
c         purpose.
c         --------
c         
c         *gauaw* is called to compute the abscissas and weights requir
c         to perform *gaussian integration.
c         
c**       interface.
c         ----------
c         
c         *call* *gauaw(pa,pw,k)*
c         
c         *pa*     - array, length at least *k,* to receive abscis
c         abscissas.
c         *pw*     - array, length at least *k,* to receive
c         weights.
c         
c         method.
c         -------
c         
c         the zeros of the *bessel functions are used as starting
c         approximations for the abscissas. newton iteration is used to
c         improve the values to within a tollerence of *eps.*
c         
c         external.
c         ---------
c         
c         *bsslzr* - routine to obtain zeros of *bessel functions.
c         
c         reference.
c         ----------
c         

      dimension pa(k),pw(k)
      data eps/1.d-13/
c         
c         ------------------------------------------------------------------
c         
c*        1.     set constants and find zeros of bessel function.
c         --- --------- --- ---- ----- -- ------ ---------
c         
      pi = 4.*datan(1.d0)
 100  continue
      c=(1.-(2./pi)**2)*0.25
      fk=k
      kk=k/2
      call bsslzr(pa,kk)
c         
      do 290 is=1,kk
        xz=dcos(pa(is)/dsqrt((fk+0.5)**2+c))
c*        giving the first approximation for *xz.*
        iter=0
c         
c         ------------------------------------------------------------------
c         
c*        2.     compute abscissas and weights.
c         ------- --------- --- -------
c         
 200    continue
c         
c*        2.1     set values for next iteration.
 210    continue
        pkm2=1.
        pkm1=xz
        iter=iter+1
        if(iter.gt.10) go to 300
c         
c*        2.2     computation of the *legendre polynomial.
 220    continue
c         
        do 222 n=2,k
          fn=n
          pk=((2.*fn-1.)*xz*pkm1-(fn-1.)*pkm2)/fn
          pkm2=pkm1
          pkm1=pk
 222    continue
c         
        pkm1=pkm2
        pkmrk=(fk*(pkm1-xz*pk))/(1.-xz**2)
        sp=pk/pkmrk
        xz=xz-sp
        avsp=dabs(sp)
        if(avsp.gt.eps) go to 210
c         
c*        2.3     abscissas and weights.
 230    continue
        pa(is)=xz
        pw(is)=(2.*(1.-xz**2))/(fk*pkm1)**2
c         
c*        2.4     odd *k* computation of weight at the equator.
 240    continue
        if (k.ne.kk*2) then
          pa(kk+1)=0.
          pk=2./fk**2
c         
          do 242 n=2,k,2
            fn=n
            pk=pk*fn**2/(fn-1.)**2
 242      continue
c         
          pw(kk+1)=pk
        else
c         
c*        2.5     use symmetry to obtain remaining values.
c         
 250      continue
c         
          do 252 n=1,kk
            l=k+1-n
            pa(l)=-pa(n)
            pw(l)=pw(n)
 252      continue
c         
        endif
 290  continue
c         write(6,*) iter
c         
      return
c         
c         ------------------------------------------------------------------
c         
c*        3.     error processing.
c         ----- -----------
c         
 300  continue
      write (nout,9901)
 9901 format(//,'  gauaw failed to converge after 10 iterations.')
      stop
c         
c         ------------------------------------------------------------------
c         
      end

      subroutine bsslzr(pbes,knum)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c         
c****     *bsslzr* - routine to return zeros of the j0 *bessel function.
c         
c         purpose.
c         --------
c         
c         *bsslzr* returns *knum* zeros, or if *knum>50,* *knum*
c         approximate zeros of the *bessel function j0.
c         
c**       interface.
c         ----------
c         
c         *call* *nsslzr(pbes,knum)*
c         
c         *pbes*   - array, dimensioned *knum,* to receive the
c         values.
c         *knum*   - number of zeros requested.
c         
c         method.
c         -------
c         
c         the first 50 values are obtained from a look up table. any
c         additional values requested are interpolated.
c         
c         externals.
c         ----------
c         
c         none.
c         
c         reference.
c         ----------
c         
*call     comcon
      real(kind=iGaKind) pbes(knum), zbes(50), api
      data zbes        / 2.4048255577,   5.5200781103,
     x 8.6537279129,  11.7915344391,  14.9309177086,  18.0710639679,
     x 21.2116366299,  24.3524715308,  27.4934791320,  30.6346064684,
     x 33.7758202136,  36.9170983537,  40.0584257646,  43.1997917132,
     x 46.3411883717,  49.4826098974,  52.6240518411,  55.7655107550,
     x 58.9069839261,  62.0484691902,  65.1899648002,  68.3314693299,
     x 71.4729816036,  74.6145006437,  77.7560256304,  80.8975558711,
     x 84.0390907769,  87.1806298436,  90.3221726372,  93.4637187819,
     x 96.6052679510,  99.7468198587, 102.8883742542, 106.0299309165,
     x 109.1714896498, 112.3130502805, 115.4546126537, 118.5961766309,
     x 121.7377420880, 124.8793089132, 128.0208770059, 131.1624462752,
     x 134.3040166383, 137.4455880203, 140.5871603528, 143.7287335737,
     x 146.8703076258, 150.0118824570, 153.1534580192, 156.2950342685/
c         
c         ------------------------------------------------------------------
c         
c*        1.     extract values from look up table.
c         ------- ------ ---- ---- -- ------
c         
c         set api
c         
      api=4.*datan(1.d0)
 100  continue
      inum=min0(knum,50)
c         
      do 110 j=1,inum
        pbes(j)=zbes(j)
 110  continue
c         
c         ------------------------------------------------------------------
c         
c*        2.     interpolate remaining values.
c         ----------- --------- -------
c         
 200  continue
c         
      zpi=api
      do 210 j=51,knum
        pbes(j)=pbes(j-1)+api
 210  continue
c         
c         ------------------------------------------------------------------
c         
      return
      end

      subroutine qprntu(a,qtitle,undef,
     $     ibeg,jbeg,m,n,iskip,iunit)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

C        
C----------------------------------------------------------------
C         
C         version of qprntn which handles undefined values
C         by having them printed as ****
C         
C         Mike Fiorino, NMC Development Division
C        
C----------------------------------------------------------------
C         
C
C         a= fwa of m x n array
C         qtitle - title
C         ibeg,jbeg=lower left corner coords to be printed
C         up to 43 x 83 points printed
c         

      dimension a(m,n),ix(81)
      character qtitle*24

      half=0.5
c
c  determine grid limits
c
      if(iskip.eq.0) iskip=1
      iend=min0(ibeg+79*iskip,m)
      jend=min0(jbeg+79*iskip,n)
c
   24 continue
c
c  index backwards checking for max
c
   11 xm=0.
      jendsc=min0(jend,n)
      do j=jbeg,jendsc,iskip
        jend_qp = j
        do i=ibeg,iend,iskip
          xmax=abs(a(i,j))
          if(a(i,j).eq.undef) xmax=0.0
          xm=max(xm,xmax)
        end do
      end do
c
c  determine scaling factor limits
c
      if(xm.lt.1.0e-32.or.xm.eq.0.0) xm=99.0
      xm=log10(99.0/xm)
      kp=xm
      if(xm.lt.0.0)kp=kp-1
c
c  print scaling constants
c
   12 write(iunit,1) qtitle,kp,iskip,(i,i=ibeg,iend,2*iskip)

    1 format('0',a,'   k=',i3,' iskip=',i2,/,' ',41i6) 
      fk=10.0**kp
c
c  quickprint field
c
      do 2 jli=jend_qp,jbeg,-iskip
        ii= 0
        if(kp.eq.0) then 
          do i=ibeg,iend,iskip
            ii=ii+1

            if(a(i,jli).eq.undef) then
              ix(ii)=999999
            else
              ix(ii)=a(i,jli)+sign(half,a(i,jli))
            endif

          end do
        else
          do i=ibeg,iend,iskip
            ii=ii+1
            if(a(i,jli).eq.undef) then
              ix(ii)=999999
            else
              ix(ii)=a(i,jli)*fk+sign(half,a(i,jli))
            endif
          end do
        end if
        write(iunit,'(i4,81i3)') jli,(ix(i),i=1,ii),jli
2     continue
      return
      end

      subroutine qprntn(a,qtitle,ibeg,jbeg,m,n,iskip,iunit)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c**********	12 APR 91 this version outputs to iunit 
c**********	using write on the Cray Y/MP 
c
c***************************************************************
c***************************************************************
c*****                                                     *****
c*****       qprint output routine (corrected 4/26/86)     *****
c*****                                                     *****
c***************************************************************
c***************************************************************
c
c a= fwa of m x n array
c qtitle - title
c ibeg,jbeg=lower left corner coords to be printed
c up to 43 x 83 points printed
c
      real(kind=iGaKind) a(m,n),ix(81)
      real(kind=4) xm
      character qtitle*24
c
c  determine grid limits
c
      if(iskip.eq.0) iskip=1
      iend=min0(ibeg+79*iskip,m)
      jend=min0(jbeg+79*iskip,n)

      half=0.5
c
   24 continue
c
c  index backwards checking for max
c
   11 xm=0.
      jendsc=min0(jend,n)
      do j=jbeg,jendsc,iskip
      jend_qp = j
      do i=ibeg,iend,iskip
        xm=max(xm*1.d0,abs(a(i,j)))
      end do
      end do
c
c  determine scaling factor limits
c
      if(xm.lt.1.0e-32.or.xm.eq.0.0) xm=99.0
      xm=alog10(99.0/xm)
      kp=xm
      if(xm.lt.0.0)kp=kp-1
c
c  print scaling constants
c
   12 write(iunit,1) qtitle,kp,iskip,(i,i=ibeg,iend,2*iskip)

    1 format('0',a,'   k=',i3,' iskip=',i2,/,' ',41i6) 
      fk=10.0**kp
c
c  quickprint field
c
      do 2 jli=jend_qp,jbeg,-iskip
        ii= 0
        if(kp.eq.0) then 
          do i=ibeg,iend,iskip
            ii=ii+1
            ix(ii)=a(i,jli)+sign(half,a(i,jli))
          end do
        else
          do i=ibeg,iend,iskip
            ii=ii+1
            ix(ii)=a(i,jli)*fk+sign(half,a(i,jli))
          end do
        end if
        write(iunit,'(i4,81i3)') jli,(ix(i),i=1,ii),jli
2     continue
      return
      end

      function qsatw(t,p)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c     t is temperature of air in deg celcius.
c     ta is temperature in deg kelvin
c     p is pressure in mb
c     qsatw is saturation specific humidity in g/g (over water)
c
      data ps/1013.246/,ts/373.16/
c
      ta = t
      if(t .lt. 100.0) then
         ta = t + 273.16
      end if
      e1=11.344*(1.0-ta/ts)
      e2=-3.49149*(ts/ta-1.0)
      f1=-7.90298*(ts/ta-1.0)
      f2=5.02808*log10(ts/ta)
      f3=-1.3816*(10.0**e1-1.0)*1.e-7
      f4=8.1328*(10.0**e2-1.0)*1.e-3
      f5=log10(ps)
      f=f1+f2+f3+f4+f5
      es=10.0**f
      qsatw=.62197*es/(p-0.378*es)
      return
      end


      function qsati(t,p)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c     t is temperature of air in deg celcius.
c     ta is temperature in deg kelvin
c     p is pressure in mb
c     qsati is saturation specific humidity with respect to ice in g/g
c
      data p0/6.1071/,t0/273.16/
c
      t = ta
      if(t .lt. 100.0) then
         ta = t + 273.16
      end if
      f1=-9.09718*(t0/ta-1.0)
      f2=-3.56654*log10(t0/ta)
      f3=0.876793*(1.0-ta/t0)
      f4=log10(p0)
      es=10.0**(f1+f2+f3+f4)
      qsati=.62197*es/(p-0.378*es)
      return
      end

      subroutine read_grid(iunit,a,ni,nj,istat)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension a(ni,nj)
      istat=1
      read(iunit,err=800) a
      go to 999
 800  continue
      istat=0
 999  continue
      return
      end

      function satvp(temp)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      real(kind=iGaKind) a(7)
      data a/6.10779961,0.4436518521,0.01428945805,2.650648471e-4,
     &       3.031240396e-6,2.034080948e-8,6.136820929e-11/
      data e0, aa, bb/6.1078, 17.2693882, 237.3/
      data t0/273.16/
c
      t = temp
c convert temperature to degrees c, if necessary.
      if(temp .gt. 100.0) t = temp - t0
c polynomial is only good from -50 to + 50 c.
c if outside this range teton's formmula is used.
      if(t .gt. 50.0 .or. t .lt. -50.0) then
        satvp = e0*exp(aa*t/(t+bb))
      else
      satvp = a(1)+t*(a(2)+t*(a(3)+t*(a(4)+t*(a(5)+t*(a(6)+a(7)*t)))))
      end if
      return
      end
 
      subroutine sfc_area(fld,rlon,rlat,undef,ni,nj,
     $     area,iunit_diag)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension fld(ni,nj),rlat(nj+1),rlon(ni+1),
     $     area(ni,nj)

      deg2rad=3.14115926/180.0

      do i=1,ni
        do j=1,nj

          rlat1=rlat(j+1)
          rlat0=rlat(j)

          if(rlat1.gt.90.0) rlat1=90.0
          if(rlat0.gt.90.0) rlat0=90.0
          if(rlat1.lt.-90.0) rlat1=-90.0
          if(rlat0.lt.-90.0) rlat0=-90.0

          dlon=(rlon(i+1)-rlon(i))*deg2rad
          dlat=sin(rlat1*deg2rad)-sin(rlat0*deg2rad)
          area(i,j)=dlon*dlat
          if(fld(i,j).eq.undef) area(i,j)=0.0

        end do

cccc        rlatout=(rlat(j+1)+rlat(j))*0.5
cccc        write(iunit_diag,*) 'j = ',j,' sfc area = ',rlatout,area(1,j)
      end do

      return
      end

      subroutine trim_grid(a,ni,niinew,nj,dum)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension a(ni,nj),dum(niinew,nj)
      do j=1,nj
        do i=1,niinew
          dum(i,j)=a(i,j)
        end do
      end do
      return
      end

      subroutine vload2(a,b,ni,nj)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension a(ni,nj),b(ni,nj)
      do j=1,nj
        do i=1,ni
          a(i,j)=b(i,j)
        end do
      end do
      return
      end

      subroutine ul_case(cc,ilen)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      character*1 cc(ilen)
      do i=1,ilen
        if(ichar(cc(i)).ge.65.and.ichar(cc(i)).le.90)
     $       cc(i)=char(ichar(cc(i))+32)
      end do
      return
      end
      
      subroutine write_grid(iunit,a,ni,nj)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      dimension a(ni,nj)
      write(iunit) a
      return
      end

      subroutine rcalhdst (slat,slon,elat,elon,head,dist)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

C..........................START PROLOGUE..............................
C
C  SCCS IDENTIFICATION:  @(#)rcalhdst.f	1.1 12/15/94
C                        22:44:12 @(#)
C
C  CONFIGURATION IDENTIFICATION:
C
C  MODULE NAME:  rcalhdst
C
C  DESCRIPTION:  use rhumb line to calculate heading and distance from
C                slat,slon to elat,elon
C
C  COPYRIGHT:                  (C) 1994 FLENUMOCEANCEN
C                              U.S. GOVERNMENT DOMAIN
C                              ALL RIGHTS RESERVED
C
C  CONTRACT NUMBER AND TITLE:  GS-09K-90-BHD0001
C                              ADP SUPPORT FOR HIGHLY TECHNICAL SOFTWARE
C                              DEVELOPMENT FOR SCIENTIFIC APPLICATIONS
C
C  REFERENCES:  none
C
C  CLASSIFICATION:  unclassified
C
C  RESTRICTIONS:  none
C
C  COMPUTER/OPERATING SYSTEM
C               DEPENDENCIES:  Cray UNICOS
C
C  LIBRARIES OF RESIDENCE:
C
C  USAGE:  call rcalhdst (slat,slon,elat,elon,head,dist)
C
C  PARAMETERS:
C     NAME         TYPE        USAGE             DESCRIPTION
C   --------      -------      ------   ------------------------------
C      slat         real         in     initial latitude
C      slon         real         in     initial longitude
C      elat         real         in     final latitude
C      elon         real         in     final longitude
C      head         real         out    heading (deg)
C      dist         real         out    distance (nm)
C
C  COMMON BLOCKS:  none
C
C  FILES:  none
C
C  DATA BASES:  none
C
C  NON-FILE INPUT/OUTPUT:  none
C
C  ERROR CONDITIONS:  none
C
C  ADDITIONAL COMMENTS:
C
C...................MAINTENANCE SECTION................................
C
C  MODULES CALLED:  none
C
C  LOCAL VARIABLES:
C          NAME      TYPE                 DESCRIPTION
C         ------     ----       ----------------------------------
C           a45r     real       radians per 45 degrees
C           eln1     real       intermediate calculation factor
C           eln2     real       intermediate calculation factor
C           inil      int       set-up caculation flag
C            rad     real       degrees per radian
C           radi     real       radians per degree
C           rai2     real       radians per two degrees
C           tiny     real       small real number, hardware dependent
C             xl     real       working initial latitude
C             xn     real       working initial longitude
C             xr     real       intermediate calculation factor
C             yl     real       working final latitude
C             yn     real       working final longitude
C             yr     real       intermediate calculation factor
C
C  METHOD:  standard calculations, with near pole point corrections
C           omitted - no tropical cyclones near the poles
C
C  INCLUDE FILES:  none
C
C  COMPILER DEPENDENCIES:  Fortran 77
C
C  COMPILE OPTIONS:
C
C  MAKEFILE:
C
C  RECORD OF CHANGES:
C
C
C...................END PROLOGUE.......................................
C
c
c         formal parameters
      real(kind=iGaKind) slat, slon, elat, elon, head, dist
c
c         local variables
      integer inil
c
      real(kind=iGaKind) rad, radi, rdi2, a45r, tiny, xl, xn, yl, yn
      real(kind=iGaKind) eln1, eln2, xr, yr
c
      save rad,radi,rdi2,a45r
c
      data tiny/0.1e-6/
c                   maximum poleward latitude, hardware dependent
ccc   data plmx/89.99/
      data inil/-1/
c . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
c
      if (inil .ne. 0) then
         inil = 0
         rad  = 180.0/acos (-1.0)
         radi = 1/rad
         rdi2 = 0.5*radi
         a45r = 45.0*radi
      endif
c                    same point returns 0.0 head and 0.0 dist
      head = 0.0
      dist = 0.0
c                   southern hemisphere latitude is negative
      xl = slat
      yl = elat
c                   longitude is 0 -360 in degrees East or
c                             negative for West longitude
      xn = slon
      yn = elon
      if (xl.ne.yl .or. xn.ne.yn) then
c                   if longitude is west, convert to 0-360 east
        if (xn .lt. 0.0) xn = xn +360.0
c                   if longitude is west, convert to 0-360 east
        if (yn .lt. 0.0) yn = yn +360.0
c                    check for shortest angular distance
        if (xn.gt.270.0 .and. yn.lt.90.0) then
          yn = yn +360.0
        elseif (yn.gt.270.0 .and. xn.lt.90.0) then
          xn = xn +360.0
        endif
        if (abs (xl -yl) .gt. tiny) then
c                   calculate initial distance
          dist = 60.0*(xl -yl)
          if (abs (xn -yn) .gt. tiny) then
c                   check for positions poleward of 89+ degrees latitude
ccc         if (abs (xl).gt.plmx .or. abs (yl).gt.plmx) then
c              (hardware dependent - not required for tropical cyclones)
ccc           xlt = xl
ccc           if (abs (xlt) .gt. plmx) xlt = sign (plmx,xl)
ccc           ylt = yl
ccc           if (abs (ylt) .gt. plmx) ylt = sign (plmx,yl)
ccc           xr   = tan (xlt*rdi2 +sign (a45r,xl))
ccc           yr   = tan (ylt*rdi2 +sign (a45r,yl))
ccc         else
              xr   = tan (xl*rdi2 +sign (a45r,xl))
              yr   = tan (yl*rdi2 +sign (a45r,yl))
ccc         endif
            eln1 = sign (log (abs (xr)),xr)
            eln2 = sign (log (abs (yr)),yr)
            head = rad*(atan ((xn -yn)/(rad*(eln1 -eln2))))
            if (yl   .lt. xl)  head = head +180.0
            if (head .le. 0.0) head = head +360.0
c                   correct initial distance, based only on latitiude
            dist = dist/cos (head*radi)
          else
c                  resolve 0 or 180 heading, note head is preset to zero
            if (yl .lt. xl) head = 180.0
          endif
        else
c                    resolve 90 or 270 heading
          head = 90.0
          if (yn .lt. xn) head = 270.0
          dist = 60.0*(yn -xn)*cos (xl*radi)
        endif
        dist = abs (dist)
      endif
      return
c
      end

      subroutine rcaltln (slat,slon,head,dist,elat,elon)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

C
C..........................START PROLOGUE..............................
C
C  SCCS IDENTIFICATION:  @(#)rcaltln.f	1.1 12/15/94
C                        22:44:15 @(#)
C
C  CONFIGURATION IDENTIFICATION:
C
C
C  MODULE NAME:  rcalltln
C
C  DESCRIPTION:  use rhumb line to calculate ending lat,lon given
C                starting lat,lon, heading and distance
C
C  COPYRIGHT:                  (C) 1994 FLENUMOCEANCEN
C                              U.S. GOVERNMENT DOMAIN
C                              ALL RIGHTS RESERVED
C
C  CONTRACT NUMBER AND TITLE:  GS-09K-90-BHD0001
C                              ADP SUPPORT FOR HIGHLY TECHNICAL SOFTWARE
C                              DEVELOPMENT FOR SCIENTIFIC APPLICATIONS
C
C  REFERENCES:  none
C
C  CLASSIFICATION:  unclassified
C
C  RESTRICTIONS:  none
C
C  COMPUTER/OPERATING SYSTEM
C               DEPENDENCIES:  Cray UNICOS
C
C  LIBRARIES OF RESIDENCE:
C
C  USAGE:  call rcaltln (slat,slon,head,dist,elat,elon)
C
C  PARAMETERS:
C     NAME         TYPE        USAGE             DESCRIPTION
C   --------      -------      ------   ------------------------------
C      slat         real         in     starting latitude
C      slon         real         in     starting longitude
C      head         real         in     heading (deg)
C      dist         real         in     distance (nm)
C      elat         real         out    ending latitude
C      elon         real         out    ending longitude
C
C  COMMON BLOCKS:  none
C
C  FILES:  none
C
C  DATA BASES:  none
C
C  NON-FILE INPUT/OUTPUT:  none
C
C  ERROR CONDITIONS:  none
C
C  ADDITIONAL COMMENTS:
C
C...................MAINTENANCE SECTION................................
C
C  MODULES CALLED:  none
C
C  LOCAL VARIABLES:
C          NAME      TYPE                DESCRIPTION
C         ------     ----      ----------------------------------
C           crhd     real      heading converted to radians
C         degrad     real      conversion factor, deg to radians
C           dlat     real      distance in terms of latitude
C           dlon     real      distance in terms of longitude
C         hdgrad     real      half of degrad
C           icrs      int      integer value of heading
C           inil      int      initialization flag
C         raddeg     real      conversion factor, radian to degrees
C         rad045     real      radians in 45 degrees
C           rdst     real      absolute distance, nm
C           rhed     real      local copy of heading
C           tiny     real      small number
C
C  METHOD:
C
C  INCLUDE FILES:  none
C
C  COMPILER DEPENDENCIES:  Fortran 77
C
C  COMPILE OPTIONS:
C
C  MAKEFILE:
C
C  RECORD OF CHANGES:
C
C
C...................END PROLOGUE.......................................
C
c         formal parameters
      real(kind=iGaKind) slat, slon, head, dist, elat, elon
c
c         local variables
      integer icrs, inil
      real(kind=iGaKind) crhd, degrad, dlat, dlon, hdgrad, raddeg, rad045, rdst, rhed
      real(kind=iGaKind) tiny
c
      save raddeg, degrad, hdgrad, rad045, inil
      data inil/0/
      data tiny/1.0e-3/
c . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
c
      if (inil .eq. 0) then
        inil   = -1
        degrad = acos (-1.0)/180.0
        hdgrad = 0.5*degrad
        rad045 = 45.0*degrad
        raddeg = 1.0/degrad
      endif
c
      rdst = abs (dist)
      if (rdst .gt. tiny) then
        rhed = head
        if (rhed .lt. 0.0) then
          rhed = rhed +360.0
        elseif (rhed .gt. 360.0) then
          rhed = rhed -360.0
        endif
        if (abs (rhed -360.0) .le. tiny) rhed = 0.0
        icrs = nint (rhed)
        if (abs (head -270.0) .le. tiny .or.
     &      abs (head  -90.0) .le. tiny) then
          dlon = rdst/(60.0*cos (slat*degrad))
c                 longitude is in degrees east, 0.0 to 360.0
          if (icrs .eq. 270) then
            elon = slon -dlon
          else
            elon = slon +dlon
          endif
          elat = slat
        elseif (abs (rhed -360.0) .le. tiny .or.
     &          abs (head -180.0) .le. tiny) then
          dlat = rdst/60.0
          if (icrs .eq. 360) then
            elat = slat +dlat
          else
            elat = slat -dlat
          endif
          elon = slon
        else
          crhd = head*degrad
          elat = slat +(rdst*cos (crhd)/60.0)
          elon = slon +raddeg*(log (tan (rad045 +hdgrad*elat))
     .          -log (tan (rad045 +hdgrad*slat)))*tan (crhd)
        endif
      else
        elon = slon
        elat = slat
      endif

      return
c
      end



      subroutine lonlat2ij(loni,latj,lons,lats,ni,nj,undef,cxi,cyj)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      real(kind=iGaKind) loni,latj,undef,cxi,cyj
      real(kind=iGaKind) latc,latcm1,lonc,loncm1
      real(kind=iGaKind) lons(ni),lats(nj)
      integer ni,nj


      do i=1,ni
        if(lons(i).ge.loni) then
          ic=i
          icm1=i-1
          lonc=lons(ic)
          loncm1=lons(icm1)
          dj=lonc-loncm1
          dc=(lonc-loni)/dj
          dcm1=(loni-loncm1)/dj
          cxi=dc*icm1+dcm1*ic
cccc          print*, 'iiiiiiiiiiiiii found it: ',ic,icm1,lonc,loncm1,dc,dcm1,cxi
          exit

        endif

      end do

      do j=1,nj
        if(lats(j).ge.latj) then
          jc=j
          jcm1=j-1
          latc=lats(jc)
          latcm1=lats(jcm1)
          dj=latc-latcm1
          dc=(latc-latj)/dj
          dcm1=(latj-latcm1)/dj
          cyj=dc*jcm1+dcm1*jc
cccc          print*, 'jjjjjjjjjjjjjj found it: ',jc,jcm1,latc,latcm1,dc,dcm1,cyj
          exit

        endif

      end do

      return
      end


      subroutine bssl1(xi,f,m,res,undef)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c  general purpose 1-d dimensional bessel interpolation
c
c  d. henson  --  april, 1987
c
c  arguments --
c
c   xi  -- i coordinate (real) 1.le.i.le.m
c   f   -- array to be interpolated
c   n   -- number of rows in f
c   res -- returned interpolated value at f(xi,xj)
c
c   note----xi and xj are not tested for legal range
c         
      real(kind=8) :: f(m)
      real(kind=8) :: xi,r
c
      mm1 = m-1
      ii = int(xi)
      r = xi-ii

C         
C         test for undef points in outermost row
C
      if(  
     $     f(ii+2).eq.undef .or.
     $     f(ii-1).eq.undef
     $     ) go to 5

      if(ii.ge.2 .and. ii.lt.mm1) go to 10
c
      if(ii.lt.m) go to 5 
c
c   top or right edge
c
      if(ii.eq.m) then
        res = f(m)
        return
      endif

c
c   border zone -- use bilinear interpolation
c

 5    continue
CCC      print*,'1111 ',ii,f(ii),r,f(ii+1)
      res = (1.-r)*f(ii) + r*f(ii+1)
      return
C         
C 4-point          
C         

 10   continue

      r1 = r-0.5
      r2 = r*(r-1.)*0.5
      r3 = r1*r2*0.3333333333334

      u = (f(ii)+f(ii+1))*0.5
      del = f(ii+1)-f(ii)
      udel2 = (f(ii+2)-f(ii+1)+f(ii-1)-f(ii))*0.5
      del3 = f(ii+2)-f(ii+1)-2.*del+f(ii)-f(ii-1)
      res = u+r1*del+r2*udel2+r3*del3
CCC      print*,'4444 ',r1,r2,r3,u,del,udel2,del3,res

      return
      end

      subroutine bssl5(xi,xj,f,m,n,res,undef)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c  general purpose two dimensional bessel interpolation
c
c  d. henson  --  april, 1987
c
c  arguments --
c
c   xi  -- i coordinate (real) 1.le.i.le.m
c   xj  -- j coordinate (real) 1.le.j.le.n
c   f   -- array to be interpolated
c   m   -- number of columns in f
c   n   -- number of rows in f
c   res -- returned interpolated value at f(xi,xj)
c
c         note----xi and xj are not tested for legal range
c mf 20100929 -- put check here...
c         
      dimension f(m,n)
      dimension fr(4)
c         
      mm1 = m-1
      nm1 = n-1

      if( (xi.lt.1.0 .or. xi.gt.float(mm1) ) .or. (xj.lt.1.0 .or. xj.gt.float(nm1) ) ) then
        res=undef
      endif

      ii = int(xi)
      j = int(xj)
      r = xi-ii
      s = xj-j

C f(i,j+2)
c
c   r and s are the fractional parts of xi and xj respectively
c
c   test for top/right edge, border, or interior 
c
      if(ii.ge.2 .and. j.ge.2 .and. ii.lt.mm1 .and. j.lt.nm1) then

        if(  
     $       f(ii,j+2).eq.undef .or.
     $       f(ii+2,j).eq.undef .or.
     $       f(ii-1,j).eq.undef .or.
     $       f(ii,j-1).eq.undef
     $       ) then
          go to 5

        else
          go to 10
        endif

      endif

      if(ii.lt.m .and. j.lt.n) go to 5 

      print*,'bbbb ',ii,j,f(ii,j+2),f(ii,j-1),f(ii+2,j),f(ii-1,j)

C1111111111111111111111111111111111111111111111111111111111111111111111111111111
C         
C         top, bottom, right - 1st order - linear
C
C1111111111111111111111111111111111111111111111111111111111111111111111111111111

      if(ii.eq.m .and. j.eq.n) then
        res = f(m,n)

      else if(ii.eq.m) then

        if(f(ii,j).ne.undef. and. f(ii,j+1).ne.undef) then
          res = (1.-s)*f(ii,j)+s*f(ii,j+1)
        else
          res=undef
        endif

      else if(j.eq.n.or.j.eq.1) then

        if(f(ii,j).ne.undef .and. f(ii+1,j).ne.undef) then
          res = (1.-r)*f(ii,j)+r*f(ii+1,j)
        else
          res=undef
        endif

      endif

      return

C22222222222222222222222222222222222222222222222222222222222222222222222222222222
C
C         bilinear only -- 2-D 1st order
C
C22222222222222222222222222222222222222222222222222222222222222222222222222222222

 5    continue

      iok_bilinear=1
      if(f(ii,j).eq.undef.or.
     $     f(ii+1,j).eq.undef.or.
     $     f(ii,j+1).eq.undef.or.
     $     f(ii+1,j+1).eq.undef) iok_bilinear=0
      
      if(iok_bilinear.eq.1) then
CCCC      print*,'BL: ',f(ii,j),f(ii+1,j),f(ii,j+1),f(ii+1,j+1)
        res = (1.-s)*((1.-r)*f(ii,j)+r*f(ii+1,j))
     $       +s*((1.-r)*f(ii,j+1)+r*f(ii+1,j+1))
      else
        res=undef
      endif
      return

C33333333333333333333333333333333333333333333333333333333333333333333333333333333333         
C         
C         interior -- 2-D 3rd order
C
C33333333333333333333333333333333333333333333333333333333333333333333333333333333333         

 10   continue

c
c   interpolate 4 columns (i-1,i,i+1,i+2) to j+s and store in fr(1)
c   through fr(4)
c

      r1 = r-0.5
      r2 = r*(r-1.)*0.5
      r3 = r1*r2*0.3333333333334
      s1 = s-0.5
      s2 = s*(s-1.)*0.5
      s3 = s1*s2*0.3333333333334

      k = 0
      im1 = ii-1
      ip2 = ii+2

      do 20 i = im1,ip2
        k = k+1
        u = (f(i,j)+f(i,j+1))*0.5
        del = f(i,j+1)-f(i,j)
        udel2 = (f(i,j+2)-f(i,j+1)+f(i,j-1)-f(i,j))*0.5
        del3 = f(i,j+2)-f(i,j+1)-2.*del+f(i,j)-f(i,j-1)
        fr(k) = u+s1*del+s2*udel2+s3*del3
 20   continue

c         
c   interpolate the fr row to ii+r
c
      u = (fr(2)+fr(3))*0.5
      del = fr(3)-fr(2)
      udel2 = (fr(4)-fr(3)+fr(1)-fr(2))*0.5
      del3 = fr(4)-fr(3)-2.*del+fr(2)-fr(1)

      res = u+r1*del+r2*udel2+r3*del3

      return
      end


      subroutine ireverse(n,indx,ondx)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      integer indx(n),ondx(n)
      do i=1,n
        ii=n-i+1
        ondx(i)=indx(ii)
      enddo
      return
      end

      function ichar_len(c,imax)
      character*1 c(imax)
      iend=-1
      ii=1
      do while (iend.eq.-1.and.ii.le.imax)
        if(c(ii).eq.' '.or.ichar(c(ii)).eq.0) iend=ii
        ii=ii+1
      end do  
      if(ii.gt.imax) then
        ichar_len=imax
      else
        ichar_len=iend-1
      end if
      return
      end



      
c
c$$$  subprogram documentation block
c                .      .    .                                       .
c subprogram:    sindb        sine function from degrees input
c   prgmmr: s. j. lord       org: w/nmc22    date: 91-06-06
c
c abstract: sine function from degrees input.
c
c program history log:
c   91-06-06  s. j. lord
c   yy-mm-dd  modifier1   description of change
c   yy-mm-dd  modifier2   description of change
c
c usage:    call pgm-name(inarg1, inarg2, wrkarg, outarg1, ... )
c   input argument list:
c     inarg1   - generic description, including content, units,
c     inarg2   - type.  explain function if control variable.
c
c   output argument list:      (including work arrays)
c     wrkarg   - generic description, etc., as above.
c     outarg1  - explain completely if error return
c     errflag  - even if many lines are needed
c
c   input files:   (delete if no input files in subprogram)
c     ddname1  - generic name & content
c
c   output files:  (delete if no output files in subprogram)
c     ddname2  - generic name & content as above
c     ft06f001 - include if any printout
c
c remarks: list caveats, other helpful hints or information
c
c attributes:
c   language: indicate extensions, compiler options
c   machine:  nas, cyber, whatever
c
c$$$
      function sindb(arg)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)
c     degrad converts degrees to radians
      data degrad/0.017453/
      sindb=sin(arg*degrad)
      return
      end
c
c$$$  subprogram documentation block
c                .      .    .                                       .
c subprogram:    cosdb        cosine function from degrees input
c   prgmmr: s. j. lord       org: w/nmc22    date: 91-06-06
c
c abstract: returns cosine function from degrees input
c
c program history log:
c   91-06-06  s. j. lord
c   yy-mm-dd  modifier1   description of change
c   yy-mm-dd  modifier2   description of change
c
c usage:    call pgm-name(inarg1, inarg2, wrkarg, outarg1, ... )
c   input argument list:
c     inarg1   - generic description, including content, units,
c     inarg2   - type.  explain function if control variable.
c
c   output argument list:      (including work arrays)
c     wrkarg   - generic description, etc., as above.
c     outarg1  - explain completely if error return
c     errflag  - even if many lines are needed
c
c   input files:   (delete if no input files in subprogram)
c     ddname1  - generic name & content
c
c   output files:  (delete if no output files in subprogram)
c     ddname2  - generic name & content as above
c     ft06f001 - include if any printout
c
c remarks: list caveats, other helpful hints or information
c
c attributes:
c   language: indicate extensions, compiler options
c   machine:  nas, cyber, whatever
c
c$$$
      function cosdb(arg)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c     degrad converts degrees to radians
      data degrad/0.017453/
      cosdb=cos(arg*degrad)
      return
      end


c$$$      subprogram documentation block
c         .      .    .                                       .
c         subprogram:    sortrl      sorts real numbers
c         prgmmr: s. j. lord       org: w/nmc22    date: 91-06-04
c         
c         abstract: sorts real numbers.  output array is the index of
c         the input values that are sorted.
c         
c         program history log:
c         91-06-04  s. j. lord (modified from ncar code)
c         
c         usage:    call sortrl(a,la,nl)
c         input argument list:
c         a        - array of elements to be sorted.
c         nl       - number of elements to be sorted.
c         
c         output argument list:      (including work arrays)
c         la       - integer array containing the index of the sorted
c         - elements.  sorting is from small to large.  e.g.
c         - la(1) contains the index of the smallest element in
c         - array.  la(nl) contains the index of the largest.
c         
c         
c         remarks: none
c         
c         attributes:
c         language: fortran 77
c         machine:  any
c         
c$$$      
      subroutine sortrl(a,la,npoints,nl)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c         
c         entry sortrl(a,la,nl)    sort up real numbers
c         ** revised (6/13/84) for the use in vax-11
c         arguments ...
c         a    input array of nl elements to be sorted or re-ordered
c         la   output array of nl elements in which the original location
c         of the sorted elements of a are saved, or
c         input array to specify the reordering of array a by sorted
c         nl   the number of elements to be treated
c         
      save
c         
      dimension a(npoints),la(npoints),ls1(64),ls2(64)
      data nsx/64/,warned/-1./
c         
c         set the original order in la
c         
    5 continue
      do l=1,nl
        la(l)=l
      enddo
c         
c         separate negatives from positives
c         
 10   continue
      l = 0
      m = nl + 1

 12   continue
      l = l + 1
      if(l.ge.m) go to 19

      if(a(l).lt.0.0) then
        go to 12
      elseif(a(l).ge.0.0) then
        go to 15
      endif

 15   continue
      m = m - 1
      if(l.ge.m) go to 19

 16   continue
      if(a(m).lt.0.0) then
        go to 18
      else if(a(m).ge.0.0) then
        go to 15
      endif

 18   continue
      az = a(m)
      a(m) = a(l)
      a(l) = az
      lz = la(m)
      la(m) = la(l)
      la(l) = lz
      go to 12

 19   continue
      l = l - 1
      lnegx=l
c         
c         note that min and max for interval (1,nl) have not been determined
c         
      ls1(1) = 0
      l2 = nl + 1
      ns = 1
c         
c         step up
c         
 20   ls1(ns) = ls1(ns) + 1
      ls2(ns) = l
      ns = ns + 1
      if(ns.gt.nsx) go to 80
      l1 = l + 1
      ls1(ns) = l1
      l2 = l2 - 1
      go to 40
c         
c         step down
c         
 30   ns=ns-1
      if (ns.le.0) go to 90
      l1 = ls1(ns)
      l2 = ls2(ns)
 40   if(l2.le.l1) go to 30
c         
c         find max and min of the interval (l1,l2)
c         
 50   if (a(l1)-a(l2) .le. 0) go to 52
      an = a(l2)
      ln = l2
      ax = a(l1)
      lx = l1
      go to 54
 52   an = a(l1)
      ln = l1
      ax = a(l2)
      lx = l2
 54   l1a = l1 + 1
      l2a = l2 - 1
      if(l1a.gt.l2a) go to 60
c         
      do 58 l=l1a,l2a
        if (a(l)-ax .gt. 0) go to 56
        if (a(l)-an .ge. 0) go to 58
        an = a(l)
        ln = l
        go to 58
 56     ax = a(l)
        lx = l
 58   continue
c         
c         if all elements are equal (an=ax), step down
c         
 60   if (an .eq. ax)  go to 30
c         
c         place min at l1, and max at l2
c         if either ln=l2 or lx=l1, first exchange l1 and l2
c         
      if(ln.eq.l2.or.lx.eq.l1) go to 62
      go to 64
 62   az=a(l1)
      a(l1)=a(l2)
      a(l2)=az
      lz=la(l1)
      la(l1)=la(l2)
      la(l2)=lz
c         
c         min to l1, if ln is not at either end
c         
 64   if(ln.eq.l1.or.ln.eq.l2) go to 66
      a(ln)=a(l1)
      a(l1)=an
      lz=la(ln)
      la(ln)=la(l1)
      la(l1)=lz
c         
c         max to l2, if lx is not at either end
c         
 66   if(lx.eq.l2.or.lx.eq.l1) go to 68
      a(lx)=a(l2)
      a(l2)=ax
      lz=la(lx)
      la(lx)=la(l2)
      la(l2)=lz
c         
c         if only three elements in (l1,l2), step down.
c         
 68   if(l1a.ge.l2a) go to 30
c         
c         set a criterion to split the interval (l1a,l2a)
c         ac is an approximate arithmetic average of ax and an,
c         provided that ax is greater than an.  (it is the case, here)
c         ** if a is distributed exponentially, geometric mean may
c         be more efficient
c         
      ac = (ax+an)/2
c         
c         min at l1 and max at l2 are outside the interval
c         
      l = l1
      m = l2
 72   l = l + 1
      if(l.ge.m) go to 78
 73   if (a(l)-ac .le. 0) go to 72
 75   m = m - 1
      if(l.ge.m) go to 78
 76   if (a(m)-ac .gt. 0) go to 75
      az = a(m)
      a(m) = a(l)
      a(l) = az
      lz = la(m)
      la(m) = la(l)
      la(l) = lz
      go to 72
c         
c         since 75 is entered only if 73 is false, 75 is not tentative
c         but 72 is tentative, and must be corrected if no false 76 occurs
c         
 78   l = l - 1
      go to 20
 80   write(6,85) nsx
 85   format('0 === sorting incomplete. split exceeded',i3,' ===',/)
 90   return
      end




c         
c$$$      subprogram documentation block
c         .      .    .                                       .
c         subprogram:    distsp      distance on great circle
c         prgmmr: s. j. lord       org: w/nmc22    date: 91-06-06
c         
c         abstract: calculates distance on great circle between two lat/lon
c         points.
c         
c         program history log:
c         91-06-06  s. j. lord
c         yy-mm-dd  modifier1   description of change
c         yy-mm-dd  modifier2   description of change
c         
c         usage:    dxy=distsp(dlat1,dlon1,dlat2,dlon2)
c         input argument list:
c         dlat1    - latitude of point 1 (-90<=lat<=90)
c         dlon1    - longitude of point 1 (-180 to 180 or 0 to 360)
c         dlat2    - latitude of point 2 (-90<=lat<=90)
c         dlon1    - longitude of point 2
c         
c         
c         remarks: distance is in nm
c         
c         attributes:
c         language: indicate extensions, compiler options
c         machine:  nas, cyber, whatever
c         
c$$$      



      function distsp(dlat1,dlon1,dlat2,dlon2)

      USE gex
      USE const
      
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

      xxd=cosdb(dlon1-dlon2)*cosdb(dlat1)*cosdb(dlat2)+
     1     sindb(dlat1)*sindb(dlat2)
c         
      xxm=min(1.0d0,max(-1.0d0,xxd))
c         
      distsp=acos(xxm)*rearth*km2nm
      return
      end



      subroutine rumltlg(course,distance,rlat0,rlon0,rlat1,rlon1)

      USE gex
      implicit integer(i-n)
      implicit real(kind=iGaKind)(a-h,o-z)

c
c****       routine to calculate lat,lon after traveling "dt" time
c****       along a rhumb line specifed by the course and speed
c****       of motion
c             
c*****************            longitude is in degree west
c      
CCCC input distance (nm)      distance=speed*dt
c
c...     now find the lat/long point dt hours away along
c...     this course and heading
c         


      logical degeast

      degeast=.true.

      rad=4.0*atan(1.0)/180.0
      radinv=1.0/rad
      icrse=ifix(real(course)+0.01)
c
      if(icrse.eq.90.or.icrse.eq.270) then
c      
c*****            take care of due east and west motion
c
        dlon=distance/(60.0*cos(rlat0*rad))

        if(degeast) then
          if(icrse.eq.90) rlon1=rlon0+dlon
          if(icrse.eq.270) rlon1=rlon0-dlon
        else
          if(icrse.eq.90) rlon1=rlon0-dlon
          if(icrse.eq.270) rlon1=rlon0+dlon
        endif

        rlat1=rlat0
c
      else
c
        rlat1=rlat0+distance*cos(course*rad)/60.0
        d1=(45.0+0.5*rlat1)*rad
        d2=(45.0+0.5*rlat0)*rad
        td1=tan(d1)
        td2=tan(d2)
        rlogtd1=alog(real(td1))
        rlogtd2=alog(real(td2))
        rdenom=rlogtd1-rlogtd2 
        if(degeast) then
          rlon1=rlon0+(tan(course*rad)*rdenom)*radinv
        else
          rlon1=rlon0-(tan(course*rad)*rdenom)*radinv
        endif
c
      endif
c
      return
      end
      subroutine rumdirdist (sl,sg,el,eg,head,dist)
c
c..........................START PROLOGUE..............................
c
c  SCCS IDENTIFICATION:  @(#)dirdist.f90	1.1 6/1/96
c
c  CONFIGURATION IDENTIFICATION:
c
c  MODULE NAME:  dirspd
c
c  DESCRIPTION:  Calculate heading and speed from "sl,sg" to "el,eg",
c                in "time" hours for the tropics and sub-tropics
c
c  COPYRIGHT:                  (C) 1995 FLENUMOCEANCEN
c                              U.S. GOVERNMENT DOMAIN
c                              ALL RIGHTS RESERVED
c
c  CONTRACT NUMBER AND TITLE:  GS-09K-94-BHD-0107
c                              ADP SUPPORT FOR HIGHLY TECHNICAL SOFTWARE
c                              DEVELOPMENT FOR SCIENTIFIC APPLICATIONS
c
c  REFERENCES:  None
c
c  CLASSIFICATION:  Unclassified
c
c  RESTRICTIONS:  None
c
c  COMPUTER/OPERATING SYSTEM
c               DEPENDENCIES:   None
c
c  LIBRARIES OF RESIDENCE:
c
c  USAGE:  call dirspd (sl,sg,el,eg,head,dist)
c
c  PARAMETERS:
c     NAME         TYPE        USAGE             DESCRIPTION
c   --------      -------      ------   ------------------------------
c        sl        real        input    starting latitude, -SH
c        sg        real        input    starting longitude, (0 - 360 E,
c                                       or -W)
c        el        real        input    ending latitude, -SH
c        eg        real        input    ending longitude, (0 - 360 E,
c                                       or -W)
c      head        real        output   heading (deg)
c      dist        real        output   distance (nm)
c
c  COMMON BLOCKS:  None
c
c  FILES:  None
c
c  DATA BASES:  None
c
c  NON-FILE INPUT/OUTPUT:  None
c
c  ERROR CONDITIONS:
c         CONDITION                 ACTION
c     -----------------        ----------------------------
c    negative time             return negative speed
c
c  ADDITIONAL COMMENTS:
c
c...................MAINTENANCE SECTION................................
c
c  MODULES CALLED:  none
c
c  LOCAL VARIABLES:
c          NAME      TYPE                 DESCRIPTION
c         ------     ----       ----------------------------------
c         a45r       real       radians in 45 degrees
c         dist       real       distance between sl,sg and el,eg (nm)
c         eln1       real       calculation factor
c         eln2       real       calculation factor
c         ihead      int        integer of head times 10
c         inil       int        flag for initial calculations
c         ispd       int        integer of spd times 10
c         rad        real       degrees per radian
c         radi       real       radinas per degree
c         rdi2       real       0.5 times radi
c         tiny       real       tiny number, hardware dependent
c         xg         real       local copy of sg
c         xl         real       local copy of sl
c         xr         real       calculation factor
c         yr         real       calculation factor
c         yg         real       local copy of eg
c         yl         real       local copy of el
c
c  METHOD:  Based upon rhumb line calculations from Texas Instruments
c           navigation package for hand held calculator
c
c  INCLUDE FILES:  None
c
c  COMPILER DEPENDENCIES:  F77 with F90 extentions or F90
c
c  COMPILE OPTIONS:  Standard operational settings
c
c  MAKEFILE:
c
c  RECORD OF CHANGES:
c
c  <<change notice>>  V1.1  (05 JUN 1996)  H. Hamilton
c    initial installation of software on OASIS
c
c...................END PROLOGUE.......................................
c         
      USE gex
      implicit none
c
c         formal parameters

      real(kind=iGaKind)  sl, sg, el, eg, head, dist
c
c         local variables
      integer           inil
      double precision  a45r, eln1, eln2, rad, radi, rdi2, headd, distd
      double precision  xg, xl, yg, yl, xr, yr, tiny
c
      save inil, rad, radi, rdi2, a45r
c
      data inil/-1/, tiny/0.1e-8/
c . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
c
      if (inil .ne. 0) then
        inil = 0
        rad  = 180.0d0/acos (-1.0d0)
        radi = 1.0d0/rad
        rdi2 = 0.5d0*radi
        a45r = 45.0d0*radi
      endif
c
c                   pre-set heading and distance, for same point input
c
      head = 0.0
      dist = 0.0
      if (abs (sl -el).gt.tiny .or. abs (sg -eg).gt.tiny) then
        xl = sl
        xg = sg
c
c                   if longitude is west, convert to 0-360 East
c
c       if (xg .lt. 0.0d0) xg = xg +360.0d0
        yl = el
        yg = eg
c
c                   if longitude is west, convert to 0-360 East
c
c       if (yg .lt. 0.0d0) yg = yg +360.0d0
c
c                    check for shortest angular distance
c
        if (xg.gt.270.0d0 .and. yg.lt.90.0d0) yg = yg +360.0d0
        if (yg.gt.270.0d0 .and. xg.lt.90.0d0) xg = xg +360.0d0
c
        if (abs (xl -yl) .le. tiny) then
c
c                    resolve 90 or 270 heading
c
          head = 90.0
          if (yg .lt. xg) head = 270.0
          dist = 60.0d0*(yg -xg)*cos (xl*radi)
        else
          distd = 60.0d0*(xl -yl)
          if (abs (xg -yg) .le. tiny) then
c
c                  resolve 0 or 180 heading, note head is preset to zero
c
            if (yl .lt. xl) head = 180.0
            dist = distd
          else
c                   CHECK FOR POSITIONS POLEWARD OF 89+ DEGREES LATITUDE
cCC         IF (ABS (XL).GT.PLMX .OR. ABS (YL).GT.PLMX) THEN
c           (HARDWARE DEPENDENT - NOT REQUIRED FOR TROPICAL CYCLONES)
cCC           XLT = XL
cCC           IF (ABS (XLT) .GT. PLMX) XLT = SIGN (PLMX,XL)
cCC           YLT = YL
cCC           IF (ABS (YLT) .GT. PLMX) YLT = SIGN (PLMX,YL)
cCC           XR = TAN (XLT*RDI2 +SIGN (A45R,XL))
cCC           YR = TAN (YLT*RDI2 +SIGN (A45R,YL))
cCC         ELSE
              xr = tan (xl*rdi2 +sign (a45r,xl))
              yr = tan (yl*rdi2 +sign (a45r,yl))
cCC         ENDIF
            eln1  = sign (log (abs (xr)),xr)
            eln2  = sign (log (abs (yr)),yr)
            headd = rad*(atan ((xg -yg)/(rad*(eln1 -eln2))))
            if (yl    .lt. xl)  headd = headd +180.0d0
            if (headd .lt. 0.0) headd = headd +360.0d0
c
c                   correct initial distance, based only on latitude
c
            dist = distd/cos (headd*radi)
            head = headd
          endif
        endif
        dist = abs (dist)
      endif
      return

      end

      subroutine rumlatlon (head,dist,slat,slon,elat,elon)
c
c..........................START PROLOGUE..............................
c
c  SCCS IDENTIFICATION:  @(#)expltln.f90	1.1 6/1/96
c
c  CONFIGURATION IDENTIFICATION:
c
c  MODULE NAME:  expltln
c
c  DESCRIPTION:  extraplolate lat/lon based upon starting lat/lon,
c                heading and distance
c
c  COPYRIGHT:                  (C) 1996 FLENUMOCEANCEN
c                              U.S. GOVERNMENT DOMAIN
c                              ALL RIGHTS RESERVED
c
c  CONTRACT NUMBER AND TITLE:  GS-09K-94-BHD-0107
c                              ADP SUPPORT FOR HIGHLY TECHNICAL SOFTWARE
c                              DEVELOPMENT FOR SCIENTIFIC APPLICATIONS
c
c  REFERENCES:  None
c
c  CLASSIFICATION:  Unclassified
c
c  RESTRICTIONS:
c    Restricted to tropics and sub-tropics - latitudes of tropical cyclones
c
c  COMPUTER/OPERATING SYSTEM
c               DEPENDENCIES:   None
c
c  LIBRARIES OF RESIDENCE:
c
c  USAGE:  call expltln (head,dist,slat,slon,elat,elon)
c
c  PARAMETERS:
c     NAME       TYPE      USAGE             DESCRIPTION
c   --------    -------    ------    ------------------------------
c     head       real      input     rhumb-line heading in degrees from
c                                    slat/slon
c     dist       real      input     distance from slat/slon to elat/elon (nm)
c     slat       real      input     starting latitude, negative if South
c     slon       real      input     startint longitude, in degrees East
c     elat       real      output    extrapolated latitude, negative if South
c     elon       real      output    extrapolated longitude, in degrees East
c
c  COMMON BLOCKS:  None
c
c  FILES:  None
c
c  DATA BASES:  None
c
c  NON-FILE INPUT/OUTPUT:  None
c
c  ERROR CONDITIONS:  none
c
c  ADDITIONAL COMMENTS:
c
c     Uses rhumb-line approximations.
c
c...................MAINTENANCE SECTION................................
c
c  MODULES CALLED: none
c
c  LOCAL VARIABLES:
c      NAME      TYPE               DESCRIPTION
c     ------     ----     ----------------------------------
c     degrad     real     degrees to radians conversion factor
c     dlon       real     delta longitude from slon to elon for
c                         090 or 270 heading
c     hdgrad     real     half of degrad
c     icrs        int     nearest integer of heading (deg)
c     inil        int     initialization flag, 0 - not initialized
c     raddeg     real     radian to degrees conversion factor
c     rad045     real     45 degrees expressed in radians
c     rdhd       real     heading in radians
c
c  METHOD:  Based upon rhumb-line calculations frpm Texas Instruments
c           Navigation Psckage for hand-held calculator
c
c  INCLUDE FILES:  None
c
c  COMPILER DEPENDENCIES:  F90
c
c  COMPILE OPTIONS:  Standard operational settings
c
c  MAKEFILE:
c
c  RECORD OF CHANGES:
c
c  <<change notice>>  V1.1  (05 JUN 1996)   H. Hamilton
c    initial installation of software on OASIS
c
c...................END PROLOGUE.......................................
c
      implicit none
c
c         formal parameters
      real(kind=8)  head, dist, slat, slon, elat, elon
c
c         local variables
      integer  icrs, inil
      real(kind=8)     tiny
      double precision  dlon, rdhd, raddeg, degrad, hdgrad, rad045
c
      save inil, raddeg, degrad, hdgrad, rad045
c
      data inil/-1/, tiny/0.1e-6/
c . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
c
      if (inil .ne. 0) then
        inil   = 0
        degrad = acos (-1.0d0)/180.0d0
        hdgrad = 0.5d0*degrad
        rad045 = 45.0d0*degrad
        raddeg = 1.0d0/degrad
      endif
c
c         latitude, minus for South, longitude, degrees East (0 - 360)
c
      icrs = nint (head)
      if (abs (head -90.0) .lt. tiny .or. abs (head -270.0) .lt. tiny)
     &  then
        dlon = dist/(60.0*cos (slat*degrad))
        if (icrs .eq. 270) then
          elon = slon -dlon
        else
          elon = slon +dlon
        endif
        elat = slat
      else
        rdhd = head*degrad
        elat = slat +(dist*cos(rdhd)/60.0d0)
        if (icrs .eq. 180 .and. abs (head -180.0) .gt. tiny) then
          icrs = 181
        elseif (icrs .eq. 360 .and. abs (head -360.0) .gt. tiny) then
          icrs = 359
        endif
        if (mod (icrs,180) .ne. 0) then
c
c                   Following test NOT required for tropical cyclones
c!!       IF (ABS (ELAT) .GT. 89.0) ELAT = SIGN (89.0,ELAT)
c
          elon = slon +raddeg*(log (tan (rad045 +hdgrad*elat))
     &          -log (tan (rad045 +hdgrad*slat)))*tan (rdhd)
        else
          elon = slon
        endif
      endif
      return
c
      end
