#include "llm.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

LLM::LLM(const String &apiKey, const String &baseUrl) : _apiKey(apiKey), _baseUrl(baseUrl)
{
}

ChatMessage LLM::chatCompletion(const std::vector<ChatMessage> &messages, const LLMCompletionOptions &options)
{
    HTTPClient http;
    String endpoint = _baseUrl + "/chat/completions";

    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + _apiKey);

    // Prepare JSON payload
    DynamicJsonDocument doc(16384); // Adjust size based on expected payload size

    doc["model"] = options.model;
    doc["temperature"] = options.temperature;
    doc["max_tokens"] = options.maxTokens;

    JsonArray messagesArray = doc.createNestedArray("messages");
    for (const auto &message : messages)
    {
        JsonObject msgObj = messagesArray.createNestedObject();
        msgObj["role"] = message.role;
        msgObj["content"] = message.content;
    }

    String payload;
    serializeJson(doc, payload);

    // Make the HTTP POST request
    int httpResponseCode = http.POST(payload);
    String response = "";

    if (httpResponseCode > 0)
    {
        response = http.getString();

        // Parse response to extract completion
        DynamicJsonDocument responseDoc(16384);
        deserializeJson(responseDoc, response);

        if (responseDoc.containsKey("choices") && responseDoc["choices"].size() > 0)
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
