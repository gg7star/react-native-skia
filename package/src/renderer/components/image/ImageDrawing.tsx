import type { ReactNode } from "react";
import React from "react";

import type { SkRect } from "../../../skia";
import { Skia, TileMode, FilterMode } from "../../../skia";
import { createDrawing } from "../../nodes";
import type { SkEnum } from "../../processors";
import { enumKey } from "../../processors";

interface ImageDrawingProps {
  tx: SkEnum<typeof TileMode>;
  ty: SkEnum<typeof TileMode>;
  fm: SkEnum<typeof FilterMode>;
  rect: SkRect;
  children?: ReactNode | ReactNode[];
}

const onDraw = createDrawing<ImageDrawingProps>(
  (ctx, { tx, ty, fm, rect }, node) => {
    if (node.paint === undefined) {
      const recorder = Skia.PictureRecorder();
      const canvas = recorder.beginRecording(rect);
      node.visit({
        ...ctx,
        canvas,
      });
      const pic = recorder.finishRecordingAsPicture();
      const shaderPaint = Skia.Paint();
      shaderPaint.setShader(
        pic.makeShader(
          TileMode[enumKey(tx)],
          TileMode[enumKey(ty)],
          FilterMode[enumKey(fm)]
        )
      );
      node.paint = shaderPaint;
    }
    if (node.paint) {
      ctx.canvas.drawRect(rect, node.paint);
    }
  }
);

export const ImageDrawing = (props: ImageDrawingProps) => {
  return <skDrawing onDraw={onDraw} skipProcessing {...props} />;
};

ImageDrawing.defaultProps = {
  tx: "decal",
  ty: "decal",
  fm: "nearest",
} as const;
