#include "variable.h"

void SetMapEditMode(MapEditMode mode)
{
    SetBlockTypeSelect = (mode == MapEditMode::PlaceBlock);
    blockDeleteMode = (mode == MapEditMode::DeleteBlock);
    playerPointSet = (mode == MapEditMode::SetPlayerPoint);
}