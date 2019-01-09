function main(args)

'open ../../test_data/model'
'ipc_verb'
'set gxout shaded'

say ''
say '*** Simple save/load'
say ''
        'ipc_save ps ps.bin'
        'define saved = ipc_load("ps.bin")' 
        'display saved'
        'set gxout stat'
        'display ps-saved'
        say result
        'set gxout shaded'
        pull ans
       'clear'

say ''
say '*** The same example, using B<open> and B<close>:'
say ''

        'ipc_open ps.bin w'
        'ipc_save ps'
        'ipc_close'

        'ipc_open ps.bin r'
         'display ps-ipc_load()'
        'ipc_close'
       pull ans
       'clear'

say ''
say '*** Saving a timeseries:'
say ''

        'set lon 0 360'
        'set lat -90 90'
        'set lev 300'
        'set t 1 5'
        'ipc_open zg.bin w'
        'define void = ipc_save(zg)'
        'ipc_close '

say ''
say '*** Retrieving the same time series:'
say ''

        'set t 1 5'
        'ipc_open zg.bin r'
        'define saved = ipc_load()'
        'ipc_close '
        'display saved '

say ''
say '** all done.'
say ''

return
