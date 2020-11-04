# arg1 workspaceFolder
# arg2 relative dirname
# arg3 relative file Path
# arg4 file Base Name
cd $1

if [[ "$OSTYPE" == "linux-gnu"* ]]
then 
    g++ -IOpenGL/include -LOpenGL/dll $3  -o ./bin/$4 -lGL -lglut
    ./bin/$4
else 
    export PATH=/c/msys64/mingw64/bin:$PATH
    g++ -IOpenGL/include -LOpenGL/dll $3  -o ./bin/$4 -lfreeglut -lOPENGL32
    for file in OpenGL/dll/x64/*.dll
    do
        cp $file ./bin/
    done
    ./bin/$4.exe
fi