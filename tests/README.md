To run tests all tests you will need to run the `kennethreitz/httpbin` and `ghcr.io/chatterino/twitch-pubsub-server-test:latest` docker images

For example:

```bash
docker run --network=host --detach ghcr.io/chatterino/twitch-pubsub-server-test:latest
docker run -p 9051:80 --detach kennethreitz/httpbin
```
