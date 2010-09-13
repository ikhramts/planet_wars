::A script for creating a submission .zip file.
@ECHO OFF

::Clean up previous submission.
IF EXIST submission.zip del /q submission.zip
IF EXIST submission del /q submission\*.*

::Create a submission directory.
IF NOT EXIST submission mkdir submission
copy planet_wars\*.cc submission
copy planet_wars\*.h submission
copy planet_wars\makefile submission

::Create the submission zip file.
7z a -tzip -r submission.zip submission