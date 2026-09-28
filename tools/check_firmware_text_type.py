"""Read-only ARM check of one verified NT firmware's plug-in type conversion.

Requires unicorn and capstone. Accepts the owner's firmware ZIP or extracted
disting_NT.bin. No firmware bytes are bundled, modified, or sent to hardware.
The hash guard prevents applying inferred memory layouts to another build.
"""
import hashlib
from pathlib import Path
import struct
import sys
import zipfile

from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
from unicorn.arm_const import (
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC,
)


def firmware_bytes(path):
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            names = [n for n in archive.namelist() if n.endswith('/disting_NT.bin')]
            if len(names) != 1:
                raise ValueError('Expected exactly one disting_NT.bin in the archive')
            return archive.read(names[0])
    return path.read_bytes()


def check(path):
    firmware = firmware_bytes(path)
    digest = hashlib.sha256(firmware).hexdigest()
    expected = '5c354798f2774cf4deb939e603a70f63a1b8a9f73d5e1c1d897f6ce009a3463b'
    if digest != expected:
        raise ValueError('Unverified firmware build: ' + digest)

    flash_base = 0x60001000
    name = firmware.index(b'NT_updateParameterDefinition\0')
    # The export table pairs each function address with its following name.
    reference = firmware.index(struct.pack('<I', flash_base + name))
    wrapper = struct.unpack_from('<I', firmware, reference - 4)[0] & ~1
    decoder = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    instructions = list(decoder.disasm(
        firmware[wrapper - flash_base:wrapper - flash_base + 96], wrapper))
    call = next(i for i in instructions if i.mnemonic == 'bl')
    conversion = int(call.op_str.removeprefix('#'), 16)
    if wrapper != 0x6006fa2c or conversion != 0x600a36b8:
        raise ValueError('Unexpected conversion routine')

    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    cpu.mem_map(0x60000000, 0x200000)
    cpu.mem_write(flash_base, firmware)
    cpu.mem_map(0x20000000, 0x100000)
    slot, plugin, parameters = 0x20010000, 0x20020000, 0x20021000
    results = {}
    for unit in range(256):
        cpu.mem_write(slot, bytes(4096))
        cpu.mem_write(plugin, struct.pack('<I', parameters))
        cpu.mem_write(slot + 0x768, struct.pack('<I', plugin))
        # The public SDK's ARM _NT_parameter layout; min/max/default all zero.
        cpu.mem_write(parameters, struct.pack('<IhhhBBI', 0, 0, 0, 0, unit, 0, 0))
        cpu.reg_write(UC_ARM_REG_R0, slot)
        cpu.reg_write(UC_ARM_REG_R1, 1)  # first plug-in parameter, after Bypass
        cpu.reg_write(UC_ARM_REG_SP, 0x200f0000)
        cpu.reg_write(UC_ARM_REG_LR, 0x200e0001)
        cpu.emu_start(conversion | 1, 0x200e0000, count=1000)
        if cpu.reg_read(UC_ARM_REG_PC) != 0x200e0000:
            raise RuntimeError('Conversion did not return')
        results[unit] = cpu.mem_read(slot + 0x784, 1)[0]

    print('Firmware: v1.19.0beta, Sep 16 2026 11:56:15')
    print('SHA-256:', digest)
    print('SDK 0..17 ->', [results[i] for i in range(18)])
    print('SDK 18..99 ->', sorted({results[i] for i in range(18, 100)}))
    print('SDK 100..255 ->', sorted({results[i] for i in range(100, 256)}))
    print('Declarations producing native text type 18:',
          [unit for unit, native_type in results.items() if native_type == 18])
    expected_results = {unit: unit if unit < 18 else 0 if unit < 100 else 1
                        for unit in range(256)}
    if results != expected_results:
        raise RuntimeError('Type conversion differs from the observed hardware behavior')
    print('CONFIRMED: this firmware conversion cannot expose plug-in text type 18')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('Usage: check_firmware_text_type.py <firmware.zip|disting_NT.bin>')
    check(Path(sys.argv[1]))
