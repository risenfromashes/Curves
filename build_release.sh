# arg1 workspaceFolder
# arg2 relative dirname
# arg3 relative file Path
# arg4 file Base Name
# arg5 build target x86/x64
cd $1
if [[ "$OSTYPE" == "linux-gnu"* ]]
then 
    g++ -IOpenGL/include -LOpenGL/dll $3  -o ./bin/$4 -O3 -static-libgcc -static-libstdc++ -lGL -lglut
    cd bin
    7z a -tzip $4_linux_x64.zip $4
    mv $4_linux_x64.zip ../release
else 
    if [[ "$5" == "x64" ]]
    then 
        export PATH=/c/msys64/mingw64/bin:$PATH
    else
        export PATH=/c/msys64/mingw32/bin:$PATH
    fi
    if [[ -f $2/$4.rc ]]
    then 
        windres $2/$4.rc -O coff -o $2/$4.res
        g++ -IOpenGL/include -LOpenGL/dll/$5 $3  -o ./bin/$5/$4.exe ./$2/$4.res -mwindows -O3 -static-libgcc -static-libstdc++ -lfreeglut -lOPENGL32 -lgdi32
        rm $2/$4.res
    else
        g++ -IOpenGL/include -LOpenGL/dll/$5 $3  -o ./bin/$5/$4.exe -mwindows -O3 -static-libgcc -static-libstdc++ -lfreeglut -lOPENGL32 -lgdi32
    fi
    cd $2
    if [[ -f $4_pb.sh ]]
        then
        ./$4_pb.sh
    fi
    cd $1
    for file in ./OpenGL/dll/$5/*.dll
    do
        cp $file ./bin/$5
    done
    cd bin/$5
    7z a -tzip $4_win_$5.zip $4.exe ./*.dll
    mv $4_win_$5.zip ../../release
fi
