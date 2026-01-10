# FanBoy

The FanBoy is a custom Gameboy emulator I wrote in C++ as a project of my resume.  The emulator overall took around of a month of both programming and planning.  

## Setup
Download and install CMake for your specific operating system (https://cmake.org/download).  

Go to the latest release of SDL2 (https://github.com/libsdl-org/SDL/releases) and download SDL2-devel.  

Unzip the SDL2 files into a safe and easily accessible folder location.  

Clone the FanBoy repo.  

Open the repository and create a new build directory inside of it using the file explorer or the following command:
> mkdir build

Run the following command inside of the build folder to generate the CMake BuildSystem:
> cmake ..

After the CMake project was generated,  go into the CMakeCache.txt file and edit the variable SDL2_DIR so that it is set to the location of SDL2-devel

Run the following command to build a runnable executeable.  
> cmake --build .

Finally drag SDL2.dll that is inside of the lib folder of SDL2-devel into the same folder as the executable.

## Screenshots
<p align="center">
    <img src="images/mario.png" style="margin-right: 30% margin-left: 30%;" width="350" height="350"> 
    <img src="images/pokemon.png" style="margin-right: 30% margin-left: 30%;"width="350" height="350"> 
    <img src="images/zelda.png" style="margin-right: 30% margin-left: 30%;" width="350" height="350"> 
    <img src="images/tetris.png" style="margin-right: 30% margin-left: 30%;" width="350" height="350"> 
</p>



