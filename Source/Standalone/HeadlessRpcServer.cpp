#include "HeadlessRpcServer.h"

#if JUCE_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include "../Utils/httplib.h"
#include <juce_core/juce_core.h>

HeadlessRpcServer::HeadlessRpcServer(CZ101AudioProcessor& processor, int port)
    : m_processor(processor), m_port(port)
{
    m_server = std::make_unique<httplib::Server>();
    setupRoutes();
}

HeadlessRpcServer::~HeadlessRpcServer()
{
    stop();
}

void HeadlessRpcServer::start()
{
    if (m_running.load()) return;
    
    m_running.store(true);
    
    m_serverThread = std::make_unique<std::thread>([this]() {
        juce::Logger::writeToLog("Headless RPC Server starting on port " + juce::String(m_port));
        m_server->listen("0.0.0.0", m_port);
        juce::Logger::writeToLog("Headless RPC Server stopped.");
    });
}

void HeadlessRpcServer::stop()
{
    if (!m_running.load()) return;
    
    m_server->stop();
    if (m_serverThread && m_serverThread->joinable())
        m_serverThread->join();
        
    m_running.store(false);
}

void HeadlessRpcServer::setupRoutes()
{
    m_server->Post("/rpc", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        juce::var parsedJson;
        juce::Result result = juce::JSON::parse(req.body, parsedJson);
        
        if (!result.wasOk())
        {
            res.status = 400;
            res.set_content("{\"error\": \"Invalid JSON\"}", "application/json");
            return;
        }
        
        if (!parsedJson.isObject())
        {
            res.status = 400;
            res.set_content("{\"error\": \"JSON must be an object\"}", "application/json");
            return;
        }
        
        auto* obj = parsedJson.getDynamicObject();
        juce::String method = obj->getProperty("method").toString();
        juce::var params = obj->getProperty("params");
        juce::var id = obj->getProperty("id");
        
        juce::var resultValue;
        bool hasError = false;
        juce::String errorMsg;
        
        if (method == "setParameterValue")
        {
            if (params.isArray() && params.getArray()->size() >= 2)
            {
                juce::String paramID = params.getArray()->getReference(0).toString();
                float value = static_cast<float>(params.getArray()->getReference(1));
                
                if (auto* param = m_processor.getParameters().getAPVTS().getParameter(paramID))
                {
                    // Parameters are normalized [0.0, 1.0] from WebUI/RPC
                    param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
                    resultValue = juce::var();
                }
                else
                {
                    hasError = true;
                    errorMsg = "Parameter not found: " + paramID;
                }
            }
            else
            {
                hasError = true;
                errorMsg = "Invalid params for setParameterValue";
            }
        }
        else if (method == "getParameterValue")
        {
            if (params.isArray() && params.getArray()->size() >= 1)
            {
                juce::String paramID = params.getArray()->getReference(0).toString();
                if (auto* param = m_processor.getParameters().getAPVTS().getParameter(paramID))
                {
                    resultValue = param->getValue(); // Normalized 0..1
                }
                else
                {
                    hasError = true;
                    errorMsg = "Parameter not found: " + paramID;
                }
            }
            else
            {
                hasError = true;
                errorMsg = "Invalid params for getParameterValue";
            }
        }
        else if (method == "loadPreset")
        {
            if (params.isArray() && params.getArray()->size() >= 1)
            {
                int idx = static_cast<int>(params.getArray()->getReference(0));
                m_processor.getPresetManager().loadPreset(idx);
                resultValue = juce::var();
            }
            else
            {
                hasError = true;
                errorMsg = "Invalid params for loadPreset";
            }
        }
        else if (method == "sendMidiMessage")
        {
            if (params.isArray() && params.getArray()->size() >= 1)
            {
                auto& arrayVar = params.getArray()->getReference(0);
                if (arrayVar.isArray() && arrayVar.getArray()->size() >= 3)
                {
                    uint8_t b0 = static_cast<uint8_t>(int(arrayVar.getArray()->getReference(0)));
                    uint8_t b1 = static_cast<uint8_t>(int(arrayVar.getArray()->getReference(1)));
                    uint8_t b2 = static_cast<uint8_t>(int(arrayVar.getArray()->getReference(2)));
                    
                    juce::MidiMessage msg(b0, b1, b2);
                    m_processor.addUIMidiMessage(msg);
                    resultValue = juce::var();
                }
                else
                {
                    hasError = true;
                    errorMsg = "Invalid params for sendMidiMessage";
                }
            }
        }
        else
        {
            hasError = true;
            errorMsg = "Method not found: " + method;
        }
        
        juce::DynamicObject* responseObj = new juce::DynamicObject();
        responseObj->setProperty("id", id);
        
        if (hasError)
        {
            juce::DynamicObject* errorObj = new juce::DynamicObject();
            errorObj->setProperty("message", errorMsg);
            responseObj->setProperty("error", juce::var(errorObj));
        }
        else
        {
            responseObj->setProperty("result", resultValue);
        }
        
        juce::String responseJson = juce::JSON::toString(juce::var(responseObj));
        res.set_content(responseJson.toStdString(), "application/json");
    });
    
    m_server->Options("/rpc", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });
}
