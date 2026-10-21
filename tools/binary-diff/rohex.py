import sys
sys.path.insert(0,'.')
from parse import elf_sections
da,sa=elf_sections('/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so'); db,sb=elf_sections(sys.argv[1])
A=da[sa['.rodata'][1]:sa['.rodata'][1]+sa['.rodata'][2]]; B=db[sb['.rodata'][1]:sb['.rodata'][1]+sb['.rodata'][2]]
va=int(sys.argv[2],16); ln=int(sys.argv[3]); off=va-sb['.rodata'][0]
pre=B[off-64:off]; p=A.find(pre); 
assert p>=0
ao=p+64
print('A off %#x (va %#x)  B va %#x'%(ao, ao+sa['.rodata'][0], va))
for k in range(0, ln+32, 16):
    x=A[ao+k:ao+k+16]; y=B[off+k:off+k+16]
    print('%5d %s | %s %s'%(k, x.hex(' '), y.hex(' '), '' if x==y else '<<'))
