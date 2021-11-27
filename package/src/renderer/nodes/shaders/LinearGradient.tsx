import type { Vector } from "../../math/Vector";
import type { SkEnum } from "../processors/Paint";
import { processColor, enumKey } from "../processors/Paint";
import { TileMode, Skia } from "../../../skia";
import { useDeclaration } from "../Declaration";
import type { TransformProps } from "../processors/Transform";
import { localMatrix } from "../processors/Transform";

export interface LinearGradientProps extends TransformProps {
  start: Vector;
  end: Vector;
  positions?: number[];
  colors: string[];
  mode: SkEnum<typeof TileMode>;
  flags?: number;
}

export const LinearGradient = ({
  start,
  end,
  positions,
  colors,
  mode,
  flags,
  ...transformProps
}: LinearGradientProps) => {
  const onDeclare = useDeclaration(
    ({ opacity }) => {
      return Skia.Shader.MakeLinearGradient(
        start,
        end,
        colors.map((color) => processColor(color, opacity)),
        positions ?? null,
        TileMode[enumKey(mode)],
        localMatrix(transformProps),
        flags
      );
    },
    [colors, end, flags, mode, positions, start, transformProps]
  );
  return <skDeclaration onDeclare={onDeclare} />;
};

LinearGradient.defaultProps = {
  mode: "clamp",
};
