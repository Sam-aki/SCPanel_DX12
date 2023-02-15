#pragma once

#include "pch.h"
#include "DrawPage.g.h"

namespace winrt::ProtoCAD::implementation
{
    struct DrawPage : DrawPageT<DrawPage>
    {
        DrawPage();
    };
}

namespace winrt::ProtoCAD::factory_implementation
{
    struct DrawPage : DrawPageT<DrawPage, implementation::DrawPage>
    {
    };
}
