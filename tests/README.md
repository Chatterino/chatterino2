To run tests, you'll first need to enable the test flag in cmake. You can do this by adding the `-DBUILD_TESTS=On` flag to your cmake setup call. (e.g. `mkdir build && cd build && cmake -DBUILD_TESTS=On ..`)

After you've built Chatterino, you can now run the tests with `ctest` or by manually running the newly created `chatterino-test` executable

# Pre-requisites to running tests

- Download & run [httpbox](https://github.com/Chatterino/httpbox/releases/latest)  
  We expect this to be listening to port 9051, you can launch it like this:  
  `httpbox --port 9051`

- Download & run [twitch-pubsub-server-test](https://github.com/Chatterino/twitch-pubsub-server-test/releases/latest)  
  We expect this to be listening to port 9050, you can launch it like this:
  `twitch-pubsub-server-test 127.0.0.1:9050`

# Modifying message building

If you make a change that has to do with message building, there's a big chance that some of the snapshot tests will fail.
If this happens, you might want to:

1. Set the `UPDATE_SNAPSHOTS` boolean at the top of the [IrcMessageHandler.cpp](./src/IrcMessageHandler.cpp) test file to `true`
1. Re-run the tests (this will update the snapshots to match your new reality)
1. Reset `UPDATE_SNAPSHOTS` to `false` again
1. Then run the tests a final time.

Once this is done, you should take a look at the changes made to the snapshot json files to ensure that it looks correct.
