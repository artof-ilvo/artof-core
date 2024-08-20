FROM gcc:latest

# Install necessary dependencies
RUN apt-get update && apt-get install -y cmake \
                                         curl\
                                         iputils-ping \
                                         libboost-filesystem-dev libboost-system-dev libboost-thread-dev


# Set the working directory in the container
WORKDIR /app

# Copy the source files
COPY src/ /app/src/
COPY include/ /app/include/
COPY CMakeLists.txt /app/
# Copy the libraries
COPY dependencies/x86_64/shared/libsnap7.so /usr/lib/ilvo/libsnap7.so
COPY dependencies/x86_64/static/libshp.a /app/dependencies/x86_64/static/libshp.a

# Build the project
WORKDIR /app/build
RUN NO_ASAN=1 cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_FOLDER="/usr/bin" .. && make -j4 install

# Command to run your application
CMD ilvo-system-manager