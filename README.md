# screenshot
A very smol xlib screenshot tool written in C

## Config
Config is in the first few lines of `main.c`  
`DEBUG`: Enables debug info  
`OPTDIR`: Specifies where to place screenshots, supports `~`, you must make this directory manually
`OPTQUALITY`: The WEBP quality to use (where 0 is worst and 100 is best)  
`OPTWIDTH`: The width of the selection outline  
`OPT[R/G/B]`: The color of the selection outline  

## Compiling
Requires `xlib`, `xclip` and `libwebp`  
Use `make` to compile  
Use `make debug` to compile with debug info

## Installing
Run `make install` to install  
Run `make uninstall` to uninstall  

## Usage
Run `screenshot` to select an area manually  
Run `screenshot 1` to screenshot the whole screen

