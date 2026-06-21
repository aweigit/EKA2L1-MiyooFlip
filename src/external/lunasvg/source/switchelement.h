#ifndef SWITCHELEMENT_H
#define SWITCHELEMENT_H

#include "graphicselement.h"

namespace lunasvg {

class SwitchElement : public GraphicsElement
{
public:
    SwitchElement();

    void layout(LayoutContext* context, LayoutContainer* current) const;
    std::unique_ptr<Node> clone() const;
};

} // namespace lunasvg

#endif // SWITCHELEMENT_H
