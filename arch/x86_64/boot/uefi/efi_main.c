#include <efi.h>
#include <efiprot.h>
#include <miniOS/types.h>
#include <miniOS/boot_info.h>

static EFI_SYSTEM_TABLE  *gST;
static EFI_BOOT_SERVICES  *gBS;

static void Print(const CHAR16 *s) {
    if (gST && gST->ConOut)
        gST->ConOut->OutputString(gST->ConOut, (CHAR16 *)s);
}

typedef u64 E64A; typedef u64 E64O; typedef u32 E64W;
#define PT 1
typedef struct { unsigned char id[16]; u16 t,m; u32 v; u64 e; u64 po,so; u32 fl; u16 esz,psz,pn,snsz,sn,ssi; } __attribute__((packed)) E64H;
typedef struct { E64W t,fl; E64O o; E64A va,pa; E64A fsz,msz,al; } E64P;

static boot_info_t bi;
static EFI_MEMORY_DESCRIPTOR *mp;
static UINTN ms,mk,ds; static UINT32 dv;

#define PZ 0x1000
#define PP (1ULL<<0)
#define PW (1ULL<<1)
#define PH (1ULL<<7)
static u64 *pml4,*pdpt,*pd0,*pd1,*pd2,*pd3;

static void *Alloc(UINTN sz) { void *p=NULL; gBS->AllocatePool(EfiLoaderData,sz,&p); return p; }

static u32 ut(UINT32 t) {
    switch(t){case 1:case 2:case 3:case 4:case 7:return MEM_TYPE_USABLE;
    case 9:return MEM_TYPE_ACPI_RECLAIM; case 10:return MEM_TYPE_ACPI_NVS; default:return MEM_TYPE_RESERVED;}
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE h, EFI_SYSTEM_TABLE *stbl)
{
    gST=stbl; gBS=stbl->BootServices;

    /* GOP */
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop=NULL;
    EFI_GUID g=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_STATUS s=gBS->LocateProtocol(&g,NULL,(void**)&gop);
    if(!EFI_ERROR(s)&&gop&&gop->Mode){
        bi.framebuffer_addr=(void*)(UINTN)gop->Mode->FrameBufferBase;
        bi.framebuffer_width=gop->Mode->Info->HorizontalResolution;
        bi.framebuffer_height=gop->Mode->Info->VerticalResolution;
        bi.framebuffer_pitch=gop->Mode->Info->PixelsPerScanLine*4;
        bi.framebuffer_bpp=32;
    }

    /* Load kernel */
    EFI_LOADED_IMAGE_PROTOCOL *li=NULL;
    EFI_GUID lg=EFI_LOADED_IMAGE_PROTOCOL_GUID;
    gBS->HandleProtocol(h,&lg,(void**)&li);
    EFI_FILE_IO_INTERFACE *fs=NULL;
    EFI_GUID sg=EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    gBS->HandleProtocol(li->DeviceHandle,&sg,(void**)&fs);
    EFI_FILE_HANDLE root,kf;
    fs->OpenVolume(fs,&root);
    s=root->Open(root,&kf,L"kernel.elf",EFI_FILE_MODE_READ,0);
    if(EFI_ERROR(s)){Print(L"ERR: no kernel.elf\r\n");return s;}
    E64H eh; UINTN fsz=sizeof(eh); kf->Read(kf,&fsz,&eh);
    u64 kern_end=0;
    for(E64W i=0;i<eh.pn;i++){E64P ph;UINTN psz=sizeof(ph);
        kf->SetPosition(kf,eh.po+(UINTN)i*eh.psz);kf->Read(kf,&psz,&ph);
        if(ph.t!=PT)continue;
        UINTN ss=(UINTN)ph.fsz;kf->SetPosition(kf,ph.o);
        kf->Read(kf,&ss,(void*)(UINTN)ph.pa);
        if(ph.msz>ph.fsz){
            u8*b=(u8*)(UINTN)(ph.pa+ph.fsz);UINTN bl=(UINTN)(ph.msz-ph.fsz);
            for(UINTN j=0;j<bl;j++)b[j]=0;}
        u64 se=ph.pa+ph.msz;if(se>kern_end)kern_end=se;
    }
    kf->Close(kf);root->Close(root);
    void (*ke)(boot_info_t*)=(void(*)(boot_info_t*))(UINTN)eh.e;
    bi.kernel_phys_base=eh.e;

    /* Page tables at fixed physical address right after kernel.
     * UEFI AllocatePool memory can be reclaimed by ExitBootServices. */
    u64 pt_base=PAGE_ALIGN(kern_end);
    pml4=(u64*)(UINTN)(pt_base+0*PZ);
    pdpt=(u64*)(UINTN)(pt_base+1*PZ);
    pd0 =(u64*)(UINTN)(pt_base+2*PZ);
    pd1 =(u64*)(UINTN)(pt_base+3*PZ);
    pd2 =(u64*)(UINTN)(pt_base+4*PZ);
    pd3 =(u64*)(UINTN)(pt_base+5*PZ);
    for(int i=0;i<512;i++)pml4[i]=pdpt[i]=0;
    for(int i=0;i<512;i++)pd0[i]=pd1[i]=pd2[i]=pd3[i]=0;
    pml4[0]=(u64)(UINTN)pdpt|PP|PW;
    pml4[511]=(u64)(UINTN)pdpt|PP|PW;
    pdpt[0]=(u64)(UINTN)pd0|PP|PW;
    pdpt[1]=(u64)(UINTN)pd1|PP|PW;
    pdpt[2]=(u64)(UINTN)pd2|PP|PW;
    pdpt[3]=(u64)(UINTN)pd3|PP|PW;
    for(int i=0;i<512;i++){
        pd0[i]=((u64)i<<21)|PP|PW|PH;
        pd1[i]=((u64)(512+i)<<21)|PP|PW|PH;
        pd2[i]=((u64)(1024+i)<<21)|PP|PW|PH;
        pd3[i]=((u64)(1536+i)<<21)|PP|PW|PH;
    }

    /* Memory map */
    ms=0;gBS->GetMemoryMap(&ms,NULL,&mk,&ds,&dv);
    ms+=ds*8;mp=Alloc(ms);
    if(!mp){Print(L"ERR: memmap\r\n");return EFI_OUT_OF_RESOURCES;}
    s=gBS->GetMemoryMap(&ms,mp,&mk,&ds,&dv);
    if(EFI_ERROR(s)){Print(L"ERR: mmap fail\r\n");return s;}
    UINTN cnt=ms/ds;int idx=0;
    for(UINTN i=0;i<cnt&&idx<128;i++){
        EFI_MEMORY_DESCRIPTOR*d=(EFI_MEMORY_DESCRIPTOR*)((u8*)mp+i*ds);
        bi.memory_map[idx].base=d->PhysicalStart;
        bi.memory_map[idx].length=d->NumberOfPages*4096;
        bi.memory_map[idx].type=ut(d->Type);idx++;
    }
    bi.memory_map_count=(u32)idx;

    /* Build GDT before EBS (stack is still owned by firmware) */
    u64 gdt[]={0,0x00AF98000000FFFFULL,0x00CF92000000FFFFULL};
    struct{ u16 l; u64 b; }__attribute__((packed)) gp;
    gp.l=(u16)(sizeof(gdt)-1);gp.b=(u64)(UINTN)&gdt[0];

    /* ExitBootServices */
    s=gBS->ExitBootServices(h,mk);
    if(EFI_ERROR(s)){Print(L"ERR: ebs\r\n");return s;}

    /* Bootstrap: cli + lgdt + far-jump to reload CS + cr3 + jump */
    __asm__ volatile(
        "cli\n\t"
        "lgdt %0\n\t"
        /* Reload CS: the firmware CS (0x38) is invalid in our GDT.
         * lretq pops CS:RIP from the stack — we push 0x08 (our code
         * segment) and the address of label 1. */
        "pushq $0x08\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n"
        "1:\n\t"
        "mov %1, %%cr3\n\t"
        "mov %2, %%rdi\n\t"
        "jmp *%3"
        :
        : "m"(gp),
          "r"((u64)(UINTN)pml4),
          "r"(&bi),
          "r"(ke)
        : "rax", "rdi", "memory"
    );
    __builtin_unreachable();
}
