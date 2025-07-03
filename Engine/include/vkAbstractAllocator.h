#pragma once

#include "vkCommon.h"
#include "vkMemory.h"

class Device;
class AbstractAllocator
{
public:
    AbstractAllocator(std::shared_ptr<Device> device) :
        m_device(device) {}

    virtual Block Allocate(VkDeviceSize size, VkDeviceSize alignment, int memoryTypeIndex) = 0;
    virtual void Deallocate(Block& block) = 0;

    std::shared_ptr<Device> GetDevice() const 
    {
        return m_device;
    }

    ~AbstractAllocator() = default;

    AbstractAllocator(const AbstractAllocator&) = delete;
    AbstractAllocator& operator=(const AbstractAllocator&) = delete;

protected:
    std::shared_ptr<Device> m_device;
};