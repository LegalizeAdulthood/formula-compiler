@setlocal
@set path=%path%;%ProgramFiles%\Pandoc
pandoc -f html -t plain "%1" > "%2"
