#include "engine.h"

#include "vkDevice.h"

Core::Engine Core::engine;

void Core::Engine::Initialize()
{
	m_pDevice = std::make_shared<Device>();
	m_pDevice->Initialize();
}

void Core::Engine::Update()
{
	m_pDevice->DrawFrame();
}

void Core::Engine::ShutDown()
{
	m_pDevice->ShutDown();
	m_pDevice.reset();
}

const Device& Core::Engine::GetDevice() const
{
	assert(m_pDevice.get() && "Device is not initialized or deleted");
	return *m_pDevice.get();
}
