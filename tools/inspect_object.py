"""Fail on an invalid ARM object or unexpected host/runtime imports."""
import pathlib
import struct
import subprocess
import sys

path = pathlib.Path(sys.argv[1])
data = path.read_bytes()
assert data[:7] == b'\x7fELF\x01\x01\x01', 'Expected ELF32 little endian'
assert struct.unpack_from('<HH', data, 16) == (1, 40), 'Expected ARM relocatable'
attributes = subprocess.check_output(['arm-none-eabi-readelf', '-A', str(path)], text=True)
assert 'Tag_CPU_unaligned_access: v6' not in attributes, 'Build must disable unaligned accesses'
symbols = subprocess.check_output(['arm-none-eabi-nm', str(path)], text=True)
assert any(line.split()[-2:] == ['T', 'pluginEntry'] for line in symbols.splitlines())
allowed = {'NT_drawText', 'NT_intToString', 'memcpy', 'memset', 'strlen', 'strncpy', 'strcpy',
           'NT_algorithmIndex', 'NT_parameterOffset', 'NT_setParameterFromAudio', 'NT_updateParameterDefinition', 'NT_updateParameterPages', 'strcat', 'NT_getSlot', 'NT_sendMidiSysEx', '_ZNK8_NT_slot4guidEv', '_ZNK8_NT_slot6pluginEv',
           }  # Only imports already used by this plug-in's host API contract.
for line in symbols.splitlines():
    fields = line.split()
    if len(fields) == 2 and fields[0] == 'U':
        symbol = fields[1]
        assert symbol in allowed or symbol.startswith(('_ZN13_NT_jsonParse', '_ZN14_NT_jsonStream')), symbol
relocations = subprocess.check_output(['arm-none-eabi-readelf', '-rW', str(path)], text=True)
assert 'R_ARM_GOT' not in relocations and 'R_ARM_BASE_PREL' not in relocations, 'Unexpected GOT dependency'
print('PASS: ARM relocatable, pluginEntry, no heap imports, and no GOT dependency')
