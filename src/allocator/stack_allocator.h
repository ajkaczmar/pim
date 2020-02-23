#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "containers/array.h"
#include "os/thread.h"
#include <stdlib.h>

struct StackAllocator final : IAllocator
{
private:
    OS::Mutex m_mutex;
    Slice<u8> m_memory;
    Slice<u8> m_stack;
    Array<Allocator::Header*> m_allocations;
    AllocType m_type;

public:
    void Init(i32 bytes, AllocType type) final
    {
        ASSERT(bytes > 0);
        void* memory = malloc(bytes);
        ASSERT(memory);
        m_type = type;
        m_mutex.Open();
        m_memory = { (u8*)memory, bytes };
        m_stack = m_memory;
        m_allocations.Init(Alloc_Init);
    }

    void Reset() final
    {
        m_mutex.Lock();
        m_allocations.Reset();
        void* ptr = m_memory.begin();
        m_memory = {};
        free(ptr);
        m_mutex.Close();
    }

    void Clear() final
    {
        OS::LockGuard guard(m_mutex);
        m_stack = m_memory;
        m_allocations.Clear();
    }

    void* Alloc(i32 bytes) final
    {
        if (bytes <= 0)
        {
            ASSERT(bytes == 0);
            return nullptr;
        }
        OS::LockGuard guard(m_mutex);
        return _Alloc(bytes);
    }

    void Free(void* ptr) final
    {
        if (ptr)
        {
            OS::LockGuard guard(m_mutex);
            _Free(ptr);
        }
    }

    void* Realloc(void* ptr, i32 bytes) final
    {
        if (!ptr)
        {
            return Alloc(bytes);
        }

        if (bytes <= 0)
        {
            ASSERT(bytes == 0);
            Free(ptr);
            return nullptr;
        }

        OS::LockGuard guard(m_mutex);
        return _Realloc(ptr, bytes);
    }

private:
    void* _Alloc(i32 bytes)
    {
        using namespace Allocator;

        bytes = AlignBytes(bytes);

        Slice<u8> allocation = m_stack.Head(bytes);
        m_stack = m_stack.Tail(bytes);

        Header* hNew = (Header*)allocation.begin();
        m_allocations.PushBack(hNew);
        return MakePtr(hNew, m_type, bytes, 0);
    }

    void _Free(void* pOld)
    {
        using namespace Allocator;

        Header* hOld = ToHeader(pOld, m_type);
        ASSERT(Dec(hOld->refcount) == 1);
        Store(hOld->arg1, 1);

        while (m_allocations.size())
        {
            Header* hBack = m_allocations.back();
            if (Load(hBack->arg1))
            {
                m_allocations.Pop();
                m_stack = Combine(m_stack, hBack->AsRaw());
            }
            else
            {
                break;
            }
        }
    }

    void* _Realloc(void* pOld, i32 bytes)
    {
        using namespace Allocator;

        _Free(pOld);
        void* pNew = _Alloc(bytes);

        if (pNew != pOld)
        {
            Copy(ToHeader(pNew, m_type), ToHeader(pOld, m_type));
        }

        return pNew;
    }
};
