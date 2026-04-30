#pragma once
#include <stdint.h>

struct idt_entry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

__attribute__((aligned(0x10))) 
static struct idt_entry idt[256];

static struct idtr idtr_reg;

static inline void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    uint64_t descriptor = (uint64_t)isr;

    idt[vector].isr_low    = (uint16_t)(descriptor & 0xFFFF);
    idt[vector].kernel_cs  = 0x08; 
    idt[vector].ist        = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid    = (uint16_t)((descriptor >> 16) & 0xFFFF);
    idt[vector].isr_high   = (uint32_t)((descriptor >> 32) & 0xFFFFFFFF);
    idt[vector].reserved   = 0;
}