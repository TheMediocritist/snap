# snap
Copy secondary framebuffer to PNG file. Intended for use with [Beeperberry](https://beepberry.sqfmi.com) to enable screenshots to be taken of the Sharp Memory Display.

Forked from https://github.com/AndrewFromMelbourne/fb2png with the following changes:
* uses 1-bit Sharp Memory Display buffer

## Usage

    snap <options>
    
    --device <device>    - framebuffer device (default /dev/fb1)
    --pngname <filename> - output filename (default "fb.png")
    --help               - print usage and exit

### Notes
1. Nil

## How to build

1. Install the build tool cmake and the libbsd-dev library (if required)
    ```
    sudo apt-get install cmake
    sudo apt-get install libbsd-dev
    ```
2. Download this repo
    ```
    git clone https://github.com/TheMediocritist/snap
    ```
4. Build the executable
    ```
    cd snap
    mkdir build
    cd build
    cmake ..
    make
    ```
3. Test to ensure it's running correctly (ctrl+c to quit)
    ```
    ./snap
    ```
4. Install to binaries folder
    ```
    sudo make install
    ```

### How to uninstall

1. Delete the executable file
    ```
    sudo rm /usr/local/bin/snap
    ```
