
#pragma once

#include "JsiDomRenderNode.h"

namespace RNSkia {

class JsiGroupNode: public JsiDomRenderNode {
public:
  JsiGroupNode(std::shared_ptr<RNSkPlatformContext> context,
              jsi::Runtime& runtime,
              const jsi::Value *arguments,
              size_t count):
  JsiDomRenderNode(context, runtime, arguments, count) {}
  
  static const jsi::HostFunctionType
  createCtor(std::shared_ptr<RNSkPlatformContext> context) {
    return JSI_HOST_FUNCTION_LAMBDA {
      auto rectNode = std::make_shared<JsiGroupNode>(context, runtime, arguments, count);
      return jsi::Object::createFromHostObject(runtime, std::move(rectNode));
    };
  }
  
protected:
  void onPropsRead(jsi::Runtime &runtime) override {
    JsiDomRenderNode::onPropsRead(runtime);
  }
  
  void renderNode(std::shared_ptr<JsiBaseDrawingContext> context) override {
    for(auto& node: getChildren()) {
      auto renderNode = std::dynamic_pointer_cast<JsiDomRenderNode>(node);
      if(renderNode != nullptr) {
        renderNode->render(context);
      }
    }
  }
  
  // FIXME: Add to enum and sync with JS somehow?
  const char* getType() override { return "skGroup"; }

private:
  SkRect _rect;
};

}
