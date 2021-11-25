import React from "react";
import { StyleSheet, Dimensions, ScrollView, PixelRatio } from "react-native";
import {
  Skia,
  PaintStyle,
  Canvas,
  Rect,
  DRect,
  Group,
  Oval,
  Line,
  Points,
  Patch,
} from "@shopify/react-native-skia";

import { Title } from "./components/Title";

const { width } = Dimensions.get("window");
const SIZE = width / 4;
const paint = Skia.Paint();
paint.setAntiAlias(true);
paint.setColor(Skia.Color("#61DAFB"));

const strokePaint = paint.copy();
strokePaint.setStyle(PaintStyle.Stroke);
strokePaint.setStrokeWidth(2);

const c = { x: width / 2, y: SIZE / 2 + 16 };
const S = 25;
const c1 = [
  Skia.Point(c.x - 2 * S, c.y - S),
  Skia.Point(c.x - S, c.y - 2 * S),
  Skia.Point(c.x - S, c.y - S),
];

const c2 = [
  Skia.Point(c.x, c.y - 2 * S),
  Skia.Point(c.x + S, c.y),
  Skia.Point(c.x + S, c.y - S),
];

const c3 = [
  Skia.Point(c.x - 10, c.y + 10),
  Skia.Point(c.x + S, c.y),
  Skia.Point(c.x + S, c.y + S),
];

const c4 = [
  Skia.Point(c.x - 2 * S, c.y + S),
  Skia.Point(c.x - S, c.y + 2 * S),
  Skia.Point(c.x - S, c.y + S),
];

const cubics = [...c1, ...c2, ...c3, ...c4];

const PADDING = 16;
const outer = Skia.RRectXY(
  Skia.XYWHRect(2 * SIZE + 3 * 16, PADDING, SIZE, SIZE),
  25,
  25
);
const inner = Skia.RRectXY(
  Skia.XYWHRect(2 * SIZE + 4 * PADDING, 2 * PADDING, SIZE - 32, SIZE - 32),
  0,
  0
);

export const Shapes = () => {
  return (
    <ScrollView>
      <Title>Rectangles</Title>
      <Canvas style={styles.container}>
        <Group color="#61DAFB">
          <Rect x={PADDING} y={PADDING} width={SIZE} height={SIZE} />
          <Rect
            x={SIZE + 2 * PADDING}
            y={PADDING}
            width={SIZE}
            height={SIZE}
            rx={25}
          />
          <DRect outer={outer} inner={inner} />
        </Group>
      </Canvas>
      <Title>Ovals & Circles</Title>
      <Canvas style={styles.container}>
        <Group color="#61DAFB">
          <Oval x={PADDING} y={PADDING} width={2 * SIZE} height={SIZE} />
          <Oval
            x={2 * SIZE + 2 * 16 + SIZE / 2}
            y={PADDING}
            width={SIZE}
            height={SIZE}
          />
        </Group>
      </Canvas>
      <Title>Points & Lines</Title>
      <Canvas style={styles.container}>
        <Group
          color="#61DAFB"
          paintStyle="stroke"
          strokeWidth={PixelRatio.get()}
        >
          <Points mode="polygon" points={cubics} />
          <Line x1={c.x} y1={c.y} x2={SIZE} y2={0} />
          <Points mode="points" points={cubics} color="red" />
        </Group>
      </Canvas>
      <Title>Coon Patch</Title>
      <Canvas style={styles.container}>
        <Patch
          colors={["#61DAFB", "#fb61da", "#61fbcf", "#dafb61"]}
          cubics={cubics}
        />
      </Canvas>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    width,
    height: SIZE + 32,
  },
});
