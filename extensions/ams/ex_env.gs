function utenv()
*
* Demonstrate environment variable related commands
*

 'reinit'

* getenv - print value of each environment variable
* ------
  say ''
  say '>>> Quering existing environment variables'
  say 'ga-> getenv USER HOME TMPDIR'
 'getenv USER HOME TMPDIR'

* setenv - set the value of a given environment variable
* ------  
  say ''
  say '>>> Setting environment variables'
  say 'ga-> setenv OPENGRADS /share/dasilva/opengrads'
 'setenv OPENGRADS /share/dasilva/opengrads'

* printenv - expand environment variables in a string
* --------
  say ''
  say '>>> Expand environment variables in strings'
  say 'ga-> printenv $HOME/src/grads/test_data'
 'printenv $HOME/src/grads/test_data'

* Here is a handy way to get the PID for creating temporary files, etc
* --------------------------------------------------------------------
  say ''
  say '>>> Use $$ to get the PID, say, for creating temporary files'
  say 'ga-> printenv $TMPDIR/tempfile.$$'
  'printenv $TMPDIR/tempfile.$$'

* env - expand environment variables, then execute command
* ---
  say ''
  say '>>> Expand environment variables, then execute command'
  say 'ga-> env open $OPENGRADS/test_data/model.ctl'
 'env open $OPENGRADS/test_data/model.ctl'
 'close 1'  

* @ - another name for "env"; here is how to set env variables 
*     based on another env variable
* --------------------------------------------------------------
  say ''
  say '>>> "@" is a short-hand for "env"'
  say 'ga-> @ setenv FILE $OPENGRADS/test_data/model.ctl'
  '@ setenv FILE $OPENGRADS/test_data/model.ctl'
  say 'ga-> @ open $FILE' 
  '@ open $FILE'

* Save complicate expressions in env vars (like aliases), then run them
* ---------------------------------------------------------------------
  say ''
  say '>>> You can use env variables to save long expressions'
  say 'ga-> setenv EXPR ua;va;sqrt(ua*ua+va*va)'
  'setenv EXPR ua;va;sqrt(ua*ua+va*va)'
  say 'ga-> @ display $EXPR'
  '@ display $EXPR'

  say ''
  say '>>> All done.'
  say ''

return


