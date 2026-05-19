git status *.txt | sed -e "1,/git add /d" | tr "\t" "," | sed -e "/^[^,]/d" -e "s/^,//" | tr / "\\\\" > c:\tmp\files.txt
for /f %%f in (c:\tmp\files.txt) do @(cls && ((echo %%f && type %%f ) | more) && pause)
