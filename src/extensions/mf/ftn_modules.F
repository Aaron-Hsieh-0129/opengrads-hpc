      module inparms
        type gficard  ! Define a new type for the input parms
          sequence
          real(kind=8) ::      undef  ! Undefined value for the grid
          integer   itype  ! i dimension:  0 = X-dimension
          integer   jtype  ! j dimension:  0 = Y-dimension
          integer   nx     ! num pts in i-direction
          integer   ny     ! num pts in j-direction
          integer   numpts ! num pts on grid (nx*ny)
          real(kind=8) ::      xbegi  ! lon of first point
          real(kind=8) ::      dxi    ! grid increment in i-direction
          real(kind=8) ::      ybegi  ! lat of first point
          real(kind=8) ::      dyi    ! grid increment in j-direction
        end type gficard

        type datecard  ! Define a new type for the input namelist parms
          sequence
          integer       byy    ! Beginning yy of date to search for 
          integer       bmm    ! Beginning mm of date to search for 
          integer       bdd    ! Beginning dd of date to search for 
          integer       bhh    ! Beginning hh of date to search for 
          integer       model  ! integer identifier for model data used
        end type datecard

      end module inparms

      module tracked_parms
          real(kind=8), save, allocatable  ::  xdata_in(:)
          real(kind=8), save, allocatable  ::  xin(:)
          real(kind=8), save, allocatable  ::  yin(:)
          real(kind=8), save, allocatable  ::  xdata_2d(:,:)
          real(kind=8), save, allocatable  ::  xlon(:)
          real(kind=8), save, allocatable  ::  ylat(:)
          logical(1), save, allocatable  ::  defined_pt(:,:)

      end module tracked_parms

      module const
        use gex

        real(kind=iGaKind), save :: pi =  4. * atan(1.)
        real(kind=iGaKind), save :: dtr =  (4. * atan(1.))/180.0
        real(kind=iGaKind), save :: dtk = 111.1949     ! Dist (km) over 1 deg lat
                                         ! using erad=6371.0e+3
        real(kind=iGaKind), save :: erad = 6371.0e+3   ! Earth's radius (m)
        real(kind=iGaKind), save :: ecircum = 40030.2  ! Earth's circumference
                                         ! (km) using erad=6371.e3
        real(kind=iGaKind), save :: omega = 7.292e-5

        real(kind=iGaKind), save :: rkm2nm=(60.0/111.1949)
        real(kind=iGaKind), save :: rnm2km=(111.1949/60.0)

      end module const

