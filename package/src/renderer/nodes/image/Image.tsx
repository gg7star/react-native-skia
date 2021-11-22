import { NodeType } from "../../Host";
import type { SkNode } from "../../Host";
import type { IImage } from "../../../skia";
import { useImage, ClipOp, Skia } from "../../../skia";

export interface UnresolvedImageProps {
  source: ReturnType<typeof require>;
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface ImageProps extends Omit<UnresolvedImageProps, "source"> {
  source: IImage;
}

export const Image = ({
  source,
  x,
  y,
  width,
  height,
}: UnresolvedImageProps) => {
  const image = useImage({ uri: source });
  if (image === null) {
    return null;
  }
  return <skImage source={image} x={x} y={y} width={width} height={height} />;
};

Image.defaultProps = {
  x: 0,
  y: 0,
  resizeMode: "contain",
};

export const ImageNode = (props: ImageProps): SkNode<NodeType.Image> => ({
  type: NodeType.Image,
  props,
  draw: ({ canvas, paint }, { source, x, y, width, height }) => {
    canvas.save();
    canvas.clipRect(
      Skia.XYWHRect(x, y, x + width, y + height),
      ClipOp.Intersect,
      true
    );
    canvas.drawImage(source, x, y, paint);
    canvas.restore();
  },
  children: [],
});
