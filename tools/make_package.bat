@echo off
setlocal

:: 设置Bandizip的安装路径
set "BANDIZIP_PATH=C:\Program Files\Bandizip\Bandizip.exe"

:: 设置要打包的文件夹名称
set "FOLDER_NAME=HDL APP"

:: 设置压缩包名称
set "ZIP_NAME=RTU DEV V2 0.zip"

:: 调用Bandizip进行压缩
"%BANDIZIP_PATH%" -a -r -m 9 -o "%ZIP_NAME%" %FOLDER_NAME%


:: 提示压缩完成
echo 压缩完成！请查看 %ZIP_NAME%

endlocal
