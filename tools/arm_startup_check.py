"""Run the built ARM plug-in's startup with strict data alignment.

This is a CPU/callback smoke test, not an NT firmware or loader emulator.
Host imports are explicit stubs; no hardware acceptance is implied.
"""
import pathlib
import struct
import subprocess
import sys
import tempfile

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC


def run(elf_path, imports, fill):
    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    cpu.mem_map(0x10000, 0x100000)
    cpu.mem_map(0x20000000, 0x100000)
    with elf_path.open('rb') as stream:
        elf = ELFFile(stream)
        for segment in elf.iter_segments():
            if segment['p_type'] == 'PT_LOAD':
                cpu.mem_write(segment['p_vaddr'], segment.data())
        symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
    stubs = {symbols[name] & ~1: name for name in imports}
    drawn = []

    def string(address):
        result = bytearray()
        for i in range(1024):
            byte = cpu.mem_read(address + i, 1)[0]
            if byte == 0:
                return bytes(result)
            result.append(byte)
        raise AssertionError('Unterminated string')

    def host_call(cpu, address, _size, _user):
        if address not in stubs:
            return
        name = stubs[address]
        r0, r1, r2 = [cpu.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2)]
        if name == 'memset':
            cpu.mem_write(r0, bytes([r1 & 255]) * r2)
        elif name == 'memcpy':
            cpu.mem_write(r0, bytes(cpu.mem_read(r1, r2)))
        elif name == 'strlen':
            cpu.reg_write(UC_ARM_REG_R0, len(string(r0)))
        elif name == 'strcpy':
            cpu.mem_write(r0, string(r1) + b'\0')
        elif name == 'strcat':
            cpu.mem_write(r0 + len(string(r0)), string(r1) + b'\0')
        elif name == 'strncpy':
            cpu.mem_write(r0, string(r1)[:r2].ljust(r2, b'\0'))
        elif name == 'NT_intToString':
            text = str(r1).encode()
            cpu.mem_write(r0, text + b'\0')
            cpu.reg_write(UC_ARM_REG_R0, len(text))
        elif name == 'NT_drawText':
            drawn.append(string(r2).decode())
        elif name in ('NT_algorithmIndex', 'NT_parameterOffset'):
            cpu.reg_write(UC_ARM_REG_R0, 0)
        else:
            raise AssertionError('Unexpected startup host call: ' + name)

    def memory_access(cpu, _kind, address, size, _value, _user):
        # Enforce natural alignment even when the emulator would permit it.
        if size > 1 and address % size:
            pc = cpu.reg_read(UC_ARM_REG_PC)
            raise AssertionError(f'Unaligned {size}-byte access at 0x{address:x}, PC=0x{pc:x}')

    cpu.hook_add(UC_HOOK_CODE, host_call)
    cpu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory_access)
    cpu.mem_write(0x20020000, bytes([fill]) * 65536)
    # Parameters require halfword alignment, not word alignment.
    cpu.mem_write(0x20010002, struct.pack('<hhh', 1, 0, 0))
    cpu.reg_write(UC_ARM_REG_R0, 0x20020000)
    cpu.reg_write(UC_ARM_REG_R1, 0x20010002)
    cpu.reg_write(UC_ARM_REG_SP, 0x200f0000)
    cpu.reg_write(UC_ARM_REG_LR, 0x80001)
    cpu.emu_start(symbols['probe'] | 1, 0x80000, count=1000000)
    assert cpu.reg_read(UC_ARM_REG_PC) == 0x80000, 'Startup did not return'
    assert drawn == ['Patch Helper'] + [v for i in range(1, 5) for v in (f'In {i}', 'None', '(unused)')], drawn


plugin, probe = map(pathlib.Path, sys.argv[1:])
undefined = subprocess.check_output(['arm-none-eabi-nm', '-u', str(plugin)], text=True)
imports = [line.split()[-1] for line in undefined.splitlines() if line.split()[-1] != '_GLOBAL_OFFSET_TABLE_']
with tempfile.TemporaryDirectory(prefix='patch-helper-arm-') as folder:
    folder = pathlib.Path(folder)
    assembly = folder / 'stubs.s'
    assembly.write_text('.syntax unified\n.thumb\n.text\n' + ''.join(
        f'.global {name}\n.thumb_func\n{name}:\n bx lr\n' for name in imports))
    stubs, elf = folder / 'stubs.o', folder / 'startup.elf'
    subprocess.run(['arm-none-eabi-as', '-mcpu=cortex-m7', '-mthumb', str(assembly), '-o', str(stubs)], check=True)
    subprocess.run(['arm-none-eabi-ld', '-Ttext=0x10000', '-Tdata=0x20000000', '-e', 'probe', str(probe), str(plugin), str(stubs), '-o', str(elf)], check=True)
    for fill in (0, 0xa5):
        run(elf, imports, fill)
print('PASS: ARM construction, initial parameters, first step and draw with strict alignment and poisoned SRAM')
