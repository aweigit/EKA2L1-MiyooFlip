/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace Dynarmic {

using VAddr = std::uint64_t;

enum class MemoryPermission : std::uint32_t {
    Read = 1UL << 0,
    Write = 1UL << 1,
    Execute = 1UL << 2,

    ReadWrite = Read | Write
};

constexpr MemoryPermission operator|(MemoryPermission f1, MemoryPermission f2) {
    return static_cast<MemoryPermission>(static_cast<std::uint32_t>(f1) | static_cast<std::uint32_t>(f2));
}

constexpr MemoryPermission operator&(MemoryPermission f1, MemoryPermission f2) {
    return static_cast<MemoryPermission>(static_cast<std::uint32_t>(f1) & static_cast<std::uint32_t>(f2));
}

constexpr MemoryPermission operator|=(MemoryPermission& result, MemoryPermission f) {
    return result = (result | f);
}

struct TLBEntry {
    VAddr read_addr = 0;
    VAddr write_addr = 0;
    VAddr execute_addr = 0;

    std::uint8_t *host_base = nullptr;
};

template <size_t TLB_BIT_COUNT>
struct TLB {
public:
    static constexpr std::uint32_t TLB_ENTRY_COUNT = 1 << TLB_BIT_COUNT;

    std::array<TLBEntry, TLB_ENTRY_COUNT> entries;

    std::size_t page_bits;
    std::size_t page_mask;

    explicit TLB(std::size_t page_bits)
        : page_bits(page_bits)
        , page_mask((size_t{1} << page_bits) - 1) {
        Flush();
    }

    void Flush() {
        entries.fill(TLBEntry{});
    }

    void Add(VAddr addr, std::uint8_t *host, const MemoryPermission perm) {
        const std::size_t page_index = addr >> page_bits;
        const std::size_t tlb_index = page_index & (TLB_ENTRY_COUNT - 1);
        const std::size_t addr_mod = addr & page_mask;
        const VAddr addr_normed = addr & ~page_mask;

        TLBEntry &entry = entries[tlb_index];
        entry.host_base = host - addr_mod;

        if ((perm & MemoryPermission::Read) == MemoryPermission::Read) {
            entry.read_addr = addr_normed;
        } else {
            entry.read_addr = 0;
        }

        if ((perm & MemoryPermission::Write) == MemoryPermission::Write) {
            entry.write_addr = addr_normed;
        } else {
            entry.write_addr = 0;
        }
        
        if ((perm & MemoryPermission::Execute) == MemoryPermission::Execute) {
            entry.execute_addr = addr_normed;
        } else {
            entry.execute_addr = 0;
        }
    }

    void MakeDirty(VAddr addr) {
        const std::size_t page_index = addr >> page_bits;
        const std::size_t tlb_index = page_index & (TLB_ENTRY_COUNT - 1);
        const VAddr addr_normed = addr & ~page_mask;

        TLBEntry &entry = entries[tlb_index];

        if ((entry.read_addr == addr_normed) || (entry.write_addr == addr_normed) ||
            (entry.execute_addr == addr_normed)) {
            std::memset(&entry, 0, sizeof(TLBEntry));
        }
    }

    std::uint8_t *Lookup(VAddr addr) {
        const std::size_t page_index = addr >> page_bits;
        const std::size_t tlb_index = page_index & (TLB_ENTRY_COUNT - 1);
        const VAddr addr_normed = addr & ~page_mask;

        TLBEntry &entry = entries[tlb_index];

        if (!entry.host_base) {
            return nullptr;
        }
        
        if ((entry.read_addr == addr_normed) || (entry.write_addr == addr_normed) ||
            (entry.execute_addr == addr_normed)) {
            const std::size_t addr_mod = addr & page_mask;
            return entry.host_base + addr_mod;
        }

        // TLB miss
        return nullptr;
    }
};

}