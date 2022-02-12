import type { IValue } from "../../types";
import { getResolvedParams } from "../functions/getResolvedParams";
import type { AnimationParams, TimingConfig, IAnimation } from "../types";

import { internalRunTiming } from "./internalRunTiming";

/**
 * Creates a new animation on an existing value that will be driven by
 * an animation value. The value will be run from / to the value in
 * params and modified by the provided easing curve for the length of
 * the duration. When the value has reached its desired "to" value the
 * animation will be stopped.
 *
 * @param value The value to animate
 * @param toOrParams To value or Animation parameters
 * @param config Timing configuration
 * @returns an animation value that can be used to start/stop
 * the animation.
 */
export const runTiming = (
  value: IValue<number>,
  toOrParams: number | AnimationParams,
  config?: TimingConfig
): IAnimation => {
  const resolvedParams = getResolvedParams(toOrParams, config);
  return internalRunTiming(value, resolvedParams);
};
