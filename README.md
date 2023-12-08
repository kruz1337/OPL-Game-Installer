# OPL-Game-Installer
You can't write more than 4GB of data to a FAT32 disc, so you can't easily install a game on a disc. This repository allows you to do this easily, it also creates a [Virtual Memory](https://en.wikipedia.org/wiki/Virtual_memory) Card of the desired size for the game and downloads the images of the game from the server.

#### What is Virtual Memory Card (VMC)?
Virtual Memory Card allows you to save the game on the disc you use in games without using an memory card.

![](https://img.shields.io/badge/language-c++-e76089?style=plastic) ![](https://img.shields.io/badge/license-MIT-green?style=plastic) ![](https://img.shields.io/badge/arch-x64%20%7C%20x86-d9654f?style=plastic) ![](https://img.shields.io/badge/config-Debug%20%7C%20Release-c0c0c0?style=plastic)

![Image of RequestX International Developer Group on Discord](https://raw.githubusercontent.com/kruz1337/OPL-Game-Installer/main/thumbnail.png)

## How to build OPL-Game-Installer with Curl?
* If you don't know how do you do this go download released version.
  
* First of all you should download project files on project page or clone this repository from GitBash or GitHub Desktop on your PC. [OPL-Game-Installer.zip](https://github.com/kruz1337/OPL-Game-Installer/releases)

* If you download project files with manual method you need extract zip file.

* Open the "include" folder into the "OPL Game Installer/libcurl" directory, then copy Curl includes files into it. 

* Then, compile the Curl library according to your build configuration and create a "release64" folder for "Release | x64" build. The folder layout should be like this:
```
./OPL Game Installer/libcurl
├─include
├─release64
├─release86
├─debug64
└─debug86
```
  
* After then, copy Curl compiled library file into the folder you opened.

* If the name is different, change it to "libcurl_a" or you should change it from ```"Linker > Additional Dependencies"``` in project settings.

* Run .sln file on Visual Studio (2019+).

* Press Build button or press <kbd>CTRL+B</kbd> on your keyboard.

* Check out bin folder include that.

## Usage with Arguments
```"./OPL Game Installer.exe" <Iso Path> <Root Directory> <Disk Type> [OPTIONS]```

### Options:
```
--copy               If the size of the Iso file does not exceed the limit, its allows copy directly to the disc.
--add-cover          Download arts of the game.
--create-vmc <Size>  Creates virtual memory of the entered size once defined. (Size=MB)
```

## TO-DO List
* Add ability of fetching the Size, Title, Type, Star Count and Description of the game from the server.
* Add fetching random images from the server.
* Add property of converting ul to iso.
* Build for Linux.
