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
    sram_size = None
    arena = 0x20020000

    def check_instance_access(address, size):
        if sram_size is not None and address < arena + 65536 and address + size > arena:
            assert arena <= address and address + size <= arena + sram_size, 'Access beyond requested SRAM'

    def read(address, size):
        check_instance_access(address, size)
        return bytes(cpu.mem_read(address, size))

    def write(address, data):
        check_instance_access(address, len(data))
        cpu.mem_write(address, data)


    def string(address):
        result = bytearray()
        for i in range(1024):
            byte = read(address + i, 1)[0]
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
            write(r0, bytes([r1 & 255]) * r2)
        elif name == 'memcpy':
            write(r0, read(r1, r2))
        elif name == 'strlen':
            cpu.reg_write(UC_ARM_REG_R0, len(string(r0)))
        elif name == 'strcpy':
            write(r0, string(r1) + b'\0')
        elif name == 'strcat':
            write(r0 + len(string(r0)), string(r1) + b'\0')
        elif name == 'strncpy':
            write(r0, string(r1)[:r2].ljust(r2, b'\0'))
        elif name == 'NT_intToString':
            text = str(r1).encode()
            write(r0, text + b'\0')
            cpu.reg_write(UC_ARM_REG_R0, len(text))
        elif name == 'NT_drawText':
            drawn.append(string(r2).decode())
        elif name in ('NT_updateParameterPages', 'NT_updateParameterDefinition'):
            pass
        elif name == 'NT_setParameterFromAudio':
            write(0x20010002 + r1 * 2, struct.pack('<h', r2))
        elif name in ('NT_algorithmIndex', 'NT_parameterOffset'):
            cpu.reg_write(UC_ARM_REG_R0, 0)
        else:
            raise AssertionError('Unexpected startup host call: ' + name)

    def memory_access(cpu, _kind, address, size, _value, _user):
        check_instance_access(address, size)
        # Enforce natural alignment even when the emulator would permit it.
        if size > 1 and address % size:
            pc = cpu.reg_read(UC_ARM_REG_PC)
            raise AssertionError(f'Unaligned {size}-byte access at 0x{address:x}, PC=0x{pc:x}')

    cpu.hook_add(UC_HOOK_CODE, host_call)
    cpu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory_access)
    def call(name):
        cpu.reg_write(UC_ARM_REG_SP, 0x200f0000)
        cpu.reg_write(UC_ARM_REG_LR, 0xf0001)
        cpu.emu_start(symbols[name] | 1, 0xf0000, count=1000000)
        assert cpu.reg_read(UC_ARM_REG_PC) == 0xf0000, 'Callback did not return'

    call('probeSram')
    sram_size = cpu.reg_read(UC_ARM_REG_R0)
    assert 0 < sram_size < 65536
    cpu.mem_write(arena, bytes([fill]) * sram_size)
    # Parameters require halfword alignment, not word alignment.
    cpu.mem_write(0x20010002, struct.pack('<251h', 1, *([0] * 250)))
    cpu.reg_write(UC_ARM_REG_R0, 0x20020000)
    cpu.reg_write(UC_ARM_REG_R1, 0x20010002)
    call('probe')
    assert drawn == [v for i in range(1, 5) for v in (f'In {i}', 'None', '(unused)')], drawn


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
    for text_base, data_base in ((0x10000, 0x20000000), (0x50000, 0x20001000)):
        subprocess.run(['arm-none-eabi-ld', f'-Ttext={text_base:#x}', f'-Tdata={data_base:#x}', '-e', 'probe', str(probe), str(plugin), str(stubs), '-o', str(elf)], check=True)
        for fill in (0, 0xa5):
            run(elf, imports, fill)
print('PASS: ARM construction, initial parameters, first step and draw at two load addresses, within requested SRAM, with strict alignment and poisoned memory')
