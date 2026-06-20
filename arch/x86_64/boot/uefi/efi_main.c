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
typedef struct { unsigned char id[16]; E64W t,m,v; E64A e; E64O po,so; E64W fl,esz,psz,pn,snsz,sn,ssi; } E64H;
typedef struct { E64W t,fl; E64O o; E64A va,pa; E64W fsz,msz,al; } E64P;

static boot_info_t bi;
static EFI_MEMORY_DESCRIPTOR *mp;
static UINTN ms,mk,ds; static UINT32 dv;

#define PZ 0x1000
#define PP (1ULL<<0)
#define PW (1ULL<<1)
#define PH (1ULL<<7)
static u64 *pml4,*pdpt,*pd;

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
    for(E64W i=0;i<eh.pn;i++){E64P ph;UINTN psz=sizeof(ph);
        kf->SetPosition(kf,eh.po+(UINTN)i*eh.psz);kf->Read(kf,&psz,&ph);
        if(ph.t!=PT)continue;UINTN ss=ph.fsz;kf->SetPosition(kf,ph.o);
        kf->Read(kf,&ss,(void*)(UINTN)ph.pa);
        if(ph.msz>ph.fsz){u8*b=(u8*)(UINTN)(ph.pa+ph.fsz);UINTN bl=ph.msz-ph.fsz;
            for(UINTN j=0;j<bl;j++)b[j]=0;}
    }
    kf->Close(kf);root->Close(root);
    void (*ke)(boot_info_t*)=(void(*)(boot_info_t*))(UINTN)eh.e;
    bi.kernel_phys_base=eh.e;

    /* Page tables BEFORE memmap */
    pml4=Alloc(PZ);pdpt=Alloc(PZ);pd=Alloc(PZ);
    for(int i=0;i<512;i++)pml4[i]=pdpt[i]=pd[i]=0;
    pml4[0]=(u64)(UINTN)pdpt|PP|PW;
    pml4[511]=(u64)(UINTN)pdpt|PP|PW;
    pdpt[0]=(u64)(UINTN)pd|PP|PW;
    for(int i=0;i<512;i++)pd[i]=((u64)i<<21)|PP|PW|PH;

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

    /* ExitBootServices */
    s=gBS->ExitBootServices(h,mk);
    if(EFI_ERROR(s)){ms=0;gBS->GetMemoryMap(&ms,mp,&mk,&ds,&dv);
        gBS->ExitBootServices(h,mk);}

    /* GDT + CR3 switch + jump to kernel */
    u64 gdt[]={0,0x00AF98000000FFFFULL,0x00CF92000000FFFFULL};
    struct{ u16 l; u64 b; }__attribute__((packed)) gp;
    gp.l=(u16)(sizeof(gdt)-1);gp.b=(u64)(UINTN)&gdt[0];
    __asm__ volatile("lgdt %0"::"m"(gp):"memory");
    __asm__ volatile("mov %0,%%cr3"::"r"((u64)(UINTN)pml4):"memory");
    __asm__ volatile("mov %0,%%rdi\n\tjmp *%1"::"r"(&bi),"r"(ke):"rdi");
    return EFI_SUCCESS;
}
