/**
 * @file init_paging.cpp
 *
 */

#include "types.h"
#include "boot-time.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "multiboot.h"
#include "kprintf.h"

extern PageDirPointerTableEntry kernel_page_directory_pointer_table[];
extern PageDirEntry kernel_page_directory[];
extern PageTableEntry kernel_page_table[];
extern PageMapLevel4Entry kernel_page_map_level_4[];

extern "C" void initialisePaging();
extern "C" void removeBootTimeMapping();

void initialisePaging()
{
  uint32 i;

  PageMapLevel4Entry *pml4 = (PageMapLevel4Entry*)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);
  PageDirPointerTableEntry *pdpt1 = (PageDirPointerTableEntry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_directory_pointer_table);
  PageDirPointerTableEntry *pdpt2 = pdpt1 + PAGE_DIR_POINTER_TABLE_ENTRIES;
  PageDirEntry *pd1 = (PageDirEntry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_page_directory);
  PageDirEntry *pd2 = pd1 + PAGE_DIR_ENTRIES;

  // Note: the only valid address ranges are currently:
  //         * 0000 0000 0 to * 7FFF FFFF F
  //                       binary: 011 111 111 means pml4i <= 255
  //         * 8000 0000 0 to * FFFF FFFF F
  //                       binary: 100 000 000 means pml4i >= 256

  // map the first and the last PM4L entry and one for the identity mapping
  pml4[0].page_ppn = (uint64)pdpt1 / PAGE_SIZE;
  pml4[0].writeable = 1;
  pml4[0].present = 1;
  pml4[480].page_ppn = (uint64)pdpt1 / PAGE_SIZE;
  pml4[480].writeable = 1;
  pml4[480].present = 1;
  pml4[511].page_ppn = (uint64)pdpt2 / PAGE_SIZE;
  pml4[511].writeable = 1;
  pml4[511].present = 1;

  // ident mapping 0x0                <-> 0x0 --> pml4i = 0, pdpti = 0
  // ident mapping 0x* 0000 C000 0000 <-> 0x0 --> pml4i = 0, pdpti = 3
  // ident mapping 0x* F000 0000 0000 <-> 0x0 --> pml4i = 480, pdpti = 0
  // ident mapping 0x* FFFF 8000 0000 <-> 0x0 --> pml4i = 511, pdpti = 510

  pdpt1[0].pd.page_ppn = (uint64) pd1 / PAGE_SIZE;
  pdpt1[0].pd.writeable = 1;
  pdpt1[0].pd.present = 1;
  // 1 GiB for the kernel
  pdpt2[510].pd.page_ppn = (uint64) pd2 / PAGE_SIZE;
  pdpt2[510].pd.writeable = 1;
  pdpt2[510].pd.present = 1;
  // identity map
  for (i = 0; i < PAGE_DIR_ENTRIES; ++i)
  {
    pd2[i].page.present = 0;
    pd1[i].page.page_ppn = i;
    pd1[i].page.size = 1;
    pd1[i].page.writeable = 1;
    pd1[i].page.present = 1;
  }
  // kernel map 4 MiB (2 pages)
  for (i = 0; i < 2; ++i)
  {
    pd2[i].page.page_ppn = i;
    pd2[i].page.size = 1;
    pd2[i].page.writeable = 1;
    pd2[i].page.present = 1;
  }
  extern struct multiboot_remainder mbr;
  struct multiboot_remainder &orig_mbr = (struct multiboot_remainder &)(*((struct multiboot_remainder*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&mbr)));
  uint32 j;
  if (orig_mbr.have_vesa_console)
  {
    for (i = 0; i < 8; ++i) // map the 16 MiB (8 pages) framebuffer
    {
      pd2[504+i].page.present = 1;
      pd2[504+i].page.writeable = 1;
      pd2[504+i].page.size = 1;
      pd2[504+i].page.cache_disabled = 1;
      pd2[504+i].page.write_through = 1;
      pd2[504+i].page.page_ppn = (orig_mbr.vesa_lfb_pointer / (PAGE_SIZE * PAGE_TABLE_ENTRIES))+i;
    }
  }
}

void removeBootTimeMapping()
{
  uint64* pml4 = (uint64*)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);
  pml4[0] = 0;
  pml4[1] = 0;
}
