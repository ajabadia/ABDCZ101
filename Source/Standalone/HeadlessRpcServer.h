#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include <thread>
#include <memory>
#include <atomic>

// Forward declaration of httplib::Server to avoid including httplib.h here
namespace httplib { class Server; }

class HeadlessRpcServer
{
public:
    HeadlessRpcServer(CZ101AudioProcessor& processor, int port = 8080);
    ~HeadlessRpcServer();

    void start();
    void stop();

private:
    void setupRoutes();

    CZ101AudioProcessor& m_processor;
    int m_port;
    std::unique_ptr<httplib::Server> m_server;
    std::unique_ptr<std::thread> m_serverThread;
    std::atomic<bool> m_running { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeadlessRpcServer)
};
