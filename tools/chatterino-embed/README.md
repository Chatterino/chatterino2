# chatterino-embed

Embed Chatterino into another Qt application.

## Use

chatterino-embed is a dynamic library. Applications link to it when they're
built - notably, it's not a plugin that is dynamically loaded at runtime.

The process for compiling an app with Chatterino is as follows:

1. Build `chatterino-embed` with the desired Qt version. This needs to match the
   version of the app.
   1. Configure Chatterino with `BUILD_EMBED=On`.
   2. Build the `chatterino-embed` target.
   3. Install the `chatterino-embed` component.
2. Configure your app to find the locally installed `chatterino-embed`.
3. Use `target_link_libraries(YourApp PRIVATE chatterino-embed)`
4. Build your app.
5. Install your app along with `chatterino-embed`.

Take a look at the example app on how to use the library from C++.
