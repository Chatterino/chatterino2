#include "Recovery.hpp"

#include <handler/handler_main.h>
#include <tools/tool_support.h>

#if BUILDFLAG(IS_WIN)
#    include <Windows.h>
#endif

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
int actualMain(int argc, char *argv[])
{
    // We're tapping into the crash handling by registering a data source.
    // Our source doesn't actually provide any data, but it records the crash.
    // Once the crash is recorded, we know that one happened and can attempt to restart
    // the host application.
    crashpad::UserStreamDataSources sources;
    sources.emplace_back(std::make_unique<CrashRecoverer>());

    auto ret = crashpad::HandlerMain(argc, argv, &sources);

    if (ret == 0)
    {
        auto *recoverer = dynamic_cast<CrashRecoverer *>(sources.front().get());
        if (recoverer != nullptr)
        {
            recoverer->attemptRecovery();
        }
    }
    return ret;
}

}  // namespace

// The following is adapted from handler/main.cc
#if BUILDFLAG(IS_POSIX)

int main(int argc, char *argv[])
{
    static_assert(false, "Posix is not supported by the handler");
}

#elif BUILDFLAG(IS_WIN)

// The default entry point for /subsystem:windows.
int APIENTRY wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/,
                      PWSTR /*pCmdLine*/, int /*nCmdShow*/)
{
    return crashpad::ToolSupport::Wmain(__argc, __wargv, actualMain);
}

#endif
