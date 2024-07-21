To run all tests you will need to run the `kennethreitz/httpbin` and `ghcr.io/chatterino/twitch-pubsub-server-test:latest` docker images.

For example:

```bash
docker run --network=host --detach ghcr.io/chatterino/twitch-pubsub-server-test:latest
docker run -p 9051:80 --detach kennethreitz/httpbin
```

If you're unable to use docker, you can use [httpbox](https://github.com/kevinastone/httpbox) (`httpbox --port 9051`) and [Chatterino/twitch-pubsub-server-test](https://github.com/Chatterino/twitch-pubsub-server-test/releases/latest) manually.
