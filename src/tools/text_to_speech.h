#pragma once
#include "tool.h"
#include <string>

// Tool chuyển văn bản thành file âm thanh (Text-to-Speech) qua ElevenLabs API.
class TextToSpeechTool : public Tool {
public:
    TextToSpeechTool();
    ~TextToSpeechTool() override = default;

    std::string getName() const override { return "text_to_speech"; }
    std::string getDescription() const override {
        return "Chuyen van ban thanh file am thanh (mp3) bang ElevenLabs API. "
               "Tham so: JSON dang {\"text\": \"...\", \"voice_id\": \"...\", \"output_file\": \"...\"} "
               "hoac chi can truyen thang van ban thuan can doc.";
    }
    std::string execute(const std::string& arguments) override;

private:
    std::string apiUrl_;
    std::string apiKey_;
    std::string defaultVoiceId_;

    std::string loadApiKey() const;
    std::string jsonEscape(const std::string& input) const;

    struct ParsedArgs {
        std::string text;
        std::string voiceId;
        std::string outputFile;
    };
    bool parseArguments(const std::string& arguments, ParsedArgs& out) const;

    bool callTtsApi(const std::string& text,
                     const std::string& voiceId,
                     const std::string& outputFile,
                     std::string& errorMessage) const;
};