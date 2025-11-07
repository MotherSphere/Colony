#include "programs/archive/vault/Entry.hpp"

#include <chrono>
#include <cstdint>

namespace colony::programs::archive::vault
{
namespace
{
using Clock = std::chrono::system_clock;

std::int64_t to_epoch_millis(Clock::time_point time)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

Clock::time_point from_epoch_millis(std::int64_t millis)
{
    return Clock::time_point{std::chrono::milliseconds{millis}};
}
}

void to_json(nlohmann::json& json, const Field& field)
{
    json = nlohmann::json{{"name", field.name}, {"value", field.value}, {"concealed", field.concealed}};
}

void from_json(const nlohmann::json& json, Field& field)
{
    json.at("name").get_to(field.name);
    json.at("value").get_to(field.value);
    json.at("concealed").get_to(field.concealed);
}

void to_json(nlohmann::json& json, const Attachment& attachment)
{
    json = nlohmann::json{
        {"id", attachment.id},
        {"name", attachment.name},
        {"mime_type", attachment.mime_type},
        {"created_at", to_epoch_millis(attachment.created_at)},
        {"data", nlohmann::json::binary(attachment.data)}
    };
}

void from_json(const nlohmann::json& json, Attachment& attachment)
{
    json.at("id").get_to(attachment.id);
    json.at("name").get_to(attachment.name);
    json.at("mime_type").get_to(attachment.mime_type);
    attachment.created_at = from_epoch_millis(json.at("created_at").get<std::int64_t>());

    if (const auto* binary = json.at("data").get_ptr<const nlohmann::json::binary_t*>(); binary)
    {
        attachment.data.assign(binary->begin(), binary->end());
    }
    else
    {
        attachment.data = json.at("data").get<std::vector<std::uint8_t>>();
    }
}

void to_json(nlohmann::json& json, const HistoryEvent& event)
{
    json = nlohmann::json{{"summary", event.summary}, {"timestamp", to_epoch_millis(event.timestamp)}};
}

void from_json(const nlohmann::json& json, HistoryEvent& event)
{
    json.at("summary").get_to(event.summary);
    event.timestamp = from_epoch_millis(json.at("timestamp").get<std::int64_t>());
}

void to_json(nlohmann::json& json, const Entry& entry)
{
    json = nlohmann::json{
        {"id", entry.id},
        {"title", entry.title},
        {"fields", entry.fields},
        {"attachments", entry.attachments},
        {"tags", entry.tags},
        {"history", entry.history},
        {"created_at", to_epoch_millis(entry.created_at)},
        {"updated_at", to_epoch_millis(entry.updated_at)}
    };
}

void from_json(const nlohmann::json& json, Entry& entry)
{
    json.at("id").get_to(entry.id);
    json.at("title").get_to(entry.title);
    json.at("fields").get_to(entry.fields);
    json.at("attachments").get_to(entry.attachments);
    json.at("tags").get_to(entry.tags);
    json.at("history").get_to(entry.history);
    entry.created_at = from_epoch_millis(json.at("created_at").get<std::int64_t>());
    entry.updated_at = from_epoch_millis(json.at("updated_at").get<std::int64_t>());
}

} // namespace colony::programs::archive::vault
