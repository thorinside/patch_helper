# Expose native text-input properties to C++ plug-ins

Could the C++ API expose the disting NT's existing native text editor for
plug-in parameters, with a way to receive the edited string?

Patch Helper needs editable destination names, groups, and expander names on
the module as well as in NT Helper. A custom keyboard would duplicate the
module's existing editor.

Test environment: firmware v1.19.0beta, Sep 16 2026 11:56:15; public API 13 at
`9aeda6d41484815b80416b903a564510da6026cd`.

The built-in Mixer Mono channel-name property reports unit 18. The following
plug-in definition instead reports unit 0 through the parameter-info message:

```cpp
{"Destination", 0, 0, 0, 18, 0, nullptr}
```

Using `kNT_unitHasStrings`, as in `examples/gain.cpp`, reports unit 16 and
supports formatted string readback, but does not declare native text input.
The plug-in definition conversion in this firmware changes ordinary SDK units
18–99 to 0. A local ARM-emulation check of the matching firmware image confirms
that none of the 256 possible unit values produces native type 18.

The MIDI side already works when the plug-in explicitly handles `0x53` in
`midiSysEx`, stores the supplied text, and exposes it via `parameterString`:
`0x50` and `_NT_slot::parameterString` both return the stored value. Sending
`0x53` alone does not call `parameterChanged` in the isolated test. Bypass was
explicitly set to 0 and read back; the numeric change callback also passed a
control test.

The requested support is:

- A documented plug-in parameter type that opens the existing native text editor.
- A documented way for the plug-in to receive committed native-editor text,
  and an explicit contract for MIDI `0x53` writes to the same property.
- Reading the current text through `parameterString`, including after an
  external edit or preset load.
- Defined text length/encoding and commit/cancel behavior so edits can safely
  use memory allocated in `calculateRequirements`.

Success would be a minimal C++ plug-in with a Destination field that can be
edited using the module's native editor, read and written over the native
string SysEx messages, and restored from the plug-in's serialized preset state.
