import type { SkPath, Skia } from "../../skia/types";
import { isPath } from "../../skia/types";

export type PathDef = string | SkPath;

export const processPath = (Skia: Skia, rawPath: PathDef) => {
  const path =
    typeof rawPath === "string"
      ? Skia.Path.MakeFromSVGString(rawPath)
      : rawPath;
  if (!path) {
    throw new Error("Invalid path: " + rawPath);
  }
  return path;
};

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export const isPathDef = (def: any): def is PathDef =>
  typeof def === "string" || isPath(def);
