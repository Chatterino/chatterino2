# chatterino-embed

Embed Chatterino into another Qt application.

## Structure

```mermaid
flowchart TD
    YourApp -->|QPluginLoader| E(chatterino-embed.dll/.dylib/.so)
    E -->|STATIC| chatterino
```
