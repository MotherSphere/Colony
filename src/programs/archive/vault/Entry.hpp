#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "json.hpp"

namespace colony::programs::archive::vault
{

struct Field
{
    std::string name;
    std::string value;
    bool concealed{false};
};

struct Attachment
{
    std::string id;
    std::string name;
    std::string mime_type;
    std::vector<std::uint8_t> data;
    std::chrono::system_clock::time_point created_at;
};

struct HistoryEvent
{
    std::string summary;
    std::chrono::system_clock::time_point timestamp;
};

struct Entry
{
    std::string id;
    std::string title;
    std::vector<Field> fields;
    std::vector<Attachment> attachments;
    std::vector<std::string> tags;
    std::vector<HistoryEvent> history;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

void to_json(nlohmann::json& json, const Field& field);
void from_json(const nlohmann::json& json, Field& field);

void to_json(nlohmann::json& json, const Attachment& attachment);
void from_json(const nlohmann::json& json, Attachment& attachment);

void to_json(nlohmann::json& json, const HistoryEvent& event);
void from_json(const nlohmann::json& json, HistoryEvent& event);

void to_json(nlohmann::json& json, const Entry& entry);
void from_json(const nlohmann::json& json, Entry& entry);

} // namespace colony::programs::archive::vault
