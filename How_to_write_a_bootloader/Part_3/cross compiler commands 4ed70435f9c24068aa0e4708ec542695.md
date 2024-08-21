# cross compiler commands

```
sudo apt install build-essential
sudo apt install bison
sudo apt install flex
sudo apt install libgmp3-dev
sudo apt install libmpc-dev
sudo apt install libmpfr-dev
sudo apt install texinfo
sudo apt install libisl-dev
```

Download GCC [https://ftp.lip6.fr/pub/gcc/releases/](https://ftp.lip6.fr/pub/gcc/releases/)

Download binutils [https://ftp.gnu.org/gnu/binutils/](https://ftp.gnu.org/gnu/binutils/)

Extract binulits to src folder in home dir

Pasete exports into terminal

```
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
```

Run every command from home dir

```
cd $HOME/src

mkdir build-binutils
cd build-binutils
../binutils-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install
```

Extract donwloaded gcc to src and run this from home dir (remember abotu exports)

```
cd $HOME/src

# The $PREFIX/bin dir _must_ be in the PATH. We did that above.
which -- $TARGET-as || echo $TARGET-as is notin the PATH

mkdir build-gcc
cd build-gcc
../gcc-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
```

check version

```
$HOME/opt/cross/bin/$TARGET-gcc --version
```