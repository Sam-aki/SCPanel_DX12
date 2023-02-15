#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "DrawPanel.g.h"

namespace winrt::ProtoCAD::implementation
{
    struct DrawPanel : DrawPanelT<DrawPanel>
    {
        DrawPanel();

    public:
        enum { FRAME_BUFFER_COUNT = 2 }; // ダブルバッファリングするので2

    private:
        bool EngineInit();
        bool SceneInit();

        void MainLoop();

        void Update();
        void BeginRender();
        void Draw();
        void EndRender();

        winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel m_sCPanel{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel getPanel();

    /*------------EngineInit()に必要なメソッド他------------*/
#pragma region EngineInit
    private:
        // DirectX12初期化に使う関数
        bool CreateDevice();        // デバイスを生成
        bool CreateCommandQueue();  // コマンドキューを生成
        bool CreateSwapChain();     // スワップチェインを生成
        void CreateViewPort();      // ビューポートを生成
        void CreateScissorRect();   // シザー矩形を生成
        bool CreateCommandList();   // コマンドリストとコマンドアロケーターを生成
        bool CreateFence();         // フェンスを生成

        // DirectX12初期化に使う変数，オブジェクト
        UINT dxgiFactoryFlags = 0;
        UINT m_CurrentBackBufferIndex = 0;
        UINT m_FrameBufferWidth = 0;
        UINT m_FrameBufferHeight = 0;
        UINT64 m_fenceValue[FRAME_BUFFER_COUNT]; // フェンスの値（ダブルバッファリング用に2個）

        // Pipeline objects.
        winrt::com_ptr<IDXGIFactory4> factory;
        winrt::com_ptr<ID3D12Device6> m_device = nullptr;
        winrt::com_ptr<ID3D12CommandQueue> m_commandQueue;
        winrt::com_ptr<IDXGISwapChain3> m_swapChain = nullptr; // スワップチェイン
        D3D12_VIEWPORT m_Viewport;  // ビューポート
        D3D12_RECT m_Scissor;       // シザー矩形
        winrt::com_ptr<ID3D12CommandAllocator> m_commandAllocator[FRAME_BUFFER_COUNT] = { nullptr }; // コマンドアロケーたー
        winrt::com_ptr<ID3D12GraphicsCommandList> m_commandList = nullptr; // コマンドリスト
        winrt::com_ptr<ID3D12Fence> m_fence = nullptr; // フェンス
        HANDLE m_fenceEvent = nullptr; // フェンスで使うイベント


        // 描画に使うオブジェクトとその生成関数
        bool CreateRenderTarget();  // レンダーターゲットを生成
        bool CreateDepthStencil();  // 深度ステンシルバッファを生成

        UINT m_rtvDescriptorSize;       //レンダーターゲットビューのディスクリプタサイズ
        UINT m_dsvDescriptorSize = 0;   // 深度ステンシルのディスクリプターサイズ

        winrt::com_ptr<ID3D12Resource> m_renderTargets[FRAME_BUFFER_COUNT] = { nullptr }; // レンダーターゲット（ダブルバッファリングするので2個）
        winrt::com_ptr<ID3D12DescriptorHeap> m_rtvHeap;
        winrt::com_ptr<ID3D12DescriptorHeap> m_dsvHeap = nullptr; // 深度ステンシルのディスクリプタヒープ
        winrt::com_ptr<ID3D12Resource> m_depthStencilBuffer = nullptr; // 深度ステンシルバッファ（こっちは1つでいい）
#pragma endregion EngineInit

    /*------------SceneInit()に必要なメソッド他------------*/
#pragma region SceneInit
        /*--------VertexBuffer--------*/
    private :
        void VertexBuffer(size_t size, size_t stride, const void* pInitData); // コンストラクタでバッファを生成
        D3D12_VERTEX_BUFFER_VIEW VBView() const; // 頂点バッファビューを取得
        bool VBIsValid(); // バッファの生成に成功したかを取得

        bool m_VBIsValid = false; // バッファの生成に成功したかを取得
        winrt::com_ptr<ID3D12Resource> m_pVBuffer = nullptr; // バッファ
        D3D12_VERTEX_BUFFER_VIEW m_VBView = {}; // 頂点バッファビュー
    
        /*--------ConstantBuffer--------*/
    private :
        void ConstantBuffer(size_t size, size_t i); // コンストラクタで定数バッファを生成
        bool CBIsValid(size_t i); // バッファ生成に成功したかを返す
        D3D12_GPU_VIRTUAL_ADDRESS CBGetAddress(size_t i) const; // バッファのGPU上のアドレスを返す
        D3D12_CONSTANT_BUFFER_VIEW_DESC CBViewDesc(size_t i); // 定数バッファビューを返す
        void* CBGetPtr(size_t i) const; // 定数バッファにマッピングされたポインタを返す

        template<typename T>
        T* CBGetPtr(size_t i)
        {
            return reinterpret_cast<T*>(CBGetPtr(i));
        }

        bool m_CBIsValid[FRAME_BUFFER_COUNT] = { false }; // 定数バッファ生成に成功したか
        winrt::com_ptr<ID3D12Resource> m_pCBuffer[FRAME_BUFFER_COUNT]; // 定数バッファ
        D3D12_CONSTANT_BUFFER_VIEW_DESC m_CBVDesc[FRAME_BUFFER_COUNT]; // 定数バッファビューの設定
        void* m_pCBMappedPtr[FRAME_BUFFER_COUNT] = { nullptr };

        /*--------RootSignature--------*/
    private :
        void RootSignature(); // コンストラクタでルートシグネチャを生成
        bool RSIsValid(); // ルートシグネチャの生成に成功したかどうかを返す
        ID3D12RootSignature* RSGet(); // ルートシグネチャを返す

        bool m_RSIsValid = false; // ルートシグネチャの生成に成功したかどうか
    public :
        winrt::com_ptr<ID3D12RootSignature> m_pRootSignature = nullptr; // ルートシグネチャ
        
        /*--------PipelineState--------*/
    private :
        void PipelineState(); // コンストラクタである程度の設定をする
        bool PSIsValid(); // 生成に成功したかどうかを返す

        void SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout); // 入力レイアウトを設定
        void SetRootSignature(ID3D12RootSignature* rootSignature); // ルートシグネチャを設定
        void SetVS(std::wstring filePath); // 頂点シェーダーを設定
        void SetPS(std::wstring filePath); // ピクセルシェーダーを設定
        void PSCreate(); // パイプラインステートを生成

        bool m_PSIsValid = false; // 生成に成功したかどうか
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psdesc = {}; // パイプラインステートの設定
        winrt::com_ptr<ID3D12PipelineState> m_pipelineState = nullptr; // パイプラインステート
        winrt::com_ptr<ID3DBlob> m_pVsBlob; // 頂点シェーダー
        winrt::com_ptr<ID3DBlob> m_pPSBlob; // ピクセルシェーダー
#pragma endregion SceneInit
    
    /*------------MainLoop()に必要なメソッド他------------*/
    private:
        //Update()
        float rotateY = 0;
        ID3D12Resource* m_currentRenderTarget = nullptr; // 現在のフレームのレンダーターゲットを一時的に保存しておく関数

        //Draw()
        UINT CurrentBackBufferIndex();
        ID3D12GraphicsCommandList* CommandList();


    };
}

namespace winrt::ProtoCAD::factory_implementation
{
    struct DrawPanel : DrawPanelT<DrawPanel, implementation::DrawPanel>
    {
    };
}
