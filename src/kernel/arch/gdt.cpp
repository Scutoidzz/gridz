#include "gdt.hpp"

// 5 standard entries + 2 entries for the 16-byte TSS descriptor
gdt_entry_t gdt[7]; 
tss_entry_t tss;
gdtr_t gdtr;

extern "C" void gdt_flush(uint64_t gdtr_ptr);

// Helper to fill an 8-byte entry
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].base_low    = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;
    gdt[index].limit_low   = (limit & 0xFFFF);
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].granularity |= gran & 0xF0;
    gdt[index].access      = access;
}

void gdt_init() {
    // 0: Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0); 
    
    // 1: Kernel Code (Ring 0)
    // Access: 0x9A (Present, Ring 0, Code, Exec/Read)
    // Flags: 0x20 (64-bit flag), 0xA0 (Granularity combined)
    gdt_set_entry(1, 0, 0, 0x9A, 0xA0); 
    
    // 2: Kernel Data (Ring 0)
    gdt_set_entry(2, 0, 0, 0x92, 0xA0); 
    
    // 3: User Data (Ring 3) - Note: Access 0xF2 (Ring 3 flag set)
    gdt_set_entry(3, 0, 0, 0xF2, 0xA0); 
    
    // 4: User Code (Ring 3) - Note: Access 0xFA (Ring 3 flag set)
    gdt_set_entry(4, 0, 0, 0xFA, 0xA0); 

    // 5 & 6: TSS Descriptor (16 bytes in Long Mode)
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1;
    
    // Setup TSS (TSS is a system segment, Access = 0x89)
    gdt_set_entry(5, tss_base & 0xFFFFFFFF, tss_limit, 0x89, 0x00);
    
    // The upper 32 bits of the TSS base go into the next GDT slot
    gdt[6].limit_low = (tss_base >> 32) & 0xFFFF;
    gdt[6].base_low  = (tss_base >> 48) & 0xFFFF;
    gdt[6].base_middle = 0;
    gdt[6].access = 0;
    gdt[6].granularity = 0;
    gdt[6].base_high = 0;

    // Load the GDT
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt[0];

    gdt_flush((uint64_t)&gdtr);
}
