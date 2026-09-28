# Native text-property investigation — 2026-09-28

Native field parity remains required and unfinished. This investigation does
not replace the NT's existing text editor with a custom keyboard, change Patch
Helper's installed object, or claim that string transport alone completes it.

## Environment and source

- Hardware firmware: v1.19.0beta, Sep 16 2026 11:56:15.
- Public SDK: API 13, commit `9aeda6d41484815b80416b903a564510da6026cd`.
- The manual defines `0x53` (Set string parameter value) and `0x50` (Request
  parameter value string). Parameter numbers are three MIDI-safe bytes, with
  the upper two, middle seven, and lower seven bits of the 16-bit number.
- The official `examples/gain.cpp` uses `kNT_unitHasStrings` and
  `parameterString` to format numeric gain. It does not demonstrate native
  arbitrary-text input or receive string writes.
- The connected community MCP at <https://nt-mcp.fly.dev/mcp> returned that
  same example. Its other inspected official string examples format sample
  filenames and microtuning filenames.

## Isolated diagnostic

`tools/native_text_probe.cpp` defines GUID `ThTp`, separate from `ThPh`.
It requests all instance memory upfront, uses placement construction, is PIC,
and does not process audio. It is excluded from the default hardware target and
release package. The two string definitions deliberately share one diagnostic
buffer so their declarations can be compared.

```sh
make native-text-probe build/arm_startup.o
python tools/arm_startup_check.py --native-text-probe \
  build/native_text_probe.o build/arm_startup.o
```

The ARM check passed at two load addresses with zeroed and poisoned SRAM,
strict alignment, and bounds checks against the requested allocation. Object
inspection found no GOT dependency or heap allocator imports. These checks do
not emulate the firmware's property conversion or native text editor.

Before hardware testing, save a separate recoverable copy of the active preset
and verify its full custom state. Use a disposable preset with a one-channel
Mixer Mono (`mix1`, specifications 1, 0) in slot 0 and `ThTp` in slot 1. Upload
with the existing official SD transfer, rescan, load the GUID, then add it.
Unload the diagnostic before replacing its object.

## Observed results

All slot and parameter numbers below are zero-based and include the common
Bypass parameter. Bypass 0 was explicitly set and read back before the final
tests. Both bypass values were tested during diagnosis.

| Field | Declaration | Reported type from `0x43` | `0x53` then `0x50` |
|---|---|---|---|
| Mixer channel name, slot 0 parameter 9 | Built-in | 18 | Wrote and returned `Control Text` |
| Probe text, slot 1 parameter 1 | SDK unit 18 | 0 | Wrote and returned `Field 1` with the explicit handler |
| Probe comparison, slot 1 parameter 3 | `kNT_unitHasStrings` (16), as in gain.cpp | 16 | Wrote and returned `Field 3` with the explicit handler |

The original probe had only `parameterChanged` and `parameterString`. Sending
`0x53` left its value at `Probe` and did not increment its change-callback
counter. A numeric control write did increment that counter, confirming the
callback worked. Explicitly trying Bypass 0 and 1 did not alter this result.

After adding `midiSysEx` handling for `0x53`, a write to slot 1 parameter 1
changed the readback to `Destination Test`. A firmware-saved diagnostic preset
reported `nativeStringMessages: 1`, `slotString: "Destination Test"`, and only
the initial numeric change callbacks. The built-in host string getter also
returned the stored text. Thus the native protocol receive path works when the
plug-in implements it; a missing named string-setter callback is not a transport
blocker.

For example, the text write sent to the probe was:

```text
F0 00 21 27 6D 00 53 01 00 00 01 <ASCII text> 00 F7
```

Readback used:

```text
F0 00 21 27 6D 00 50 01 00 00 01 F7
```

## Firmware conversion verified in ARM emulation

The owner's local `NT_1.19beta_18.zip` contains a firmware image whose embedded
build date exactly matches the connected NT: Sep 16 2026 11:56:15. Its SHA-256 is
`5c354798f2774cf4deb939e603a70f63a1b8a9f73d5e1c1d897f6ce009a3463b`.

The `NT_updateParameterDefinition` export leads to the plug-in definition
conversion routine. For ordinary unit values, that routine compares the SDK
unit with 18 and substitutes 0 for values greater than or equal to 18. Values
100 and above follow the separate routing-parameter path, which produces an
enum property. The plug-in constructor uses the same conversion routine.

`tools/check_firmware_text_type.py` locates the routine through the named SDK
export and executes the actual instructions under Unicorn for every possible
uint8 unit value. It rejects firmware with any other hash. Results:

| SDK unit | Firmware property type |
|---|---|
| 0–17 | Same value |
| 18–99 | 0 |
| 100–255 | 1 |

No SDK unit maps to native text type 18 in this build. This reproduces the
physical-device metadata result without changing the connected device.

```sh
uv run --with unicorn --with capstone python tools/check_firmware_text_type.py \
  /path/to/NT_1.19beta_18.zip
```

The script reads firmware locally; it does not patch, flash, or distribute it.
Its inferred firmware-internal layout is used only in isolated emulation, not
in the plug-in or on hardware.

## Remaining boundary

The connected firmware's SDK unit-to-property conversion prevents the required
native text type from being exposed. Completion needs firmware support for
plug-in text-input properties, including delivery of edits made by the module's
text editor to the plug-in's storage. The verified MIDI receive path is ready
to build on, but it is not evidence of the on-device editor's write path.
Do not claim that changing a Flutter widget, adding only the string-message
handler, or formatting text solves native editing.

Once the editor declaration is established, implement destination, group and
expander-name storage, preset round trips, and revision notifications in the
real plug-in. Preserve existing colour/tag parameter indices and address the
measured 240-parameter ceiling before adding two fields for every socket.
Do not silently reduce supported expanders or introduce a custom keyboard.

## Owner update: display-only text accepted (2026-09-28)

The owner supplied a conversation with the firmware author confirming that
native editable string properties are currently unsupported in the plug-in SDK
and will have a 32-character limit. The owner now accepts native display-only
text pending that API. This supersedes the earlier insistence on native editing
as a current acceptance gate. New destination, group and expander-name edits
are limited to 32 characters; legacy longer destinations remain intact.
The existing socket pages must remain. A full value is returned through
`parameterString()` into the SDK's minimum 64-byte buffer.
