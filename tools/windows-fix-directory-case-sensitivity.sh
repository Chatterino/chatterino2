find . -not -path "*.git*" -exec fsutil.exe file setCaseSensitiveInfo {} disable \;
