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
        enum { FRAME_BUFFER_COUNT = 2 }; // �_�u���o�b�t�@�����O����̂�2

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

    /*------------EngineInit()�ɕK�v�ȃ��\�b�h��------------*/
#pragma region EngineInit
    private:
        // DirectX12�������Ɏg���֐�
        bool CreateDevice();        // �f�o�C�X�𐶐�
        bool CreateCommandQueue();  // �R�}���h�L���[�𐶐�
        bool CreateSwapChain();     // �X���b�v�`�F�C���𐶐�
        void CreateViewPort();      // �r���[�|�[�g�𐶐�
        void CreateScissorRect();   // �V�U�[��`�𐶐�
        bool CreateCommandList();   // �R�}���h���X�g�ƃR�}���h�A���P�[�^�[�𐶐�
        bool CreateFence();         // �t�F���X�𐶐�

        // DirectX12�������Ɏg���ϐ��C�I�u�W�F�N�g
        UINT dxgiFactoryFlags = 0;
        UINT m_CurrentBackBufferIndex = 0;
        UINT m_FrameBufferWidth = 0;
        UINT m_FrameBufferHeight = 0;
        UINT64 m_fenceValue[FRAME_BUFFER_COUNT]; // �t�F���X�̒l�i�_�u���o�b�t�@�����O�p��2�j

        // Pipeline objects.
        winrt::com_ptr<IDXGIFactory4> factory;
        winrt::com_ptr<ID3D12Device6> m_device = nullptr;
        winrt::com_ptr<ID3D12CommandQueue> m_commandQueue;
        winrt::com_ptr<IDXGISwapChain3> m_swapChain = nullptr; // �X���b�v�`�F�C��
        D3D12_VIEWPORT m_Viewport;  // �r���[�|�[�g
        D3D12_RECT m_Scissor;       // �V�U�[��`
        winrt::com_ptr<ID3D12CommandAllocator> m_commandAllocator[FRAME_BUFFER_COUNT] = { nullptr }; // �R�}���h�A���P�[���[
        winrt::com_ptr<ID3D12GraphicsCommandList> m_commandList = nullptr; // �R�}���h���X�g
        winrt::com_ptr<ID3D12Fence> m_fence = nullptr; // �t�F���X
        HANDLE m_fenceEvent = nullptr; // �t�F���X�Ŏg���C�x���g


        // �`��Ɏg���I�u�W�F�N�g�Ƃ��̐����֐�
        bool CreateRenderTarget();  // �����_�[�^�[�Q�b�g�𐶐�
        bool CreateDepthStencil();  // �[�x�X�e���V���o�b�t�@�𐶐�

        UINT m_rtvDescriptorSize;       //�����_�[�^�[�Q�b�g�r���[�̃f�B�X�N���v�^�T�C�Y
        UINT m_dsvDescriptorSize = 0;   // �[�x�X�e���V���̃f�B�X�N���v�^�[�T�C�Y

        winrt::com_ptr<ID3D12Resource> m_renderTargets[FRAME_BUFFER_COUNT] = { nullptr }; // �����_�[�^�[�Q�b�g�i�_�u���o�b�t�@�����O����̂�2�j
        winrt::com_ptr<ID3D12DescriptorHeap> m_rtvHeap;
        winrt::com_ptr<ID3D12DescriptorHeap> m_dsvHeap = nullptr; // �[�x�X�e���V���̃f�B�X�N���v�^�q�[�v
        winrt::com_ptr<ID3D12Resource> m_depthStencilBuffer = nullptr; // �[�x�X�e���V���o�b�t�@�i��������1�ł����j
#pragma endregion EngineInit

    /*------------SceneInit()�ɕK�v�ȃ��\�b�h��------------*/
#pragma region SceneInit
        /*--------VertexBuffer--------*/
    private :
        void VertexBuffer(size_t size, size_t stride, const void* pInitData); // �R���X�g���N�^�Ńo�b�t�@�𐶐�
        D3D12_VERTEX_BUFFER_VIEW VBView() const; // ���_�o�b�t�@�r���[���擾
        bool VBIsValid(); // �o�b�t�@�̐����ɐ������������擾

        bool m_VBIsValid = false; // �o�b�t�@�̐����ɐ������������擾
        winrt::com_ptr<ID3D12Resource> m_pVBuffer = nullptr; // �o�b�t�@
        D3D12_VERTEX_BUFFER_VIEW m_VBView = {}; // ���_�o�b�t�@�r���[
    
        /*--------ConstantBuffer--------*/
    private :
        void ConstantBuffer(size_t size, size_t i); // �R���X�g���N�^�Œ萔�o�b�t�@�𐶐�
        bool CBIsValid(size_t i); // �o�b�t�@�����ɐ�����������Ԃ�
        D3D12_GPU_VIRTUAL_ADDRESS CBGetAddress(size_t i) const; // �o�b�t�@��GPU��̃A�h���X��Ԃ�
        D3D12_CONSTANT_BUFFER_VIEW_DESC CBViewDesc(size_t i); // �萔�o�b�t�@�r���[��Ԃ�
        void* CBGetPtr(size_t i) const; // �萔�o�b�t�@�Ƀ}�b�s���O���ꂽ�|�C���^��Ԃ�

        template<typename T>
        T* CBGetPtr(size_t i)
        {
            return reinterpret_cast<T*>(CBGetPtr(i));
        }

        bool m_CBIsValid[FRAME_BUFFER_COUNT] = { false }; // �萔�o�b�t�@�����ɐ���������
        winrt::com_ptr<ID3D12Resource> m_pCBuffer[FRAME_BUFFER_COUNT]; // �萔�o�b�t�@
        D3D12_CONSTANT_BUFFER_VIEW_DESC m_CBVDesc[FRAME_BUFFER_COUNT]; // �萔�o�b�t�@�r���[�̐ݒ�
        void* m_pCBMappedPtr[FRAME_BUFFER_COUNT] = { nullptr };

        /*--------RootSignature--------*/
    private :
        void RootSignature(); // �R���X�g���N�^�Ń��[�g�V�O�l�`���𐶐�
        bool RSIsValid(); // ���[�g�V�O�l�`���̐����ɐ����������ǂ�����Ԃ�
        ID3D12RootSignature* RSGet(); // ���[�g�V�O�l�`����Ԃ�

        bool m_RSIsValid = false; // ���[�g�V�O�l�`���̐����ɐ����������ǂ���
    public :
        winrt::com_ptr<ID3D12RootSignature> m_pRootSignature = nullptr; // ���[�g�V�O�l�`��
        
        /*--------PipelineState--------*/
    private :
        void PipelineState(); // �R���X�g���N�^�ł�����x�̐ݒ������
        bool PSIsValid(); // �����ɐ����������ǂ�����Ԃ�

        void SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout); // ���̓��C�A�E�g��ݒ�
        void SetRootSignature(ID3D12RootSignature* rootSignature); // ���[�g�V�O�l�`����ݒ�
        void SetVS(std::wstring filePath); // ���_�V�F�[�_�[��ݒ�
        void SetPS(std::wstring filePath); // �s�N�Z���V�F�[�_�[��ݒ�
        void PSCreate(); // �p�C�v���C���X�e�[�g�𐶐�

        bool m_PSIsValid = false; // �����ɐ����������ǂ���
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psdesc = {}; // �p�C�v���C���X�e�[�g�̐ݒ�
        winrt::com_ptr<ID3D12PipelineState> m_pipelineState = nullptr; // �p�C�v���C���X�e�[�g
        winrt::com_ptr<ID3DBlob> m_pVsBlob; // ���_�V�F�[�_�[
        winrt::com_ptr<ID3DBlob> m_pPSBlob; // �s�N�Z���V�F�[�_�[
#pragma endregion SceneInit
    
    /*------------MainLoop()�ɕK�v�ȃ��\�b�h��------------*/
    private:
        //Update()
        float rotateY = 0;
        ID3D12Resource* m_currentRenderTarget = nullptr; // ���݂̃t���[���̃����_�[�^�[�Q�b�g���ꎞ�I�ɕۑ����Ă����֐�

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
