#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

// One entry of the "playlist" manifest served by GET /api/display. Ordered as sent: the
// rotation walks entries in this order.
struct PlaylistEntry {
  String filename; // PNG name, used as the SPIFFS cache key
  String url;      // where to fetch the image
  String sha256;   // expected digest of the image bytes; may be empty
};

// Device-side view of the optional "playlist" member of the /api/display response.
//
//   absent              -> present == false; the device keeps stock behaviour
//   {"unchanged": true} -> present && unchanged; the server acked our PLAYLIST_CHECKSUM
//   {"entries": []}     -> present, no entries; deactivate rotation
//   {"entries": [..]}   -> present, has_entries(); rotate over the entries
struct PlaylistManifest {
  bool present = false;
  bool unchanged = false;
  String checksum;
  std::vector<PlaylistEntry> entries;

  bool has_entries() const { return present && !unchanged && !entries.empty(); }
};

// Read the optional "playlist" member out of a /api/display JSON document. Entries without a
// filename or URL are dropped; a manifest left with entries but no checksum is rejected and
// reported as absent.
PlaylistManifest parsePlaylist_manifest(const JsonDocument &doc);
