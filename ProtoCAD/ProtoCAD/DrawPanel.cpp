#include "pch.h"
#include "DrawPanel.h"
#if __has_include("DrawPanel.g.cpp")
#include "DrawPanel.g.cpp"
#endif

#include <microsoft.ui.xaml.media.dxinterop.h>

#include "SharedStruct.h"

using namespace DirectX;
using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::ProtoCAD::implementation
{
    DrawPanel::DrawPanel()
    {
        DefaultStyleKey(winrt::box_value(L"ProtoCAD.DrawPanel"));

		__super::OnApplyTemplate();
		m_sCPanel = GetTemplateChild(L"drawSCPanel").as<Microsoft::UI::Xaml::Controls::SwapChainPanel>();

        if (!EngineInit())
        {
            return;
        }

        if (!SceneInit())
        {
            return;
        }

        MainLoop();
    }

	void DrawPanel::MainLoop()
	{
		MSG msg = {};
		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE == true))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Update();
				BeginRender();
				Draw();
				EndRender();
			}
		}
	}

	winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel DrawPanel::getPanel()
	{
		return GetTemplateChild(L"drawSCPanel").as<Microsoft::UI::Xaml::Controls::SwapChainPanel>();
	}

	bool DrawPanel::EngineInit()
	{
		if (!CreateDevice())
		{
			//デバイスの生成に失敗
			return false;
		}

		if (!CreateCommandQueue())
		{
			//コマンドキューの生成に失敗
			return false;
		}

		if (!CreateSwapChain())
		{
			//スワップチェインの生成に失敗
			return false;
		}

		CreateViewPort();
		CreateScissorRect();

		if (!CreateRenderTarget())
		{
			//レンダーターゲットの生成に失敗
			return false;
		}

		if (!CreateCommandList())
		{
			//コマンドリストの生成に失敗
			return false;
		}

		if (!CreateFence())
		{
			//フェンスの生成に失敗
			return false;
		}

		if (!CreateDepthStencil())
		{
			//デプスステンシルバッファの生成に失敗
			return false;
		}

		return true;
	}

	bool DrawPanel::SceneInit()
	{
		Vertex vertices[3] = {};
		vertices[0].Position = XMFLOAT3(-1.0f, -1.0f, 0.0f);
		vertices[0].Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

		vertices[1].Position = XMFLOAT3(1.0f, -1.0f, 0.0f);
		vertices[1].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

		vertices[2].Position = XMFLOAT3(0.0f, 1.0f, 0.0f);
		vertices[2].Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

		auto vertexSize = sizeof(Vertex) * std::size(vertices);
		auto vertexStride = sizeof(Vertex);
		VertexBuffer(vertexSize, vertexStride, vertices);
		if (!VBIsValid())
		{
			//頂点バッファの生成に失敗
			return false;
		}

		auto eyePos = XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f); // 視点の位置
		auto targetPos = XMVectorZero(); // 視点を向ける座標
		auto upward = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 上方向を表すベクトル
		auto fov = XMConvertToRadians(37.5f); // 視野角
		auto aspect = static_cast<float>(50) / static_cast<float>(50); // アスペクト比

		for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			ConstantBuffer(sizeof(Transform), i);
			if (!CBIsValid(i))
			{
				//変換行列用定数バッファの生成に失敗
				return false;
			}

			// 変換行列の登録
			auto ptr = CBGetPtr<Transform>(i);
			ptr->World = XMMatrixIdentity();
			ptr->View = XMMatrixLookAtRH(eyePos, targetPos, upward);
			ptr->Proj = XMMatrixPerspectiveFovRH(fov, aspect, 0.3f, 1000.0f);
		}

		RootSignature();
		if (!RSIsValid())
		{
			//ルートシグネチャの生成に失敗
			return false;
		}

		///* pathたしかめたり
		//path p = current_path();
		//path p2 = absolute("../../GitLab/ProtoCADcpp2/x64/Debug/ProtoCAD/SimplePS.cso");
		//*/

		PipelineState();
		SetInputLayout(Vertex::InputLayout);
		SetRootSignature(m_pRootSignature.get());
		SetVS(L"../../GitLab/ProtoCADcpp/x64/Debug/ProtoCAD/SimplePS.cso");
		SetPS(L"../../GitLab/ProtoCADcpp/x64/Debug/ProtoCAD/SimplePS.cso");
		PSCreate();
		if (!PSIsValid())
		{
			//パイプラインステートの生成に失敗
			return false;
		}
		return true;
	}

#pragma region MainLoop
	void DrawPanel::Update()
	{
		if (m_sCPanel != nullptr)
		{
			com_ptr<ISwapChainPanelNative> panelNative;
			panelNative = m_sCPanel.as<ISwapChainPanelNative>();
			check_hresult(panelNative->SetSwapChain(m_swapChain.get()));
		}
		
		rotateY += 0.02f;
		auto currentIndex = m_CurrentBackBufferIndex;
		auto currentTransform = CBGetPtr<Transform>(currentIndex);
		currentTransform->World = DirectX::XMMatrixRotationY(rotateY);
	}

	void DrawPanel::BeginRender()
	{
		//現在のレンダーターゲットを更新
		m_currentRenderTarget = m_renderTargets[m_CurrentBackBufferIndex].get();

		//コマンドを初期化してためる準備をする
		m_commandAllocator[m_CurrentBackBufferIndex]->Reset();
		m_commandList->Reset(m_commandAllocator[m_CurrentBackBufferIndex].get(), m_pipelineState.get());

		// ビューポートとシザー矩形を設定
		m_commandList->RSSetViewports(1, &m_Viewport);
		m_commandList->RSSetScissorRects(1, &m_Scissor);

		// ? : Indicate that the back buffer will be used as a render target.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_CurrentBackBufferIndex].get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &barrier);

		// 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_rtvDescriptorSize);

		// 深度ステンシルのディスクリプタヒープの開始アドレス取得
		auto currentDsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

		// レンダーターゲットが使用可能になるまで待つ
		barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_CurrentBackBufferIndex].get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &barrier);

		// レンダーターゲットを設定
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &currentDsvHandle);

		// Record commands.// レンダーターゲットをクリア
		const float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// 深度ステンシルビューをクリア
		m_commandList->ClearDepthStencilView(currentDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	}

	void DrawPanel::Draw()
	{
		auto currentIndex = CurrentBackBufferIndex();
		auto commandList = CommandList(); // コマンドリスト
		auto vbView = VBView(); // 頂点バッファビュー

		commandList->SetGraphicsRootSignature(m_pRootSignature.get()); // ルートシグネチャをセット
		commandList->SetPipelineState(m_pipelineState.get()); // パイプラインステートをセット
		commandList->SetGraphicsRootConstantBufferView(0, CBGetAddress(currentIndex)); // 定数バッファをセット

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 三角形を描画する設定にする
		commandList->IASetVertexBuffers(0, 1, &vbView); // 頂点バッファをスロット0番を使って1個だけ設定する

		commandList->DrawInstanced(3, 1, 0, 0); // 3個の頂点を描画する
	}

	void DrawPanel::EndRender()
	{
		// レンダーターゲットに書き込み終わるまで待つ
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_currentRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &barrier);

		// コマンドの記録を終了
		m_commandList->Close();

		// コマンドを実行
		ID3D12CommandList* ppCmdLists[] = { m_commandList.get() };
		m_commandQueue->ExecuteCommandLists(1, ppCmdLists);

		// スワップチェーン切り替え
		m_swapChain->Present(1, 0);

		const UINT64 fence = m_fenceValue[m_CurrentBackBufferIndex];
		m_commandQueue->Signal(m_fence.get(), fence);
		m_fenceValue[m_CurrentBackBufferIndex]++;

		HRESULT hr;
		// Wait until the previous frame is finished.
		if (m_fence->GetCompletedValue() < fence)
		{
			//完了時にイベントを設定
			hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
			if (FAILED(hr))
			{
				return;
			}

			//待機処理
			if (WAIT_OBJECT_0 != WaitForSingleObject(m_fenceEvent, INFINITE))
			{
				return;
			}
		}

		m_CurrentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	}
#pragma endregion

#pragma region EngineInit
	bool DrawPanel::CreateDevice()
	{
		//check_hresult(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
		check_hresult(CreateDXGIFactory2(dxgiFactoryFlags, guid_of<IDXGIFactory4>(), factory.put_void()));

		com_ptr<IDXGIAdapter1> hardwareAdapter;

		//hardwareAdapter->Release();

		com_ptr<IDXGIAdapter1> adapter;

		com_ptr<IDXGIFactory6> factory6;

		//if (SUCCEEDED(factory.get()->QueryInterface(IID_PPV_ARGS(&factory6))))
		if (SUCCEEDED(factory.get()->QueryInterface(guid_of<IDXGIFactory6>(), factory6.put_void())))
		{
			for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, guid_of<IDXGIAdapter1>(), adapter.put_void()); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}
		else
		{
			for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, adapter.put()); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		*hardwareAdapter.put() = adapter.detach();

		HRESULT hr = D3D12CreateDevice(hardwareAdapter.get(),
			D3D_FEATURE_LEVEL_11_0,
			//IID_PPV_ARGS(&m_device));
			guid_of<ID3D12Device6>(),
			m_device.put_void());

		return SUCCEEDED(hr);
	}

	bool DrawPanel::CreateCommandQueue()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		HRESULT hr = m_device->CreateCommandQueue(&queueDesc, guid_of<ID3D12CommandQueue>(), m_commandQueue.put_void());
		return SUCCEEDED(hr);
	}

	bool DrawPanel::CreateSwapChain()
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
		swapChainDesc.Width = 500;
		swapChainDesc.Height = 500;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.SampleDesc.Count = 1;

		// When using XAML interop, the swap chain must be created for composition.
		com_ptr<IDXGISwapChain1> swapChain;
		check_hresult(factory->CreateSwapChainForComposition(
			m_commandQueue.get(), // Swap chain needs the queue so that it can force a flush on it.
			&swapChainDesc,
			nullptr,
			swapChain.put()));

		check_hresult(swapChain.as(guid_of<IDXGISwapChain3>(),m_swapChain.put_void()));
		m_CurrentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

		//bool hr = DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::High, [=]()
		//	{
		//		com_ptr<ISwapChainPanelNative> panelNative;
		//		//panelNative = swapChainPanel().as<ISwapChainPanelNative>();
		//		panelNative = m_sCPanel.as<ISwapChainPanelNative>();
		//		check_hresult(panelNative->SetSwapChain(m_swapChain.get())); 
		//	});

		//return hr;
		return true;
	}

	void DrawPanel::CreateViewPort()
	{
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = static_cast<float>(m_FrameBufferWidth);
		m_Viewport.Height = static_cast<float>(m_FrameBufferHeight);
		m_Viewport.MinDepth = 0.0f;
		m_Viewport.MaxDepth = 1.0f;
	}

	void DrawPanel::CreateScissorRect()
	{
		m_Scissor.left = 0;
		m_Scissor.right = m_FrameBufferWidth;
		m_Scissor.top = 0;
		m_Scissor.bottom = m_FrameBufferHeight;
	}

	bool DrawPanel::CreateCommandList()
	{
		HRESULT hr;
		for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			hr = m_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, 
				guid_of<ID3D12CommandAllocator>(), 
				m_commandAllocator[i].put_void());
		}

		if (FAILED(hr))
		{
			return false;
		}

		// Create the command list.
		hr = m_device->CreateCommandList(
			0, 
			D3D12_COMMAND_LIST_TYPE_DIRECT, 
			m_commandAllocator[m_CurrentBackBufferIndex].get(), 
			nullptr, guid_of<ID3D12GraphicsCommandList>(), m_commandList.put_void());

		if (FAILED(hr))
		{
			return false;
		}

		m_commandList->Close();

		return true;
	}

	bool DrawPanel::CreateFence()
	{
		for (auto i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			m_fenceValue[i] = 0;
		}

		// Create synchronization objects.
		HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, guid_of<ID3D12Fence>(), m_fence.put_void());
		if (FAILED(hr))
		{
			return false;
		}

		//m_fenceValue = 1;
		m_fenceValue[m_CurrentBackBufferIndex]++;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		return m_fenceEvent != nullptr;
	}

	bool DrawPanel::CreateRenderTarget()
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, guid_of<ID3D12DescriptorHeap>(), m_rtvHeap.put_void());

		if (FAILED(hr))
		{
			return false;
		}

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT n = 0; n < FRAME_BUFFER_COUNT; n++)
		{
			check_hresult(m_swapChain->GetBuffer(n, guid_of<ID3D12Resource>(), m_renderTargets[n].put_void()));
			m_device->CreateRenderTargetView(m_renderTargets[n].get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}

		return true;
	}

	bool DrawPanel::CreateDepthStencil()
	{
		//DSV用のディスクリプタヒープを作成する
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		//auto hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_dsvHeap));
		auto hr = m_device->CreateDescriptorHeap(&heapDesc, guid_of<ID3D12DescriptorHeap>(), m_dsvHeap.put_void());
		if (FAILED(hr))
		{
			return false;
		}

		//ディスクリプタのサイズを取得
		m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_CLEAR_VALUE dsvClearValue;
		dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		dsvClearValue.DepthStencil.Depth = 1.0f;
		dsvClearValue.DepthStencil.Stencil = 0;

		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resourceDesc(
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			500,
			500,
			1,
			1,
			DXGI_FORMAT_D32_FLOAT,
			1,
			0,
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
		hr = m_device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&dsvClearValue,
			guid_of<ID3D12Resource>(), m_depthStencilBuffer.put_void()
		);

		if (FAILED(hr))
		{
			return false;
		}

		//ディスクリプタを作成
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

		m_device->CreateDepthStencilView(m_depthStencilBuffer.get(), nullptr, dsvHandle);

		return true;
	}
#pragma endregion

#pragma region SceneInit

#pragma region VertexBuffer
	void DrawPanel::VertexBuffer(size_t size, size_t stride, const void* pInitData)
	{
		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); 	// ヒーププロパティ
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(size); 	// リソースの設定

		// リソースを生成
		auto hr = m_device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			//IID_PPV_ARGS(m_pVBuffer.put()));
			guid_of<ID3D12Resource>(), m_pVBuffer.put_void());

		if (FAILED(hr))
		{
			printf("頂点バッファリソースの生成に失敗");
			return;
		}

		// 頂点バッファビューの設定
		m_VBView.BufferLocation = m_pVBuffer->GetGPUVirtualAddress();
		m_VBView.SizeInBytes = static_cast<UINT>(size);
		m_VBView.StrideInBytes = static_cast<UINT>(stride);

		// マッピングする
		if (pInitData != nullptr)
		{
			void* ptr = nullptr;
			hr = m_pVBuffer->Map(0, nullptr, &ptr);
			if (FAILED(hr))
			{
				printf("頂点バッファマッピングに失敗");
				return;
			}

			// 頂点データをマッピング先に設定
			memcpy(ptr, pInitData, size);

			// マッピング解除
			m_pVBuffer->Unmap(0, nullptr);
		}

		m_VBIsValid = true;
	}

	D3D12_VERTEX_BUFFER_VIEW DrawPanel::VBView() const
	{
		return m_VBView;
	}

	bool DrawPanel::VBIsValid()
	{
		return m_VBIsValid;
	}
#pragma endregion VertexBuffer

#pragma region ConstantBuffer
	void DrawPanel::ConstantBuffer(size_t size, size_t i)
	{
		size_t align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		UINT64 sizeAligned = (size + (align - 1)) & ~(align - 1); // alignに切り上げる.

		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD); // ヒーププロパティ
		auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeAligned); // リソースの設定

		// リソースを生成
		auto hr = m_device->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			//IID_PPV_ARGS(m_pCBuffer[i].put()));
			guid_of<ID3D12Resource>(), m_pCBuffer[i].put_void());

		if (FAILED(hr))
		{
			//定数バッファリソースの生成に失敗
			return;
		}
		
		hr = m_pCBuffer[i]->Map(0, nullptr, &m_pCBMappedPtr[i]);

		if (FAILED(hr))
		{
			//定数バッファのマッピングに失敗
			return;
		}

		m_CBVDesc[i] = {};
		m_CBVDesc[i].BufferLocation = m_pCBuffer[i]->GetGPUVirtualAddress();
		m_CBVDesc[i].SizeInBytes = UINT(sizeAligned);

		m_CBIsValid[i] = true;
	}



	bool DrawPanel::CBIsValid(size_t i)
	{
		return m_CBIsValid[i];
	}

	D3D12_GPU_VIRTUAL_ADDRESS DrawPanel::CBGetAddress(size_t i) const
	{
		return m_CBVDesc[i].BufferLocation;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC DrawPanel::CBViewDesc(size_t i)
	{
		return m_CBVDesc[i];
	}

	void* DrawPanel::CBGetPtr(size_t i) const
	{
		return m_pCBMappedPtr[i];
	}

#pragma endregion ConstantBuffer

#pragma region RootSignature
	void DrawPanel::RootSignature()
	{
		auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // アプリケーションの入力アセンブラを使用する
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS; // ドメインシェーダーのルートシグネチャへんアクセスを拒否する
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS; // ハルシェーダーのルートシグネチャへんアクセスを拒否する
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS; // ジオメトリシェーダーのルートシグネチャへんアクセスを拒否する

		CD3DX12_ROOT_PARAMETER rootParam[1] = {};
		rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // b0の定数バッファを設定、全てのシェーダーから見えるようにする

		// スタティックサンプラーの設定
		auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		// ルートシグニチャの設定（設定したいルートパラメーターとスタティックサンプラーを入れる）
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = std::size(rootParam); // ルートパラメーターの個数をいれる
		desc.NumStaticSamplers = 1; // サンプラーの個数をいれる
		desc.pParameters = rootParam; // ルートパラメーターのポインタをいれる
		desc.pStaticSamplers = &sampler; // サンプラーのポインタを入れる
		desc.Flags = flag; // フラグを設定

		winrt::com_ptr<ID3DBlob> pBlob;
		winrt::com_ptr<ID3DBlob> pErrorBlob;

		// シリアライズ
		auto hr = D3D12SerializeRootSignature(
			&desc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			pBlob.put(),
			pErrorBlob.put());
		if (FAILED(hr))
		{
			printf("ルートシグネチャシリアライズに失敗");
			return;
		}

		// ルートシグネチャ生成
		hr = m_device->CreateRootSignature(
			0, // GPUが複数ある場合のノードマスク（今回は1個しか無い想定なので0）
			pBlob->GetBufferPointer(), // シリアライズしたデータのポインタ
			pBlob->GetBufferSize(), // シリアライズしたデータのサイズ
			//IID_PPV_ARGS(&m_pRootSignature));
			guid_of<ID3D12RootSignature>(), m_pRootSignature.put_void()); // ルートシグニチャ格納先のポインタ
		if (FAILED(hr))
		{
			printf("ルートシグネチャの生成に失敗");
			return;
		}

		m_RSIsValid = true;
	}

	bool DrawPanel::RSIsValid()
	{
		return m_RSIsValid;
	}

#pragma endregion RootSignature

#pragma region PipelineState
	void DrawPanel::PipelineState()
	{
		// パイプラインステートの設定
		psdesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // ラスタライザーはデフォルト
		psdesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングはなし
		psdesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // ブレンドステートもデフォルト
		psdesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // 深度ステンシルはデフォルトを使う
		psdesc.SampleMask = UINT_MAX;
		psdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形を描画
		psdesc.NumRenderTargets = 1; // 描画対象は1
		psdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psdesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psdesc.SampleDesc.Count = 1; // サンプラーは1
		psdesc.SampleDesc.Quality = 0;
	}

	bool DrawPanel::PSIsValid()
	{
		return m_PSIsValid;
	}

	void DrawPanel::SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout)
	{
		psdesc.InputLayout = layout;
	}

	void DrawPanel::SetRootSignature(ID3D12RootSignature* rootSignature)
	{
		psdesc.pRootSignature = rootSignature;
	}

	void DrawPanel::SetVS(std::wstring filePath)
	{
		// 頂点シェーダー読み込み
		auto hr = D3DReadFileToBlob(filePath.c_str(), m_pVsBlob.put());
		if (FAILED(hr))
		{
			//printf("頂点シェーダーの読み込みに失敗");
			return;
		}

		psdesc.VS = CD3DX12_SHADER_BYTECODE(m_pVsBlob.get());
	}

	void DrawPanel::SetPS(std::wstring filePath)
	{
		// ピクセルシェーダー読み込み
		auto hr = D3DReadFileToBlob(filePath.c_str(), m_pPSBlob.put());
		if (FAILED(hr))
		{
			//printf("ピクセルシェーダーの読み込みに失敗");
			return;
		}

		psdesc.PS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.get());
	}

	void DrawPanel::PSCreate()
	{
		// パイプラインステートを生成
		//auto hr = m_device->CreateGraphicsPipelineState(&psdesc, IID_PPV_ARGS(m_pipelineState.put()));
		auto hr = m_device->CreateGraphicsPipelineState(
			&psdesc, guid_of<ID3D12PipelineState>(),m_pipelineState.put_void());
		
		if (FAILED(hr))
		{
			//printf("パイプラインステートの生成に失敗");
			return;
		}

		m_PSIsValid = true;
	}

#pragma endregion PipelineState

#pragma endregion 

#pragma region MainLoop
	UINT DrawPanel::CurrentBackBufferIndex()
	{
		return m_CurrentBackBufferIndex;
	}

	ID3D12GraphicsCommandList* DrawPanel::CommandList()
	{
		return m_commandList.get();
	}
#pragma endregion
}
