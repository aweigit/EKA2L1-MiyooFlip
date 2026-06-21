#include "switchelement.h"
#include "layoutcontext.h"

namespace lunasvg {

SwitchElement::SwitchElement()
    : GraphicsElement(ElementID::Switch)
{
}

void SwitchElement::layout(LayoutContext* context, LayoutContainer* current) const
{
    if(isDisplayNone())
        return;

    auto group = makeUnique<LayoutGroup>();
    group->transform = transform();
    group->opacity = opacity();
    group->masker = context->getMasker(mask());
    group->clipper = context->getClipper(clip_path());
    layoutChildren(context, group.get());
    current->addChildIfNotEmpty(std::move(group));
}

std::unique_ptr<Node> SwitchElement::clone() const
{
    return cloneElement<SwitchElement>();
}

} // namespace lunasvg
