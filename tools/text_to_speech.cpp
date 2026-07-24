#include "text_to_speech.h"
#include <curl/curl.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

TextToSpeechTool::TextToSpeechTool() {
    apiUrl_ = "https://api.elevenlabs.io/v1/text-to-speech/";
    // Giong rieng tao bang Voice Design tren dashboard (khong phai giong thu vien),
    // dung duoc qua API voi tai khoan free tier.
    defaultVoiceId_ = "FYFy6dC6e6cIDamP8Uvn";
    apiKey_ = loadApiKey();
}

std::string TextToSpeechTool::loadApiKey() const {
    const char* key = std::getenv("TTS_API_KEY");
    return key ? std::string(key) : std::string();
}

std::string TextToSpeechTool::jsonEscape(const std::string& input) const {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

bool TextToSpeechTool::parseArguments(const std::string& arguments, ParsedArgs& out) const {
    auto extract = [&](const std::string& key) -> std::string {
        std::string pattern = "\"" + key + "\"";
        size_t keyPos = arguments.find(pattern);
        if (keyPos == std::string::npos) return "";
        size_t colon = arguments.find(':', keyPos + pattern.size());
        if (colon == std::string::npos) return "";
        size_t firstQuote = arguments.find('"', colon);
        if (firstQuote == std::string::npos) return "";
        size_t pos = firstQuote + 1;
        std::string value;
        while (pos < arguments.size() && arguments[pos] != '"') {
            if (arguments[pos] == '\\' && pos + 1 < arguments.size()) {
                value += arguments[pos + 1];
                pos += 2;
            } else {
                value += arguments[pos];
                pos += 1;
            }
        }
        return value;
    };

    size_t firstNonSpace = arguments.find_first_not_of(" \t\r\n");
    if (firstNonSpace == std::string::npos || arguments[firstNonSpace] != '{') {
        return false;
    }

    out.text = extract("text");
    if (out.text.empty()) return false;

    out.voiceId = extract("voice_id");
    out.outputFile = extract("output_file");
    return true;
}

static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* outFile = static_cast<std::ofstream*>(userp);
    size_t totalSize = size * nmemb;
    outFile->write(static_cast<char*>(contents), static_cast<std::streamsize>(totalSize));
    return totalSize;
}

bool TextToSpeechTool::callTtsApi(const std::string& text,
                                   const std::string& voiceId,
                                   const std::string& outputFile,
                                   std::string& errorMessage) const {
    if (apiKey_.empty()) {
        errorMessage = "Thieu API key. Hay dat bien moi truong TTS_API_KEY.";
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        errorMessage = "Khong the khoi tao CURL.";
        return false;
    }

    std::string fullUrl = apiUrl_ + defaultVoiceId_;

    std::ostringstream body;
    body << "{"
         << "\"text\":\"" << jsonEscape(text) << "\","
         << "\"model_id\":\"eleven_multilingual_v2\""
         << "}";
    std::string bodyStr = body.str();

    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile.is_open()) {
        errorMessage = "Khong the tao file output: " + outputFile;
        curl_easy_cleanup(curl);
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("xi-api-key: " + apiKey_).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bodyStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(bodyStr.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    outFile.close();
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        errorMessage = std::string("Loi CURL: ") + curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return false;
    }

    if (httpCode < 200 || httpCode >= 300) {
        std::ifstream errIn(outputFile, std::ios::binary);
        std::ostringstream errStream;
        errStream << errIn.rdbuf();
        errIn.close();
        std::remove(outputFile.c_str());

        errorMessage = "API tra ve loi HTTP " + std::to_string(httpCode) + ": " + errStream.str();
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);
    return true;
}

std::string TextToSpeechTool::execute(const std::string& arguments) {
    ParsedArgs parsed;
    std::string text;
    std::string voiceId;
    std::string outputFile = "output.mp3";

    if (parseArguments(arguments, parsed)) {
        text = parsed.text;
        if (!parsed.outputFile.empty()) {
            outputFile = parsed.outputFile;
        }
        // Tool luon su dung defaultVoiceId_, bo qua voice_id do LLM cung cap.
        if (!parsed.voiceId.empty()) {
            std::cout << "[TTS Tool] Bo qua voice_id do Agent cung cap: "
                  << parsed.voiceId << std::endl;
    }
    } else {
    text = arguments;

    // Neu Agent truyen chuoi dang "hello"
    // thi bo dau ngoac kep o hai dau.
    if (text.size() >= 2 &&
        text.front() == '"' &&
        text.back() == '"') {
        text = text.substr(1, text.size() - 2);
    }
}

    if (text.empty()) {
        return "[TTS Tool] Loi: khong co van ban nao de chuyen doi.";
    }

    std::cout << "[TTS Tool] Dang xu ly van ban: " << text << std::endl;

    std::string errorMessage;
    bool ok = callTtsApi(text, voiceId, outputFile, errorMessage);

    if (!ok) {
        std::cerr << "[TTS Tool] That bai: " << errorMessage << std::endl;
        return "Chuyen doi that bai: " + errorMessage;
    }

    return "Da chuyen doi thanh cong van ban thanh am thanh va luu tai " + outputFile;
}