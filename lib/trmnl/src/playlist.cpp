#include "playlist.h"

#include <trmnl_log.h>

PlaylistManifest parsePlaylist_manifest(const JsonDocument &doc) {
  PlaylistManifest manifest;

  JsonVariantConst playlist = doc["playlist"];
  if (!playlist.is<JsonObjectConst>()) {
    // Absent (or not an object): stock behaviour.
    return manifest;
  }

  manifest.present = true;
  manifest.unchanged = playlist["unchanged"] | false;
  manifest.checksum = playlist["checksum"] | "";

  if (manifest.unchanged) {
    // The server acked our checksum; no entries were sent and none are expected.
    return manifest;
  }

  for (JsonObjectConst item : playlist["entries"].as<JsonArrayConst>()) {
    PlaylistEntry entry;
    entry.filename = item["filename"] | "";
    entry.url = item["url"] | "";
    entry.sha256 = item["sha256"] | "";

    if (entry.filename.length() == 0 || entry.url.length() == 0) {
      Log_warn("Playlist: dropping entry without filename or URL");
      continue;
    }
    manifest.entries.push_back(entry);
  }

  if (!manifest.entries.empty() && manifest.checksum.length() == 0) {
    Log_warn("Playlist: manifest has entries but no checksum, ignoring");
    return PlaylistManifest();
  }

  return manifest;
}
