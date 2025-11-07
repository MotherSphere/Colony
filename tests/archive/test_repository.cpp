#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "programs/archive/vault/Repository.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <string>
#include <string_view>

using namespace colony::programs::archive::vault;
using namespace std::literals;

namespace
{
Entry make_entry(std::string id, std::string title)
{
    Entry entry;
    entry.id = std::move(id);
    entry.title = std::move(title);
    entry.created_at = entry.updated_at = std::chrono::system_clock::now();
    entry.tags = {"finance", "personal"};
    entry.fields = {Field{"username", "alice", false}, Field{"password", "secret", true}};
    entry.history = {HistoryEvent{"created", entry.created_at}};
    Attachment attachment;
    attachment.id = "att-1";
    attachment.name = "notes.txt";
    attachment.mime_type = "text/plain";
    attachment.created_at = entry.created_at;
    attachment.data = {0x61, 0x62, 0x63};
    entry.attachments = {std::move(attachment)};
    return entry;
}
}

TEST_CASE("Repository round trips entries through encryption")
{
    Repository repo;
    repo.metadata().version = 1;
    repo.metadata().tags = {"vault"};
    repo.entries() = {make_entry("entry-1", "Bank"), make_entry("entry-2", "Mail")};
    repo.touch();

    const auto blob = repo.seal("password"sv);
    const auto restored = Repository::unseal(blob, "password"sv);

    CHECK(restored.metadata().repository_id == repo.metadata().repository_id);
    CHECK(restored.entries().size() == repo.entries().size());
    CHECK(restored.entries()[0].title == "Bank");
    CHECK(restored.entries()[0].attachments.front().data.size() == 3);
}

TEST_CASE("Repository detects tampering")
{
    Repository repo;
    repo.entries() = {make_entry("entry", "Item")};
    const auto blob = repo.seal("password"sv);
    auto corrupted = blob;
    corrupted.back() ^= 0xFF;
    CHECK_THROWS([&]() {
        (void)Repository::unseal(corrupted, "password"sv);
    }());
}

TEST_CASE("Repository supports master password rotation")
{
    Repository repo;
    repo.entries() = {make_entry("entry", "Email")};
    const auto blob = repo.seal("old"sv);

    const auto rotated = Repository::reencrypt(blob, "old"sv, "new"sv);
    CHECK_THROWS([&]() {
        (void)Repository::unseal(blob, "new"sv);
    }());
    const auto reopened = Repository::unseal(rotated, "new"sv);
    CHECK(reopened.entries().size() == 1);
    CHECK(reopened.entries()[0].title == "Email");
}
