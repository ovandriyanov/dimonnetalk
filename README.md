# DimonneTalk
## Synopsis
This is a Telegram bot for translating normal text to Dimonne notation.

## Dependencies
* C++ 14 capable C++ compiler (currently tested with GCC 6.3.0)
* Development version of [Boost libraries](http://www.boost.org/), which includes not yet released boost.beast HTTP library
* [OpenSSL](https://www.openssl.org/)
* [Lua](https://www.lua.org/) >= 5.3

## Building dependencies
Here are steps for building **DimonneTalk** on Unix:
* Clone and build developoment version of Boost:
```bash
git clone https://github.com/boostorg/boost.git
cd boost
./bootstrap.sh --prefix="`pwd`/build"
./bjam # Add -jN option to speed up the build where N is the number of concurrent build jobs
./bjam install
```
* Clone and build OpenSSL:
```bash
git clone https://github.com/openssl/openssl.git
cd openssl

```
