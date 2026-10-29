#include "playlist.h"

#include <trmnl_log.h>

namespace {

  // A manifest is applied as a whole or not at all: a partial rotation would show the user
  // screens the server never ordered. So every entry must name its file and its URL.
  bool readEntry(JsonObjectConst item, PlaylistEntry &entry) {
    entry.filename = item["filename"] | "";
    entry.url = item["url"] | "";
    entry.sha256 = item["sha256"] | "";
    return entry.filename.length() > 0 && entry.url.length() > 0;
  }

} // namespace

PlaylistManifest parsePlaylist_manifest(const JsonDocument &doc) {
  PlaylistManifest manifest;

  JsonVariantConst playlist = doc["playlist"];
  // isNull() also covers an unbound member, so an omitted "playlist" and an explicit null
  // both mean absent: stock behaviour for a server that does not implement the contract.
  if (playlist.isNull()) {
    return manifest;
  }
  if (!playlist.is<JsonObjectConst>()) {
    Log_warn("Playlist: not an object, ignoring");
    return PlaylistManifest();
  }

  manifest.present = true;
  manifest.unchanged = playlist["unchanged"] | false;
  manifest.checksum = playlist["checksum"] | "";

  if (manifest.unchanged) {
    // The server acked our checksum: no entries were sent and none are expected.
    return manifest;
  }

  JsonVariantConst items = playlist["entries"];
  if (!items.is<JsonArrayConst>()) {
    Log_warn("Playlist: neither unchanged nor an entry array, ignoring");
    return PlaylistManifest();
  }

  unsigned index = 0;
  for (JsonVariantConst item : items.as<JsonArrayConst>()) {
    if (!item.is<JsonObjectConst>()) {
      Log_warn("Playlist: entry %u is not an object, rejecting manifest", index);
      return PlaylistManifest();
    }

    PlaylistEntry entry;
    if (!readEntry(item.as<JsonObjectConst>(), entry)) {
      Log_warn("Playlist: entry %u has no filename or URL, rejecting manifest", index);
      return PlaylistManifest();
    }
    manifest.entries.push_back(entry);
    ++index;
  }

  // The checksum drives the "unchanged" ack, so any manifest worth storing must carry one.
  // The documented deactivation form, {"entries": []}, is allowed without it.
  if (!manifest.entries.empty() && manifest.checksum.length() == 0) {
    Log_warn("Playlist: %u entries but no checksum, rejecting manifest", (unsigned)manifest.entries.size());
    return PlaylistManifest();
  }

  return manifest;
}
