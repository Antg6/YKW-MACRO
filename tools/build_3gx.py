import struct, sys, os

_3GX_MAGIC = 0x3230303024584733  # "3GX$0002"

EI_NIDENT = 16
ET_EXEC = 2
PT_LOAD = 1
PF_R = 4
PF_W = 2
PF_X = 1

def parse_plginfo(path):
    info = {'author': '', 'title': '', 'summary': '', 'description': '', 'targets': []}
    if not path or not os.path.exists(path):
        return info
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            if ': ' in line:
                key, val = line.split(': ', 1)
                key = key.lower()
                if key == 'author':
                    info['author'] = val
                elif key == 'title':
                    info['title'] = val
                elif key == 'summary':
                    info['summary'] = val
                elif key == 'description':
                    info['description'] = val
                elif key == 'targets':
                    val = val.strip()
                    if val.startswith('0x'):
                        info['targets'].append(int(val, 16))
                    else:
                        info['targets'].append(int(val))
                elif key == 'version':
                    pass
    return info

class ElfReader:
    def __init__(self, path):
        with open(path, 'rb') as f:
            self.data = f.read()
        if self.data[:4] != b'\x7fELF':
            raise ValueError("Not a valid ELF file")
        e_phoff = struct.unpack_from('<I', self.data, 28)[0]
        e_phentsize = struct.unpack_from('<H', self.data, 42)[0]
        e_phnum = struct.unpack_from('<H', self.data, 44)[0]
        e_entry = struct.unpack_from('<I', self.data, 24)[0]
        self.code_data = b''
        self.rodata_data = b''
        self.data_data = b''
        self.code_size = 0
        self.rodata_size = 0
        self.data_size = 0
        self.bss_size = 0
        base_addr = None
        for i in range(e_phnum):
            off = e_phoff + i * e_phentsize
            p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = \
                struct.unpack_from('<IIIIIIII', self.data, off)
            if p_type != PT_LOAD or p_memsz == 0:
                continue
            if i == 0:
                base_addr = p_vaddr
            if p_memsz != p_filesz and p_flags != 6:
                raise ValueError("Only data segment can have BSS")
            seg_data = self.data[p_offset:p_offset + p_filesz]
            if p_flags == 5:
                self.code_data = seg_data
                self.code_size = p_memsz
            elif p_flags == 4:
                self.rodata_data = seg_data
                self.rodata_size = p_memsz
            elif p_flags == 6:
                self.data_data = seg_data
                self.data_size = p_filesz
                self.bss_size = p_memsz - p_filesz
        if not self.code_data:
            raise ValueError("No code segment found")
        if base_addr is not None and e_entry != base_addr:
            raise ValueError("Entry point must equal segment base address")

def build_3gx(elf_path, out_path, plginfo_path):
    info = parse_plginfo(plginfo_path)
    elf = ElfReader(elf_path)

    # Embedded exe load function: returns 0 (mov r0,#0; bx lr; NOP terminator)
    exe_load_func = struct.pack('<III',
        0xE3A00000,  # mov r0, #0
        0xE12FFF1E,  # bx lr
        0xE320F000,  # nop (terminator)
    )
    exe_load_func += b'\x00' * (32 * 4 - len(exe_load_func))

    with open(out_path, 'wb') as f:
        f.write(b'\x00' * 148)
        meta = {}
        for key in ['title', 'author', 'summary', 'description']:
            val = info.get(key, '')
            if val:
                meta[key] = (f.tell(), len(val) + 1)
                f.write(val.encode('utf-8') + b'\x00')
        target_off = 0
        if info['targets']:
            target_off = f.tell()
            for t in info['targets']:
                f.write(struct.pack('<I', t))
        code_off = f.tell()
        pad = (16 - (code_off & 0xF)) & 0xF
        if pad:
            f.write(b'\x00' * pad)
            code_off += pad
        rodata_off = code_off + len(elf.code_data)
        data_off = rodata_off + len(elf.rodata_data)
        f.write(elf.code_data)
        f.write(elf.rodata_data)
        f.write(elf.data_data)

        # Embedded exe load function after segment data
        exe_off = f.tell()
        f.write(exe_load_func)

        f.seek(0)
        f.write(struct.pack('<Q', _3GX_MAGIC))
        f.write(struct.pack('<I', 2))   # version
        f.write(struct.pack('<I', 0))   # reserved
        f.write(struct.pack('<II', meta.get('author', (0,0))[1], meta.get('author', (0,0))[0]))
        f.write(struct.pack('<II', meta.get('title', (0,0))[1], meta.get('title', (0,0))[0]))
        f.write(struct.pack('<II', meta.get('summary', (0,0))[1], meta.get('summary', (0,0))[0]))
        f.write(struct.pack('<II', meta.get('description', (0,0))[1], meta.get('description', (0,0))[0]))
        f.write(struct.pack('<I', 1))   # flags: embeddedExeLoadFunc=1
        f.write(struct.pack('<I', 0))   # exeLoadChecksum
        f.write(b'\x00' * 16)          # builtInLoadExeArgs[4]
        f.write(b'\x00' * 16)          # builtInSwapSaveLoadArgs[4]
        f.write(struct.pack('<IIIIIIIIII',
            code_off, rodata_off, data_off,
            elf.code_size, elf.rodata_size, elf.data_size, elf.bss_size,
            exe_off,   # exeLoadFuncOffset
            0,         # swapSaveFuncOffset
            0))        # swapLoadFuncOffset
        f.write(struct.pack('<II', len(info['targets']), target_off))
        f.write(b'\x00' * 12)
    total = os.path.getsize(out_path)
    print(f"Built: {out_path} ({total} bytes, code={elf.code_size} rodata={elf.rodata_size} data={elf.data_size} bss={elf.bss_size}, exeLoadFuncOff={exe_off})")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.elf> <output.3gx> [plgInfo]")
        sys.exit(1)
    build_3gx(sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else None)
