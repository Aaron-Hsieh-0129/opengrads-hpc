' 
' Lightweight script to call actual executables down bellow. Eventually it could
' include the same functionality of the Perl version. Now it seats in the very
' top, above Contents/
'

Dim objFileSystem, objFILE

' Find out where we actually are
' ------------------------------
  ScriptPath = Left(WScript.ScriptFullName, _
               Len(WScript.ScriptFullName) - Len(WScript.ScriptName))

' Get the current version from file
' ---------------------------------
  Set objFileSystem = CreateObject("Scripting.fileSystemObject")
  Set objFILE = objFileSystem.OpenTextFile(ScriptPath & "Contents\Cygwin\Versions\Current@", 1)
  Versions = Split(objFILE.ReadAll, vbCrLf)
  Version = Left(Versions(0),Len(Versions(0))-1)
  objFile.Close
  Set objFileSystem = Nothing
  
' Actual executable path
' ----------------------
  ActualPath = ScriptPath & "Contents\Cygwin\Versions\" & Version & "\i686\" 
  ExecutableName = Left(WScript.ScriptName,Len(WScript.ScriptName)-4) & ".exe"
  ExecutableFullName = ActualPath & ExecutableName
  ' WScript.echo "Running <" & ExecutableFullName & ">"
  
' Command line arguments
' ----------------------
  Set ArgObj = WScript.Arguments
  sArgCount = ArgObj.Count
  args = " "
  For x = 0 to sArgCount - 1
    args = args & " " & ArgObj(x)
  Next
  set ArgObj = Nothing
 
 'Start actual application down below
 '-----------------------------------
 Set objShell = CreateObject("WScript.Shell")
 ' objShell.Run "%COMSPEC% /k" & ExecutableFullName & args
 objShell.Run ExecutableFullName & args
 Set objShell = Nothing
 
 
  
  