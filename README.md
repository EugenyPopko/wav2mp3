# wav2mp3
WAV to MP3 converter. Program implemented by using LAME library, queue and pthreads.

Windows realization
 - Tested on Visual Studio 2017
 - Uses lame-3.100. 
 - Uses implementation of pthread (ftp://sourceware.org/pub/pthreads-win32/dll-latest)
 - Uses implementation of dirent.h (https://github.com/tronkko/dirent)

Linux implementation
 - Tested on Ubuntu 14.04 LTS 64-bit
 - Uses lame-3.100.
 Compile program: g++ -o wav2mp3 wav2mp3.cpp -linclude/lame -static libmp3lame.a -lpthread -lm
 