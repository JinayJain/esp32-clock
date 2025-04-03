#include "llm.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

LLM::LLM(const String &apiKey, const String &baseUrl) : _apiKey(apiKey), _baseUrl(baseUrl)
{
}

void createLLMResponseFilter(JsonDocument &filter)
{
    JsonObject choices0 = filter["choices"][0].to<JsonObject>();
    JsonObject message = choices0.createNestedObject("message");
    message["content"] = true;
}

ChatMessage LLM::chatCompletion(const std::vector<ChatMessage> &messages, const LLMCompletionOptions &options)
{
    HTTPClient http;
    String endpoint = _baseUrl + "/chat/completions";

    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + _apiKey);

    DynamicJsonDocument requestDoc(4096);

    requestDoc["model"] = options.model;
    requestDoc["temperature"] = options.temperature;
    requestDoc["max_tokens"] = options.maxTokens;

    JsonArray messagesArray = requestDoc.createNestedArray("messages");
    for (const auto &message : messages)
    {
        JsonObject msgObj = messagesArray.createNestedObject();
        msgObj["role"] = message.role;
        msgObj["content"] = message.content;
    }

    String payload;
    serializeJson(requestDoc, payload);

    int httpResponseCode = http.POST(payload);
    String response = "";

    if (httpResponseCode > 0)
    {
        response = http.getString();

        StaticJsonDocument<128> filter;
        createLLMResponseFilter(filter);

        DynamicJsonDocument responseDoc(4096);
        DeserializationError error = deserializeJson(responseDoc, response, DeserializationOption::Filter(filter));

        if (!error && responseDoc.containsKey("choices") && responseDoc["choices"].size() > 0)
        {
            if (responseDoc["choices"][0].containsKey("message") &&
                responseDoc["choices"][0]["message"].containsKey("content"))
            {
                response = responseDoc["choices"][0]["message"]["content"].as<String>();
            }
        }
    }
    else
    {
        response = "Error on HTTP request: " + String(httpResponseCode);
    }

    http.end();
    return ChatMessage{"assistant", response};
}
