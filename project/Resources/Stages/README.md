# Stage placement rule

Stages requested for this project must follow the Development editor grid.

- Tile size: `1.0` world unit / `16 x 16` design pixels.
- Tile cells use integer indices.
- Every placed object uses the cell center: `x = gridX + 0.5`, `y = gridY + 0.5`.
- Keep model-specific visual correction in `ItemDefinition`; do not bake it into stage JSON positions.
- Off-grid positions are not allowed in generated stage data because editor selection, rotation, and deletion operate from the grid cursor.
