// Embedded PID helper functions and Unity API content
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <mach/mach.h>
#import <CoreFoundation/CoreFoundation.h>
#import <mach-o/loader.h>
#import <mach-o/fat.h>
#import <mach-o/dyld.h>
#import <mach-o/dyld_images.h>
#import <mach/vm_page_size.h>
#import <mach/task_info.h>
#import <mach/mach_traps.h>
#import <stdio.h>
#import <stdlib.h>
#import <libgen.h>
#import <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#import <cstddef>
#import <cstring>
#import <cstdlib>
#import <dlfcn.h>
#import <spawn.h>
#import <unistd.h>
#import <sys/sysctl.h>
#import <mach/mach.h>

#pragma once

#include <vector>
#include <functional>
#include <utility>
#include <cstdint>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

#include <sys/sysctl.h>
#include <sys/types.h>

extern "C" {

extern kern_return_t
vm_read(
        vm_map_read_t target_task,
        vm_address_t address,
        vm_size_t size,
        vm_offset_t *data,
        mach_msg_type_number_t *dataCnt
        );

extern kern_return_t
mach_vm_read_overwrite(
                       vm_map_t           target_task,
                       mach_vm_address_t  address,
                       mach_vm_size_t     size,
                       mach_vm_address_t  data,
                       mach_vm_size_t     *outsize);

extern kern_return_t
mach_vm_write(
              vm_map_t                          map,
              mach_vm_address_t                 address,
              pointer_t                         data,
              __unused mach_msg_type_number_t   size);

extern kern_return_t
mach_vm_region_recurse(
                       vm_map_t                 map,
                       mach_vm_address_t        *address,
                       mach_vm_size_t           *size,
                       uint32_t                 *depth,
                       vm_region_recurse_info_t info,
                       mach_msg_type_number_t   *infoCnt);

extern kern_return_t
processor_set_default(
                      host_t host,
                      processor_set_name_t *default_set
                      );

extern kern_return_t
host_processor_set_priv(
                        host_priv_t host_priv,
                        processor_set_name_t set_name,
                        processor_set_t *set
                        );

extern kern_return_t
processor_set_tasks(
                    processor_set_t processor_set,
                    task_array_t *task_list,
                    mach_msg_type_number_t *task_listCnt
                    );

extern kern_return_t pid_for_task(task_t task, int *pid);

extern kern_return_t
task_info(
          task_name_t target_task,
          task_flavor_t flavor,
          task_info_t task_info_out,
          mach_msg_type_number_t *task_info_outCnt
          );

extern host_name_port_t mach_host_self();

// libproc function declarations
int proc_listallpids(void* buffer, int buffersize);
int proc_name(int pid, void* buffer, uint32_t buffersize);

}

#else
#include <mach/mach_vm.h>
#include <mach-o/dyld_images.h>
#include <libproc.h>
#endif

template<typename T>
T Read(uintptr_t address, task_t task)
{
    T data = T();

    if (address <= 0 || address > 100000000000)
        return data;

    mach_vm_size_t out_size = 0;
    kern_return_t kr = mach_vm_read_overwrite(
        task,
        address,
        sizeof(T),
        (mach_vm_address_t)&data,
        &out_size
    );

    if (kr != KERN_SUCCESS || out_size != sizeof(T))
        return T();

    return data;
}

struct Vector4
{
    float x, y, z, w;
};

struct TMatrix
{
    Vector4 position;
    Vector4 rotation;
    Vector4 scale;
};

struct c_matrix
{
    float m[4][4];
    float *operator[](int index) { return m[index]; }
};

template <typename T>
struct monoArray
{
    task_t   task;
    uintptr_t address;

    monoArray()
        : task(MACH_PORT_NULL)
        , address(0)
    {
    }

    monoArray(task_t t, uintptr_t addr)
        : task(t)
        , address(addr)
    {
    }

    int get_Length() const
    {
        if (!address || !task)
            return 0;

        struct Header
        {
            void* klass;
            void* monitor;
            void* bounds;
            int   max_length;
        };

        Header h = Read<Header>(address, task);
        return h.max_length;
    }

    T operator [] (int i) const
    {
        if (!address || !task)
            return T();

        struct Layout
        {
            void* klass;
            void* monitor;
            void* bounds;
            int   max_length;
            T     first;
        };

        uintptr_t elem_addr =
            address +
            static_cast<uintptr_t>(offsetof(Layout, first)) +
            static_cast<uintptr_t>(sizeof(T)) * static_cast<uintptr_t>(i);

        return Read<T>(elem_addr, task);
    }

    T operator [] (int i)
    {
        return static_cast<const monoArray&>(*this)[i];
    }

    bool Contains(T item) const
    {
        int len = get_Length();
        for (int i = 0; i < len; ++i)
        {
            T v = (*this)[i];
            if (v == item)
                return true;
        }
        return false;
    }
};

template<typename T>
using Array = monoArray<T>;

template<typename TKey, typename TValue>
struct Dictionary
{
    struct KeysCollection;
    struct ValueCollection;

    struct Entry
    {
        int   hashCode;
        int   next;
        TKey  key;
        TValue value;
    };

    struct RemoteLayout
    {
        void*          kass;
        void*          monitor;
        Array<int>*    buckets;
        Array<Entry>*  entries;
        int            count;
        int            version;
        int            freeList;
        int            freeCount;
        void*          comparer;
        void*          keys;
        void*          values;
        void*          _syncRoot;
    };

    task_t    task;
    uintptr_t address;

    Dictionary()
        : task(MACH_PORT_NULL)
        , address(0)
    {
    }

    Dictionary(task_t t, uintptr_t addr)
        : task(t)
        , address(addr)
    {
    }

    RemoteLayout get_Remote() const
    {
        if (!address || !task)
            return RemoteLayout{};
        return Read<RemoteLayout>(address, task);
    }

    void* get_Comparer() const
    {
        auto r = get_Remote();
        return r.comparer;
    }

    int get_Count() const
    {
        auto r = get_Remote();
        return r.count;
    }

    int FindEntry(TKey key) const
    {
        auto r = get_Remote();
        if (!r.entries || r.count <= 0)
            return -1;

        uintptr_t entries_addr = reinterpret_cast<uintptr_t>(r.entries);
        monoArray<Entry> entries_arr(task, entries_addr);

        for (int i = 0; i < r.count; ++i)
        {
            Entry e = entries_arr[i];
            if (e.key == key)
                return i;
        }
        return -1;
    }

    bool ContainsKey(TKey key) const
    {
        return FindEntry(key) >= 0;
    }

    bool ContainsValue(TValue value) const
    {
        auto r = get_Remote();
        if (!r.entries || r.count <= 0)
            return false;

        uintptr_t entries_addr = reinterpret_cast<uintptr_t>(r.entries);
        monoArray<Entry> entries_arr(task, entries_addr);

        for (int i = 0; i < r.count; ++i)
        {
            Entry e = entries_arr[i];
            if (e.hashCode >= 0 && e.value == value)
                return true;
        }
        return false;
    }

    bool TryGetValue(TKey key, TValue* value) const
    {
        int i = FindEntry(key);
        if (i >= 0)
        {
            auto r = get_Remote();
            if (!r.entries)
            {
                *value = TValue();
                return false;
            }

            uintptr_t entries_addr = reinterpret_cast<uintptr_t>(r.entries);
            monoArray<Entry> entries_arr(task, entries_addr);
            Entry e = entries_arr[i];
            *value = e.value;
            return true;
        }
        *value = TValue();
        return false;
    }

    TValue GetValueOrDefault(TKey key) const
    {
        TValue v = TValue();
        TryGetValue(key, &v);
        return v;
    }

    TValue operator [] (TKey key)
    {
        TValue v = TValue();
        TryGetValue(key, &v);
        return v;
    }

    const TValue operator [] (TKey key) const
    {
        TValue v = TValue();
        const_cast<Dictionary*>(this)->TryGetValue(key, &v);
        return v;
    }

    struct KeysCollection
    {
        Dictionary* dictionary;

        KeysCollection(Dictionary* dictionary)
            : dictionary(dictionary)
        {
        }

        TKey operator [] (int i)
        {
            auto r = dictionary->get_Remote();
            if (!r.entries)
                return TKey();

            uintptr_t entries_addr = reinterpret_cast<uintptr_t>(r.entries);
            monoArray<Entry> entries_arr(dictionary->task, entries_addr);
            Entry e = entries_arr[i];
            return e.key;
        }

        const TKey operator [] (int i) const
        {
            return const_cast<KeysCollection*>(this)->operator[](i);
        }

        int get_Count() const
        {
            return dictionary->get_Count();
        }
    };

    struct ValueCollection
    {
        Dictionary* dictionary;

        ValueCollection(Dictionary* dictionary)
            : dictionary(dictionary)
        {
        }

        TValue operator [] (int i)
        {
            auto r = dictionary->get_Remote();
            if (!r.entries)
                return TValue();

            uintptr_t entries_addr = reinterpret_cast<uintptr_t>(r.entries);
            monoArray<Entry> entries_arr(dictionary->task, entries_addr);
            Entry e = entries_arr[i];
            return e.value;
        }

        const TValue operator [] (int i) const
        {
            return const_cast<ValueCollection*>(this)->operator[](i);
        }

        int get_Count() const
        {
            return dictionary->get_Count();
        }
    };

    KeysCollection get_Keys()
    {
        return KeysCollection(this);
    }

    ValueCollection get_Values()
    {
        return ValueCollection(this);
    }
};
