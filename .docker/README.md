## Groups

### Ubuntu 20.04 package

`Dockerfile-ubuntu-20.04-package` relies on `Dockerfile-ubuntu-20.04-build`

To build, from the repo root

1. Build a docker image that contains all the dependencies necessary to build Chatterino on Ubuntu 20.04  
   `docker buildx build -t chatterino-ubuntu-20.04-base -f .docker/Dockerfile-ubuntu-20.04-base .`
1. Build a docker image that contains all the build artifacts and source from building Chatterino on Ubuntu 20.04  
   `docker buildx build -t chatterino-ubuntu-20.04-build -f .docker/Dockerfile-ubuntu-20.04-build .`
1. Build a docker image that uses the above-built image & packages it into a .deb file  
   `docker buildx build -t chatterino-ubuntu-20.04-package -f .docker/Dockerfile-ubuntu-20.04-package .`

To extract the final package, you can run the following command:  
`docker run -v $PWD:/opt/mount --rm -it chatterino-ubuntu-20.04-package bash -c "cp /src/build/Chatterino-x86_64.deb /opt/mount/"`

### Ubuntu 22.04 package

`Dockerfile-ubuntu-22.04-package` relies on `Dockerfile-ubuntu-22.04-build`

To build, from the repo root

1. Build a docker image that contains all the dependencies necessary to build Chatterino on Ubuntu 22.04  
   `docker buildx build -t chatterino-ubuntu-22.04-base -f .docker/Dockerfile-ubuntu-22.04-base .`
1. Build a docker image that contains all the build artifacts and source from building Chatterino on Ubuntu 22.04  
   `docker buildx build -t chatterino-ubuntu-22.04-build -f .docker/Dockerfile-ubuntu-22.04-build .`
1. Build a docker image that uses the above-built image & packages it into a .deb file  
   `docker buildx build -t chatterino-ubuntu-22.04-package -f .docker/Dockerfile-ubuntu-22.04-package .`

To extract the final package, you can run the following command:  
`docker run -v $PWD:/opt/mount --rm -it chatterino-ubuntu-22.04-package bash -c "cp /src/build/Chatterino-x86_64.deb /opt/mount/"`

NOTE: The AppImage from Ubuntu 22.04 is broken. Approach with caution

#### Testing

1. Build a docker image builds the Chatterino tests  
   `docker buildx build -t chatterino-ubuntu-22.04-test -f .docker/Dockerfile-ubuntu-22.04-test .`
1. Run the tests  
   `docker run --rm --network=host chatterino-ubuntu-22.04-test`
