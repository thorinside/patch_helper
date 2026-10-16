# Choose stable AUX identities and compatible storage

Parent: ../map.md
Label: wayfinder:grilling
Type: grilling
Status: resolved
Assignee: Codex
Blocked by: 01, 02

## Question

Given the agreed AUX fields and native behavior, how should AUX identities, preset serialization, MIDI transport, and Helper capability negotiation change while preserving existing presets and parameter mappings? Choose the behavior of older clients encountering the new format.

src/patch_map.h assigns native sockets 0 through 19 and expander records from 20 onward. New maps allow eight banks, while legacy maps retain thirteen. src/patch_protocol.h uses one MIDI data byte for row addresses. Adding 44 AUX records to the eight-bank model totals 128 records; legacy thirteen-bank maps exceed that. readMapBody currently accepts only versions 1 and 2 with exact record counts. Record the selected design and evidence in the resolution.

## Answer

Keep all physical socket identities and parameter indices unchanged. Store AUX records separately by AUX number 1 through 44, with fixed storage for all 44 possibilities and an explicit present flag. Preset version 3 adds a sparse aux array and includes the existing expanders field. Continue reading versions 1 and 2, including thirteen-bank legacy maps. Serialize maps without AUX in the existing version 1 or 2 shape.

Negotiate AUX support through the existing begin-session command using payload revision 3. Older plugins explicitly reject that payload, so new Helper retries revision 2. Keep older-session replies unchanged. Add separate inventory/read/add/write/remove AUX commands under the same lease and revision checks. Append the selected AUX number to the negotiated watch response. Old clients can still edit physical records without changing AUX records; old binaries cannot load the new preset format and must be updated before using AUX presets.

Append a native AUX selector and read-only Description and Group properties. The custom list includes added AUX rows and shows their descriptive text without cable controls. This stays below the existing parameter ceiling and avoids renumbering saved mappings. Exact protocol layouts belong to docs/live-map-protocol.md and the matching tests.
