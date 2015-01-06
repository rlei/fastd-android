fastd for Android
=================
An unofficial Android port of [fastd](https://projects.universe-factory.net/projects/fastd/wiki).

Runtime Requirements
--------------------
* Android 4.1+
* x86 / ARMv7
  * NEON support is planned but not currently required
  * Not tested with x86\_64 or AArch64 but should work too

How to Build
------------
* Android NDK r10d+ (r10c or older versions won't work)
    * make sure ANDROID\_NDK\_HOME is set
* Ubuntu 12.04+
    * `sudo apt-get install build-essential automake bison cmake libtool pkg-config`
    * For Ubuntu **12.04**: cmake 2.8.7 won't work; get a newer version from https://launchpad.net/~kalakris/+archive/ubuntu/cmake
* or Mac OS X 10.10+ (older version should work too)
    * Homebrew
    * `brew install automake libtool cmake bison`

Then simply run `./build-fastd-android.sh` from `fastd-android` folder. Be warned the script is not perfect; you may need to look into it should anything go wrong.

