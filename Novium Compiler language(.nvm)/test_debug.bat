@echo off
"C:\Users\uchih\Novium Programming language\Novium Compiler language(.nvm)\build\novium.exe" --check "..\examples\minimal.nvm" > novium_output.txt 2>&1
echo EXIT: %errorlevel%