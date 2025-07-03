#pragma once

#include "vkCommon.h"
#include "vkMemory.h"

class Device;
class AbstractAllocator
{
public:
    AbstractAllocator() = default;

    virtual Block Allocate(VkDeviceSize size, VkDeviceSize alignment, int memoryTypeIndex) = 0;
    virtual void Deallocate(const Block& block) = 0;

    ~AbstractAllocator() = default;

    AbstractAllocator(const AbstractAllocator&) = delete;
    AbstractAllocator& operator=(const AbstractAllocator&) = delete;
};