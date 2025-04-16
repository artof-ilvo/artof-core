# ARTOF core processes

## Introduction

This CPP project contains the source code of the processes running in the operation layer of the [ARTOF](https://artof-ilvo.github.io) project.


## Build source code
Create a build directory for debug and release builds.
```
mkdir -p build/debug && mkdir -p build/release
```
1. To debug the code use the command below. You can also compile the tests by adding the option `DUTEST=1` and `NO_ASAN=1` to disable asan.
```
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug -DINSTALL_FOLDER="/usr/bin" ../..
```

Use `make -j8` to build the source code and use `make install` to install the binaries (to `/usr/bin` in this example).


2. To release the code use the command below. 
```
cd build/release
NO_ASAN=1 cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_FOLDER="<project-folder>/ilvo-artof-core/usr/bin" ../..
```

Use `make -j8` to build the source code and use `make install` to install the copy the binaries to the debian package folder.

3. (Optional) build dev package and push to minio
```
dpkg-deb --build artof-core artof-core-dev.deb
mc cp artof-core-dev.deb ilvo-minio/tv115-ilvo-robotics
```

## Build docker folder

Mirror the initial program files to the docker folder.

```
cp -ralf artof-core/var/lib/docker/field/* docker/data/field
cp -ralf artof-core/var/lib/docker/implement/* docker/data/implement
cp -alf artof-core/var/lib/docker/settings.json docker/data/settings.json
cp -alf artof-core/var/lib/docker/types.json docker/data/types.json
```

The other files have the following important differences:

The `docker/data/config.json`:
- Line 11: `"ip": "redis"`

The `docker/data/redis.init.json`:
- Line 47 and 81-87: volume mapping `/d/artof-docker/docker/data/node-red`
- Line 53 and 93: network mode `"NetworkMode": "ilvo_artof_network"`
- Line 62 and 102: exposed ports `ExposedPorts`


Create zip file and register to minio.
```
zip -r artof-docker.zip ilvo
mc cp artof-docker.zip ilvo-minio/tv115-ilvo-robotics
```


## Licence

This project is under the ``ILVO LICENCE``.

```
Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

When the Software, including modifications and extensions, is used for:
- commercial or non-commercial machinery: the ILVO logo has to be clearly
   visible on the machine or on any promotion material which may be used in any
   agricultural fair or conference, in a way it is clear that ILVO contributed
   to the development of the software for the machine.
- a scientific or vulgarising publication: a reference to ILVO must be made as
   well as to the website of the living lab Agrifood Technology of ILVO:
   https://www.agrifoodtechnology.be

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```