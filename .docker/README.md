## Groups

### Ubuntu 20.04 package

`Dockerfile-ubuntu-20.04-package` relies on `Dockerfile-ubuntu-20.04-build`

#### Build steps

From the repo root

1. Build a docker image that contains all the build artifacts and source from building Chatterino on Ubuntu 20.04  
   `docker build -t chatterino-ubuntu-20.04-build -f .docker/Dockerfile-ubuntu-20.04-build .`  
1. Build a docker image that uses the above-built image & packages it into a .deb file  
   `docker build -t chatterino-ubuntu-20.04-package -f .docker/Dockerfile-ubuntu-20.04-package .`
