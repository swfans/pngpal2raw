# pngpal2raw

**Bullfrog Engine graphic formats converter**

Converts PNG file with given PAL colors to Bullfrog Engine graphic sprites or RAW images.

## About

This tool converts `.png` files into formats which are native to the game engine.
It can bes used to create `.raw` files, but also `.dat` and `.jty` sprite catalogues.

It requires either `.png` or `.txt` file with list of png images at input. 
Additionally, there must be input pal file which stores palette used for the sprites.

## Building

This tool should build and work on any CPU architecture.

### General building instructions

To build **Bullfrog Engine graphic formats converter**, you will need the following:

* GNU Autotools
* GNU C++ compiler
* development versions of the following libraries:
  * libpng
  * zlib

Once you've made sure you have the above, proceed with the following steps:

1. go into the directory with `pngpal2raw` source release (containing `res`, `src` etc.)
2. do `autoreconf -if` to create build scripts from templates
3. do `./configure` to make the build scripts find required toolchain and libraries
4. do `make` to compile the executable file

You should now have a working `pngpal2raw` executable file.

#### Build example - Ubuntu 22.04

Here are specific commands required to compile the executable on Ubuntu linux.

Install the dependencies:

```
sudo apt install gcc-multilib g++-multilib lib32z1
sudo apt install build-essential autoconf libtool make pkg-config
sudo apt install libpng-dev
```

Now as our host is ready, we can start working on the actual `pngpal2raw` sources.
Go to that folder, and generate build scripts from templates using autotools:

```
autoreconf -ivf
```

Next, proceed with the build steps; we will do that in a separate folder.

```
mkdir -p release; cd release
../configure
make V=1
```

The `V=1` variable makes `make` print each command it executes, which makes
diagnosing issues easier.

On success, you will have the executable.

Finally, you can copy the files to some installation folder, ie. `pkg`:

```
make V=1 DESTDIR=$PWD/pkg install
```

## Done

This concludes the document.
Remember that with most console tools you can use `--help`.
