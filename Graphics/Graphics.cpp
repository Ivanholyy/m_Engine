#include "Graphics.h"

bool Graphics::Initialize(HWND hwnd, int width, int height)
{
	if (!InitializeDirectX(hwnd, width, height))
		return false;

	if (!InitializeShaders())
		return false; 

	if (!InitializeScene())
		return false; 

	return true;
}

void Graphics::RenderFrame()
{
	float bgcolor[] = { 0.0f, 0.0f, 0.0f, 1.0f };  
	this->deviceContext->ClearRenderTargetView(this->renderTargetView.Get(), bgcolor);
	this->deviceContext->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); 

	this->deviceContext->IASetInputLayout(this->vertexshader.GetInputLayout()); 
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);         
	this->deviceContext->RSSetState(this->rasterizerState.Get()); 
	this->deviceContext->OMSetDepthStencilState(this->depthStencilState.Get(), 0); 
	this->deviceContext->PSSetSamplers(0, 1, this->samplerState.GetAddressOf()); 
	this->deviceContext->VSSetShader(vertexshader.GetShader(), NULL, 0); 
	this->deviceContext->PSSetShader(pixelshader.GetShader(), NULL, 0); 

	UINT offset = 0; 

	this->deviceContext->PSSetShaderResources(0, 1, this->myTexture.GetAddressOf()); 
	this->deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), vertexBuffer.StridePtr(), &offset); 
	this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);  
	this->deviceContext->DrawIndexed(indicesBuffer.BufferSize(), 0, 0); 

	spriteBatch->Begin(); 
	spriteFont->DrawString(spriteBatch.get(), VERSION, DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::Colors::White, 0.0f, DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));  
	spriteBatch->End(); 
	 
	this->swapchain->Present(1, NULL); 
}

bool Graphics::InitializeDirectX(HWND hwnd, int width, int height)
{
	std::vector<AdapterData> adapters = AdapterReader::GetAdapters();

	if (adapters.size() < 1) {
		ErrorLogger::Log("No GPU adapters found.");
		return false;
	}

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC)); 

	scd.BufferDesc.Width = width;
	scd.BufferDesc.Height = height;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;

	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
	scd.BufferCount = 1;
	scd.OutputWindow = hwnd; 
	scd.Windowed = TRUE; 
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; 
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; 

	int maxmem = 0;
	int index = 0;
	for (int i = 0; i < adapters.size(); i++) {
		int curmem = adapters[i].description.DedicatedVideoMemory;   

		if (curmem > maxmem) {   
			maxmem = curmem;   
			index = i; 
		}
	}

	HRESULT hr; 
	hr = D3D11CreateDeviceAndSwapChain(adapters[index].pAdapter,     
								  D3D_DRIVER_TYPE_UNKNOWN,
								  NULL,
								  NULL,
		                          NULL,
		                          0,
		                          D3D11_SDK_VERSION,
		                          &scd,
		                          this->swapchain.GetAddressOf(),
		                          this->device.GetAddressOf(),
		                          NULL,
		                          this->deviceContext.GetAddressOf());

	if (FAILED(hr)) {
		ErrorLogger::Log(hr, "Failed to create a device and swapchain.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = this->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "GetBuffer Failed."); 
		return false;
	}

	hr = this->device->CreateRenderTargetView(backBuffer.Get(), NULL, this->renderTargetView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create render target view."); 
		return false;
	}

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	hr = this->device->CreateTexture2D(&depthStencilDesc, NULL, this->depthStencilBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil buffer.");
		return false;
	}

	hr = this->device->CreateDepthStencilView(this->depthStencilBuffer.Get(), NULL, this->depthStencilView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil view.");
		return false;
	}

	this->deviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get());   

	D3D11_DEPTH_STENCIL_DESC depthstencildesc;
	ZeroMemory(&depthstencildesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthstencildesc.DepthEnable = true;
	depthstencildesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
	depthstencildesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

	hr = this->device->CreateDepthStencilState(&depthstencildesc, this->depthStencilState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil state.");
		return false;
	}

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width; 
	viewport.Height = height;  
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f; 

	this->deviceContext->RSSetViewports(1, &viewport); 

	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID; 
	rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	hr = this->device->CreateRasterizerState(&rasterizerDesc, this->rasterizerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create rasterizer state.");
		return false;
	}

	spriteBatch = std::make_unique<DirectX::SpriteBatch>(this->deviceContext.Get()); 
	spriteFont = std::make_unique<DirectX::SpriteFont>(this->device.Get(), L"Data\\Fonts\\consolas_16.spritefont");  

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = this->device->CreateSamplerState(&sampDesc, this->samplerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create sampler state.");
		return false;
	}

	return true;
}

bool Graphics::InitializeShaders()
{

	std::wstring shaderfolder = L"";
#pragma region ShaderPath
	if (IsDebuggerPresent() == TRUE) 
	{
#ifdef _DEBUG
    #ifdef _WIN64
			shaderfolder = L"..\\x64\\Debug\\"; 
    #else 
			shaderfolder = L"..\\Debug\\";  
    #endif
    #else
    #ifdef _WIN64  
			shaderfolder = L"..\\x64\\Release\\";
    #else
			shaderfolder = L"..\\Release\\";
    #endif
#endif
	}

	D3D11_INPUT_ELEMENT_DESC layout[] = 
	{
		{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0},  
		{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0}, 
	};

	UINT numElements = ARRAYSIZE(layout); 

	if (!vertexshader.Initialize(this->device, shaderfolder + L"vertexshader.cso", layout, numElements))     
		return false; 

	if (!pixelshader.Initialize(this->device, shaderfolder + L"pixelshader.cso"))
		return false; 


	return true;
}

bool Graphics::InitializeScene()
{
	Vertex v[] = 
	{
	    Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f), 
		Vertex(-0.5f, 0.5f, 1.0f, 0.0f, 0.0f), 
		Vertex(0.5f, 0.5f, 1.0f, 1.0f, 0.0f), 
		Vertex(0.5f, -0.5f, 1.0f, 1.0f, 1.0f), 
	};

	HRESULT hr = this->vertexBuffer.Initialize(this->device.Get(), v, ARRAYSIZE(v)); 
	if (FAILED(hr)) 
	{
		ErrorLogger::Log(hr, "Failed to create vertex buffer."); 
		return false;
	}

	DWORD indices[] = 
	{
		0, 1, 2,
		0, 2, 3
	}; 

	hr = this->indicesBuffer.Initialize(this->device.Get(), indices, ARRAYSIZE(indices)); 
	if (FAILED(hr))  
	{
		ErrorLogger::Log(hr, "Failed to create indices buffer."); 
		return hr; 
	}

	hr = DirectX::CreateWICTextureFromFile(this->device.Get(), L"Data\\Textures\\tex_1.png", nullptr, myTexture.GetAddressOf());  
	if (FAILED(hr)) 
	{
		ErrorLogger::Log(hr, "Failed to create texture from file."); 
		return false;
	}

	return true;
}
