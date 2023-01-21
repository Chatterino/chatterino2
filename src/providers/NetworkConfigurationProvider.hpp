#pragma once

namespace chatterino {

class Env;

/** This class manipulates the global network configuration (e.g. proxies). */
class NetworkConfigurationProvider
{
public:
    /** This class should never be instantiated. */
    NetworkConfigurationProvider() = delete;

    /**
     * Applies the configuration requested from the environment variables.
     *
     * Currently a proxy is applied if configured.
     */
    static void applyFromEnv(const Env &env);
};

}  // namespace chatterino
