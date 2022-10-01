#pragma once

#include "JsiHostObject.h"

#include "base/JsiDependencyManager.h"

#include "nodes/JsiRectNode.h"
#include "nodes/JsiRRectNode.h"
#include "nodes/JsiGroupNode.h"

#include "nodes/JsiPaintNode.h"

namespace RNSkia
{

  using namespace facebook;

  class JsiDomApi : public JsiHostObject
  {
  public:
    JsiDomApi(std::shared_ptr<RNSkPlatformContext> context)
        : JsiHostObject()
    {
      installFunction("DependencyManager", JsiDependencyManager::createCtor(context));
      
      installFunction("RectNode", JsiRectNode::createCtor(context));
      installFunction("RRectNode", JsiRRectNode::createCtor(context));
      installFunction("GroupNode", JsiGroupNode::createCtor(context));
      installFunction("PaintNode", JsiPaintNode::createCtor(context));
    }
  };

}
