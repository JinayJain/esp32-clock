#pragma once

#include <Arduino.h>
#include <vector>

struct ChatMessage
{
    String role;
    String content;
};

struct LLMCompletionOptions
{
    String model = "gpt-4o-mini";
    float temperature = 0.7;
    int maxTokens = 1024;
};

class LLM
{
public:
    LLM(const String &apiKey, const String &baseUrl = "https://api.openai.com/v1");

    String chatCompletion(const std::vector<ChatMessage> &messages, const LLMCompletionOptions &options = {});

private:
    String _apiKey;
    String _baseUrl;
};