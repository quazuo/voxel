#!/bin/bash

for asset in assets/*; do
  assetName=$(basename "$asset")
  cp "$asset" cmake-build-debug/"$assetName"
done

for file in src/render/*; do
  fileName=$(basename "$file")
  if [[ $fileName = "shader."* ]]; then
    cp "$file" cmake-build-debug/"$fileName"
  fi
done
