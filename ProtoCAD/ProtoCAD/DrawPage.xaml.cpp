#include "pch.h"
#include "DrawPage.xaml.h"
#if __has_include("DrawPage.g.cpp")
#include "DrawPage.g.cpp"
#endif

namespace winrt::ProtoCAD::implementation
{
    DrawPage::DrawPage()
    {
        InitializeComponent();
    }
}
