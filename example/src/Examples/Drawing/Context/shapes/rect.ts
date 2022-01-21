import type { IPaint } from "@shopify/react-native-skia";
import { Skia } from "@shopify/react-native-skia";

import type { DrawingElement } from "../types";

export const createRect = (
  x: number,
  y: number,
  currentPaint: IPaint
): DrawingElement => {
  const path = Skia.Path.Make();
  path.moveTo(x, y);
  path.lineTo(x + 1, y);
  path.lineTo(x + 1, y + 1);
  path.lineTo(x, y + 1);
  path.lineTo(x, y);
  return {
    type: "rectangle",
    primitive: path,
    p: currentPaint,
  };
};
