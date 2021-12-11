import React from "react";
import { Dimensions, StyleSheet } from "react-native";
import { Canvas, Timing, useValue, useLoop } from "@shopify/react-native-skia";

import { AnimationElement, AnimationDemo, Size, Padding } from "./Components";

const { width } = Dimensions.get("window");

export const InterpolationWithEasing = () => {
  const progress = useValue(0);
  useLoop(
    progress,
    Timing.create({
      from: 10,
      to: width - Size - Padding,
      durationSeconds: 1,
      easing: Timing.Easing.inOut(Timing.Easing.cubic),
    }),
    { yoyo: true }
  );
  return (
    <AnimationDemo title={"Interpolating value using an easing."}>
      <Canvas style={styles.canvas}>
        <AnimationElement x={() => progress.value} />
      </Canvas>
    </AnimationDemo>
  );
};

const styles = StyleSheet.create({
  canvas: {
    height: 80,
    width: width - Padding,
    backgroundColor: "#FEFEFE",
  },
});
