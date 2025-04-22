#include "engine.h"

#include "vkDevice.h"
#include "vkRender.h"

Core::Engine Core::engine;

void Core::Engine::Initialize()
{
	m_pDevice = std::make_shared<Device>();
	m_pDevice->Initialize();

	m_pRenderer = std::make_shared<Renderer>(m_pDevice);
}

void Core::Engine::Update()
{
	m_pRenderer->Update();
	m_pRenderer->Render();
}

void Core::Engine::ShutDown()
{
	m_pRenderer.reset();
	m_pDevice->ShutDown();
	m_pDevice.reset();
}

const Device& Core::Engine::GetDevice() const
{
	assert(m_pDevice.get() && "Device is either uninitialized or deleted");
	return *m_pDevice.get();
}

const Renderer& Core::Engine::GetRenderer() const
{
	assert(m_pRenderer.get() && "Renderer is either uninitialized or deleted");
	return *m_pRenderer.get();
}
