# Define what an AUX row records

Parent: ../map.md
Label: wayfinder:grilling
Type: grilling
Status: resolved
Assignee: Codex
Blocked by: none

## Question

What does an AUX row represent, and which fields does it expose? Decide whether AUX support documents bus purpose, records algorithm endpoints, or changes actual routing. Decide whether Description and Group suffice or physical cable fields remain useful. Establish AUX numbering and terminology against the device before finalizing labels.

The current plugin records physical connections and does not modify bus buffers. The owner has agreed that any of 44 AUX buses can be selected, with only added rows shown. Field semantics remain open.

## Answer

The owner chose "Bus description and Group" in the live conversation on 2026-10-15. AUX rows describe what an internal bus carries. They expose Description and Group, with no cable colour or cable tag. This adds documentation to the existing plugin rather than signal routing.

Use AUX 1 through AUX 44 as user-facing identities. NT Helper already formats firmware bus 21 as Aux 1 and bus 64 as Aux 44 in lib/models/device_io_profile.dart, formatBusLabel. The firmware bus offset is not an AUX row's displayed number.

The owner previously chose any of the 44 AUX buses, with only explicitly added rows shown. Description and Group follow existing editable-text constraints unless a later compatibility decision requires otherwise.
