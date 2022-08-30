#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <JsiHostObject.h>
#include <JsiValueWrapper.h>
#include <RNSkDrawView.h>
#include <RNSkPlatformContext.h>
#include <RNSkValue.h>
#include <jsi/jsi.h>

namespace RNSkia {
using namespace facebook;

using RNSkViewInfo = struct RNSkViewInfo {
  RNSkViewInfo() {
    view = nullptr;
  }
  std::shared_ptr<RNSkDrawView> view;
  std::unordered_map<std::string, JsiValueWrapper> props;
};

class RNSkJsiViewApi : public JsiHostObject, public std::enable_shared_from_this<RNSkJsiViewApi> {
public:
  /**
   Sets a custom property on a view given a view id. The property name/value will
   be stored in a map alongside the id of the view and propagated to the view when
   needed.
   */
  JSI_HOST_FUNCTION(setCustomProperty) {
    if (count != 3) {
      _platformContext->raiseError(
          std::string("setCustomProperty: Expected 3 arguments, got " +
                      std::to_string(count) + "."));
      return jsi::Value::undefined();
    }

    if (!arguments[0].isNumber()) {
      _platformContext->raiseError(
          "setCustomProperty: First argument must be a number");
      return jsi::Value::undefined();
    }
    
    if (!arguments[1].isString()) {
      _platformContext->raiseError(
          "setCustomProperty: Second argument must be the name of the property to set.");
      
      return jsi::Value::undefined();
    }
    auto nativeId = arguments[0].asNumber();    
    auto info = getEnsuredViewInfo(nativeId);
    info->props.emplace(arguments[1].asString(runtime).utf8(runtime), JsiValueWrapper(runtime, arguments[2]));

    // Now let's see if we have a view that we can update
    if(info->view != nullptr) {
      // Update view!
      info->view->setNativeId(nativeId);
      info->view->setCustomProps(info->props);
    }
    
    return jsi::Value::undefined();
  }
  
  /**
   Calls a custom command / method on a view by the view id.
   */
  JSI_HOST_FUNCTION(callCustomAction) {
    if (count < 2) {
      _platformContext->raiseError(
          std::string("callCustomCommand: Expected at least 2 arguments, got " +
                      std::to_string(count) + "."));
      
      return jsi::Value::undefined();
    }

    if (!arguments[0].isNumber()) {
      _platformContext->raiseError(
          "callCustomCommand: First argument must be a number");
      
      return jsi::Value::undefined();
    }
    
    if (!arguments[1].isString()) {
      _platformContext->raiseError(
          "callCustomCommand: Second argument must be the name of the action to call.");
      
      return jsi::Value::undefined();
    }
    
    auto nativeId = arguments[0].asNumber();
    auto action = arguments[1].asString(runtime).utf8(runtime);
    
    auto info = getEnsuredViewInfo(nativeId);
    
    if(info->view == nullptr) {
      jsi::detail::throwJSError(runtime,
          std::string("callCustomCommand: Could not call action " + action +
                      " on view - view not ready.").c_str());
      
      return jsi::Value::undefined();
    }

    // Get arguments
    size_t paramsCount = count - 2;
    const jsi::Value* params = paramsCount > 0 ? &arguments[2] : nullptr;
    return info->view->callCustomAction(runtime, action, params, paramsCount);
  }
  
  JSI_HOST_FUNCTION(registerValuesInView) {
      // Check params
      if(!arguments[1].isObject() || !arguments[1].asObject(runtime).isArray(runtime)) {
        jsi::detail::throwJSError(runtime, "Expected array of Values as second parameter");
        return jsi::Value::undefined();
      }
      
      // Get identifier of native SkiaView
      int nativeId = arguments[0].asNumber();
      
      // Get values that should be added as dependencies
      auto values = arguments[1].asObject(runtime).asArray(runtime);
      std::vector<std::function<void()>> unsubscribers;
      const std::size_t size = values.size(runtime);
      unsubscribers.reserve(size);
      for(size_t i=0; i<size; ++i) {
        auto value = values.getValueAtIndex(runtime, i).asObject(runtime).asHostObject<RNSkReadonlyValue>(runtime);
        
        if(value != nullptr) {
          // Add change listener
          unsubscribers.push_back(value->addListener([weakSelf = weak_from_this(), nativeId](jsi::Runtime&){
            auto self = weakSelf.lock();
            if(self) {
              auto info = self->getEnsuredViewInfo(nativeId);
              if(info->view != nullptr) {
                info->view->requestRedraw();
              }
            }
          }));
        }
      }
      
      // Return unsubscribe method that unsubscribes to all values
      // that we subscribed to.
      return jsi::Function::createFromHostFunction(runtime,
                                                   jsi::PropNameID::forUtf8(runtime, "unsubscribe"),
                                                   0,
                                                   JSI_HOST_FUNCTION_LAMBDA {
        // decrease dependency count on the Skia View
        for(auto &unsub : unsubscribers) {
          unsub();
        }
        return jsi::Value::undefined();
      });
    }
      
  JSI_EXPORT_FUNCTIONS(JSI_EXPORT_FUNC(RNSkJsiViewApi, setCustomProperty),
                       JSI_EXPORT_FUNC(RNSkJsiViewApi, callCustomAction),
                       JSI_EXPORT_FUNC(RNSkJsiViewApi, registerValuesInView))

  /**
   * Constructor
   * @param platformContext Platform context
   */
  RNSkJsiViewApi(std::shared_ptr<RNSkPlatformContext> platformContext)
      : JsiHostObject(), _platformContext(platformContext) {}

  /**
   * Invalidates the Skai View Api object
   */
  void invalidate() {
    unregisterAll();
  }

  /**
   Call to remove all draw view infos
   */
  void unregisterAll() {
    // Unregister all views
    auto tempList = _viewInfos;
    for (const auto& info : tempList) {
      unregisterSkiaDrawView(info.first);
    }
    _viewInfos.clear();
  }

  /**
   * Registers a skia view
   * @param nativeId Id of view to register
   * @param view View to register
   */
  void registerSkiaDrawView(size_t nativeId, std::shared_ptr<RNSkDrawView> view) {
    auto info = getEnsuredViewInfo(nativeId);
    info->view = view;
    info->view->setNativeId(nativeId);
    info->view->setCustomProps(info->props);
  }

  /**
   * Unregisters a Skia draw view
   * @param nativeId View id
   */
  void unregisterSkiaDrawView(size_t nativeId) {
    if (_viewInfos.count(nativeId) == 0) {
      return;
    }
    auto info = getEnsuredViewInfo(nativeId);
    info->view = nullptr;
    
    _viewInfos.erase(nativeId);
  }
  
  /**
   Sets a skia draw view for the given id. This function can be used
   to mark that an underlying SkiaView is not available (it could be
   removed due to ex. a transition). The view can be set to a nullptr
   or a valid view, effectively toggling the view's availability.
   */
  void setSkiaDrawView(size_t nativeId, std::shared_ptr<RNSkDrawView> view) {
    if (_viewInfos.find(nativeId) == _viewInfos.end()) {
      return;
    }
    auto info = getEnsuredViewInfo(nativeId);
    if (view != nullptr) {
      info->view = view;
      info->view->setNativeId(nativeId);
      info->view->setCustomProps(info->props);      
    } else if(view == nullptr) {
      info->view = view;
    }
  }

private:
  /**
   * Creates or returns the callback info object for the given view
   * @param nativeId View id
   * @return The callback info object for the requested view
   */
  RNSkViewInfo *getEnsuredViewInfo(size_t nativeId) {
    if (_viewInfos.count(nativeId) == 0) {
      RNSkViewInfo info;
      _viewInfos.emplace(nativeId, info);
    }
    return &_viewInfos.at(nativeId);
  }
  
  // List of callbacks
  std::unordered_map<size_t, RNSkViewInfo> _viewInfos;
  
  // Platform context
  std::shared_ptr<RNSkPlatformContext> _platformContext;
};
} // namespace RNSkia
