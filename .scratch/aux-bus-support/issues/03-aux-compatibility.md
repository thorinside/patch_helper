# Choose stable AUX identities and compatible storage

Parent: ../map.md
Label: wayfinder:grilling
Type: grilling
Status: open
Assignee: none
Blocked by: 01, 02

## Question

Given the agreed AUX fields and native behavior, how should AUX identities, preset serialization, MIDI transport, and Helper capability negotiation change while preserving existing presets and parameter mappings? Choose the behavior of older clients encountering the new format.

src/patch_map.h assigns native sockets 0 through 19 and expander records from 20 onward. New maps allow eight banks, while legacy maps retain thirteen. src/patch_protocol.h uses one MIDI data byte for row addresses. Adding 44 AUX records to the eight-bank model totals 128 records; legacy thirteen-bank maps exceed that. readMapBody currently accepts only versions 1 and 2 with exact record counts. Record the selected design and evidence in the resolution.
