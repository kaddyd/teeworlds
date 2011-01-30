@echo off
REG ADD HKEY_CLASSES_ROOT\teeworlds\shell\open\command\ /f /ve /t REG_SZ /d "%CD%\teeworlds.exe \"connect %%1\""
REG ADD HKEY_CLASSES_ROOT\teeworlds\ /v "URL Protocol" /f /t REG_SZ /d ""
REG ADD HKEY_CLASSES_ROOT\teeworlds\ /v "EditFlags" /f /t REG_DWORD /d 2
echo Windows protocol handler registered
