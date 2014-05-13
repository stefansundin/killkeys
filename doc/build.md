# Introduction

Your first task is to download the KillKeys source code from Github, either through Git or by [downloading a zip of the latest code](https://github.com/stefansundin/killkeys/zipball/master).

If you get the code through Git, I recommend TortoiseGit. You need to download two packages:
- https://msysgit.github.io/
- https://code.google.com/p/tortoisegit/wiki/Download


## Compiler

I use [mingw-w64](http://mingw-w64.sourceforge.net/) to build KillKeys. Contrary to its name, there is also a 32-bit compiler available. You need the 32-bit toolchain to build 32-bit binaries, and the 64-bit toolchain to build 64-bit binaries. So if you want to build for both 32-bit and 64-bit, you will need to get both toolchains.

The [mingw-w64 file download](http://sourceforge.net/projects/mingw-w64/files/) section is a mess, here are two downloads that should work: [mingw-w32](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/rubenvb/gcc-4.7-release/i686-w64-mingw32-gcc-4.7.4-release-win32_rubenvb.7z/download) (rename `windres.exe` to `i686-w64-mingw32-windres.exe`), [mingw-w64](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/rubenvb/gcc-4.7-release/x86_64-w64-mingw32-gcc-4.7.4-release-win32_rubenvb.7z/download). Their buildbot seems to have problems building nightly versions for windows lately. If you get errors about `windres`, open `build.bat` and set `prefix32` to an empty string.

It doesn't matter where you extract them, but I usually use `C:\mingw-w32` and `C:\mingw-w64`.

Note that the mingw-w32 and mingw-w64 toolchains have prefixed binaries, e.g. the name of `gcc` for mingw-w32 is `i686-w64-mingw32-gcc` and for mingw-w64 is `x86_64-w64-mingw32-gcc`. The normal MinGW binaries does not have any prefixes. This means that you can have MinGW, mingw-w32 and mingw-w64 installed and all three in PATH without any conflicts. It does make it a bit annoying to type manually though, but I have a build script that will make it easier.


## Configure PATH

After downloading the compilers, you need to add them to the PATH variable so the build script can use the them. Do this by _right clicking_ on the _Computer_ desktop icon, go to the _Advanced_ tab, press the _Environment variables_ button, locate the PATH variable and add _<mingw path>\bin_ to the list, and press ok on all the dialogs. If you want to build both 32-bit and 64-bit, add both to PATH.

Here is an example of my PATH variable: `C:\mingw-w32\bin;C:\mingw-w64\bin`

If you had the terminal open when you changed the PATH variable, you must restart it.


# Build it

Now everything should hopefully work when you run `build.bat`. It's more comfortable to run it in a terminal so you can see all the output. You can also run `build run` to automatically run KillKeys after it has been built. To build the 64-bit binary run `build x64`.


## Installer

To build the installer you need to download [NSIS Unicode](http://www.scratchpaper.com/). Put NSIS in PATH, then run `build all` in the terminal to automatically build the installer. Run `build all x64` to build and include the 64-bit part in the installer.


# Video walkthrough

If you don't get it working, it might help to watch [this video](https://www.youtube.com/watch?v=4ENwQxSr_So). In it I explain how to compile another program of mine, SuperF4.


# Improve this page

Post [in the subreddit](http://www.reddit.com/r/stefansundin/) to suggest improvements to this page.
