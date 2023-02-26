# screenshot
A very smowl xlib c screenshot tool
It will save the file into ~/Screenshots if it exists and will copy the image to clipboard
Just drag the area you want

### Config
Config is in the first few lines of `main.c`  
`DEBUG`: Enables debug info!  
`OPTDIR`: Specifies where to place screenshots (relative to home), if invalid or undefined /tmp/ is used  
`OPTQUALITY`: The WEBP quality to use (where 0 is worst and 100 is best)  
`OPTFPS`: How frequently to accept mouse move events (too high or low makes it laggy)  
`OPTWIDTH`: The width of the selection outline  
`OPTR/G/B`: The color of the selection outline  

### Compiling
You need xlib, xclip and libwebp  
Use `make` to compile  
Use `make install` to syslink the executable to bin so you can just run "screenshot"  

### Usage
Run `screenshot` to select an area manually  
Run `screenshot 1` to screenshot the whole screen

