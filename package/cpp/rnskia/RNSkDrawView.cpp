//
// Created by Christian Falch on 23/08/2021.
//

#include "RNSkDrawView.h"

#include <chrono>

#include "RNSkLog.h"
#include "RNSkMeasureTime.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include <SkCanvas.h>
#include <SkFont.h>
#include <SkGraphics.h>
#include <SkPaint.h>
#include <SkString.h>

#pragma clang diagnostic pop

namespace RNSkia {

using namespace std::chrono;

void RNSkDrawView::setDrawCallback(std::shared_ptr<jsi::Function> callback) {

  _originalCallback = callback;
  _callback = nullptr;

  if (_originalCallback == nullptr) {
    // We can just reset everything - this is a signal that we're done.
    endDrawingLoop();
    return;
  }

  try {
    // Install as worklet if necessary
    jsi::HostFunctionType callbackFunction =
        [callback](jsi::Runtime &rt, const jsi::Value &thisVal,
                   const jsi::Value *args, size_t count) -> jsi::Value {
      if (thisVal.isObject()) {
        return callback->callWithThis(rt, thisVal.asObject(rt), args, count);
      } else {
        return callback->call(rt, args, count);
      }
    };

    // Set up timing
    auto timingInfo = std::make_shared<RNSkTimingInfo>();
    timingInfo->lastTimeStamp = -1;
    timingInfo->lastDurationIndex = 0;
    timingInfo->lastDurationsCount = 0;

    auto callbackRuntime = _platformContext->getJsRuntime();

    // Create draw callback wrapper
    _callback = std::make_shared<RNSkDrawCallback>(
        [this, callbackRuntime, callback, callbackFunction,
         timingInfo](std::shared_ptr<JsiSkCanvas> canvas, int width, int height,
                     double timestamp, RNSkPlatformContext *context) {
          auto start = high_resolution_clock::now();

          // First argument is the canvas
          auto canvasObj =
              jsi::Object::createFromHostObject(*callbackRuntime, canvas);

          // Second argument should be the height/width
          auto infoObj = jsi::Object(*callbackRuntime);
          infoObj.setProperty(*callbackRuntime, "width", width);
          infoObj.setProperty(*callbackRuntime, "height", height);

          // Now the timestamp if available
          infoObj.setProperty(*callbackRuntime, "timestamp", timestamp);
          double delta = 0;
          if (timingInfo->lastTimeStamp > -1) {
            delta = timestamp - timingInfo->lastTimeStamp;
          }
          timingInfo->lastTimeStamp = timestamp;

          infoObj.setProperty(*callbackRuntime, "delta", delta);
          infoObj.setProperty(*callbackRuntime, "fps", 1.0 / delta);

          // To be able to call the drawing function we'll wrap it once again
          auto p = jsi::Function::createFromHostFunction(
              *callbackRuntime,
              jsi::PropNameID::forUtf8(*callbackRuntime, "fn"), 0,
              [&callbackFunction](jsi::Runtime &rt, const jsi::Value &thisVal,
                                  const jsi::Value *args,
                                  size_t count) -> jsi::Value {
                return callbackFunction(rt, thisVal, args, count);
              });

          // Call draw callback js function
          p.callWithThis(*callbackRuntime, p, canvasObj, infoObj);

          // Draw debug overlays
          if (_showDebugOverlay) {
            // Stop clock
            auto stop = high_resolution_clock::now();

            // Calculate duration
            auto duration = duration_cast<milliseconds>(stop - start).count();

            // Average duration
            timingInfo->lastDurations[timingInfo->lastDurationIndex++] =
                duration;
            if (timingInfo->lastDurationIndex == LAST_DURATION_COUNT) {
              timingInfo->lastDurationIndex = 0;
            }

            if (timingInfo->lastDurationsCount < LAST_DURATION_COUNT) {
              timingInfo->lastDurationsCount++;
            }

            long average = 0;
            for (size_t i = 0; i < timingInfo->lastDurationsCount; i++) {
              average += timingInfo->lastDurations[i];
            }
            average = average / timingInfo->lastDurationsCount;

            auto debugString = std::to_string(average) + "ms";

            if (_drawingMode == RNSkDrawingMode::Continuous) {
              debugString +=
                  " " + std::to_string((size_t)round(1.0 / delta)) + "fps";
            }

            if (_isWorklet) {
              debugString += " worklet";
            }

            if (_drawingMode == RNSkDrawingMode::Continuous) {
              debugString += "/continuous";
            } else {
              debugString += "/default";
            }

            auto font = SkFont();
            font.setSize(16);
            auto paint = SkPaint();
            paint.setColor(SkColors::kRed);

            _jsiCanvas->getCanvas()->drawSimpleText(
                debugString.c_str(), debugString.size(), SkTextEncoding::kUTF8,
                18, 18, font, paint);
          }
        });

    // Request redraw
    requestRedraw();

  } catch (const jsi::JSError &err) {
    return _platformContext->raiseError(err);
  } catch (const std::exception &err) {
    return _platformContext->raiseError(err);
  } catch (...) {
    return _platformContext->raiseError(
        "An unknown error occured when installing the draw callback.");
  }
}

void RNSkDrawView::drawInSurface(sk_sp<SkSurface> surface, int width,
                                 int height, double time,
                                 RNSkPlatformContext *context) {

  // auto measure = RNSkMeasureTime("RNSkDrawView::drawInSurface");

  try {
    // Get the canvas
    auto skCanvas = surface->getCanvas();
    _jsiCanvas->setCanvas(skCanvas);

    // Call the draw callback and perform js based drawing
    if (_callback != nullptr) {
      // Make sure to scale correctly
      auto pd = context->getPixelDensity();
      skCanvas->save();
      skCanvas->scale(pd, pd);
      // Call draw function
      ((RNSkDrawCallback)(*_callback))(
                      _jsiCanvas, width / pd, height / pd, time, context);
      // Restore canvas
      skCanvas->restore();
      skCanvas->flush();
    }
  } catch (const jsi::JSError &err) {
    _callback = nullptr;
    return _platformContext->raiseError(err);
  } catch (const std::exception &err) {
    _callback = nullptr;
    return _platformContext->raiseError(err);
  } catch (const std::runtime_error &err) {
    _callback = nullptr;
    return _platformContext->raiseError(err);
  } catch (...) {
    _callback = nullptr;
    return _platformContext->raiseError(
        "An error occured while rendering the Skia View.");
  }
}

void RNSkDrawView::requestRedraw() {
  if (!isReadyToDraw()) {
    return;
  }

  _isDrawing = true;

  auto performDraw = [this]() {
    if (_drawingMode == RNSkDrawingMode::Continuous) {
      _isDrawing = false;
      beginDrawingLoop();
      return;
    }

    try {
      if (_platformContext != nullptr) {
        drawFrame(-1);
      }
    } catch (...) {
      _isDrawing = false;
      throw;
    }

    _isDrawing = false;
  };

  _platformContext->runOnJavascriptThread(performDraw);
}

bool RNSkDrawView::isReadyToDraw() {
  if (_isDrawing) {
    return false;
  }

  if (_platformContext == nullptr) {
    endDrawingLoop();
    return false;
  }

  if (_callback == nullptr) {
    endDrawingLoop();
    return false;
  }

  return true;
}

void RNSkDrawView::beginDrawingLoop() {
  if (_platformContext == nullptr) {
    return;
  }

  if (_drawingLoopIdentifier != -1 || _platformContext == nullptr) {
    return;
  }
  
  // Set to zero to avoid calling beginDrawLoop before we return
  _drawingLoopIdentifier = 0;
  _drawingLoopIdentifier =
      _platformContext->beginDrawLoop([this](double timestamp) {
        auto performDraw = [=]() {
          if (!isReadyToDraw()) {
            return;
          }

          _isDrawing = true;

          try {
            if (_platformContext != nullptr) {
              drawFrame(timestamp);
            }
          } catch (...) {
            _isDrawing = false;
            throw;
          }

          _isDrawing = false;
        };

        _platformContext->runOnJavascriptThread(performDraw);
      });
}

void RNSkDrawView::endDrawingLoop() {
  if (_platformContext == nullptr) {
    return;
  }
  
  if (_drawingLoopIdentifier == -1) {
    return;
  }
  
  _platformContext->endDrawLoop(_drawingLoopIdentifier);
  _drawingLoopIdentifier = -1;
}

void RNSkDrawView::setDrawingMode(RNSkDrawingMode mode) {
  endDrawingLoop();
  _drawingMode = mode;
  requestRedraw();
}

} // namespace RNSkia
