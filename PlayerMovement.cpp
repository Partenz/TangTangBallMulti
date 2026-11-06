#include "function.h"


// 공통 점프 상태 계산. 효과음은 각 호출 위치에서 처리합니다.
void StartJump(double startY, int heightInCells)
{
    playerJumpStartY = startY;
    playerJumpHeight = startY - gridSpacing * heightInCells;
}

// 전기블럭은 ON/OFF 타입을 모두 합산합니다.
static bool IsLightningBlock(const Block* block)
{
    return block->BlockType == BlockType_LightningOnBlock
        || block->BlockType == BlockType_LightningOffBlock;
}

// 홀수 개이면 모두 ON, 짝수 개(0개 포함)이면 모두 OFF.
// DrawStage()가 BlockType으로 비트맵을 선택하므로 그림도 함께 바뀝니다.
static bool UpdateLightningState()
{
    int count = 0;
    Block* p = firstBlockPointer;
    for (int i = 0; i < BlockCnt; ++i) {
        if (IsLightningBlock(p))
            ++count;
        p = p->next;
    }

    const bool isOn = (count % 2 == 1);
    p = firstBlockPointer;
    for (int i = 0; i < BlockCnt; ++i) {
        if (IsLightningBlock(p)) {
            p->BlockType = isOn
                ? BlockType_LightningOnBlock
                : BlockType_LightningOffBlock;
        }
        p = p->next;
    }
    return isOn;
}

// 최신 좌표로 검사합니다. 위/아래/좌/우 어느 방향에서 닿아도 사망합니다.
static bool HandleLightningContact(bool lightningOn)
{
    const int margin = 4;

    if (!lightningOn || !gamestart || stageClear
        || SetBlockTypeSelect || playerPointSet || blockDeleteMode)
        return false;

    Block* p = firstBlockPointer;
    for (int i = 0; i < BlockCnt; ++i) {
        if (IsLightningBlock(p)
            && playerX + playerSize >= p->rt.left + margin && playerX - playerSize <= p->rt.right - margin && playerY + playerSize >= p->rt.top + margin && playerY - playerSize <= p->rt.bottom - margin) {
            playerMoveStart = false;
            playerDropping = true;
            doubleJumpMode = false;
            PlaySound(L"DeadSound.wav", NULL, SND_ASYNC);
            mapDelete();
            mapLoad();
            StartJump(playerY + playerSize - 3, 2);
            UpdateLightningState();
            // 맵 재로딩으로 p가 삭제되었으므로 즉시 종료합니다.
            return true;
        }
        p = p->next;
    }
    return false;
}

// 더블클릭 메시지에서 호출하며 아이템은 한 번만 사용합니다.
void TryUseDoubleJump()
{
    if (not gamestart or stageClear or not doubleJumpMode
        or SetBlockTypeSelect or playerPointSet or blockDeleteMode)
        return;

    doubleJumpMode = false;
    playerMoveStart = true;
    playerDropping = false;
    PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
    StartJump(playerY + playerSize, 3);
}

// 좌우 버튼의 누름 상태에 따른 수평 이동
void ApplyHorizontalInput()
{
    if (RMBHold and not LMBHold) { // 우측 방향 이동
        playerX += playerSpeed * 0.7;
    }
    else if (LMBHold and not RMBHold) { // 좌측으로 이동 
        playerX -= playerSpeed * 0.7;
    }
}

// 1. 좌우 충돌, 입력 이동 및 아이템 획득
void UpdateHorizontalMovement()
{
    Block* p = firstBlockPointer;
    // x 이동
    for (int i = 0; i < BlockCnt; i++) {
        if (playerRT.left >= p->rt.left and playerRT.left <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom
            and p->BlockType != BlockType_Star and p->BlockType != BlockType_DoubleJump and !IsLightningBlock(p)) { // 플레이어 왼쪽 히트박스가 블럭내부로 들어옴
            if (p->BlockType != BlockType_UpDown) {
                playerX += playerSpeed * 0.7;
            }
            break;
        }
        else if (playerRT.right >= p->rt.left and playerRT.right <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom
            and p->BlockType != BlockType_Star and p->BlockType != BlockType_DoubleJump and !IsLightningBlock(p)) { // 플레이어 오른쪽 히트박스가 블럭내부로 들어옴
            if (p->BlockType != BlockType_UpDown) {
                playerX -= playerSpeed * 0.7;
            }
            break;
        }
        else if (playerRT.left >= p->rt.left and playerRT.left <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom and
            (p->BlockType == BlockType_Star or p->BlockType == BlockType_DoubleJump)) { // 플레이어 왼쪽 히트박스가 블럭내부로 들어옴
            if (p->BlockType == BlockType_DoubleJump) { // 더블 점프 아이템 먹음
                ApplyHorizontalInput();
                Block* temp = p;
                p = p->prev;
                blockDelete(temp);
                doubleJumpMode = true;
            }
            else if (p->BlockType == BlockType_Star) { // 별 먹음
                PlaySound(L"StarSound.wav", NULL, SND_ASYNC);
                ApplyHorizontalInput();
                Block* temp = p;
                p = p->prev;
                blockDelete(temp);
                starCnt--;
            }
            break;
        }
        else if (playerRT.right >= p->rt.left and playerRT.right <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom
            and (p->BlockType == BlockType_Star or p->BlockType == BlockType_DoubleJump)) { // 플레이어 오른쪽 히트박스가 블럭내부로 들어옴
            if (p->BlockType == BlockType_DoubleJump) { // 더블 점프 아이템 먹음
                ApplyHorizontalInput();
                Block* temp = p;
                p = p->prev;
                blockDelete(temp);
                doubleJumpMode = true;
            }
            else if (p->BlockType == BlockType_Star) { // 별 먹음
                PlaySound(L"StarSound.wav", NULL, SND_ASYNC);
                ApplyHorizontalInput();
                Block* temp = p;
                p = p->prev;
                blockDelete(temp);
                starCnt--;
            }
            break;
        }
        else if (i == BlockCnt - 1) { // 모든 블럭과 충돌 안 함
            ApplyHorizontalInput();
        }
        if (p->next != firstBlockPointer)
            p = p->next;
    }
}

// 2. 세로 이동, 착지, 천장 충돌 및 점프 종료
void UpdateVerticalMovement()
{
    // y 이동
    Block* p = firstBlockPointer;
    if (playerDropping) {
        playerY += playerSpeed;
    }
    else if (not playerDropping) {
        playerY -= playerSpeed;
    }
    for (int i = 0; i < BlockCnt; i++) {
        if (!IsLightningBlock(p) and playerX + playerSize / 4 >= p->rt.left and playerX - playerSize / 4 <= p->rt.right) { // 블럭과 같은 열에 있음
            if (playerDropping and playerY + playerSize >= p->rt.top and playerY + playerSize <= p->rt.bottom and p->BlockType != BlockType_UpDown) { // 블럭과 충돌
                if (p->BlockType == BlockType_Basic) { // 일반 블럭과 충돌
                    playerDropping = false;
                    PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
                    StartJump(playerY + playerSize - 3, 2);
                }
                else if (p->BlockType == BlockType_Thorn) { // 가시 블럭과 충돌
                    doubleJumpMode = false;
                    PlaySound(L"DeadSound.wav", NULL, SND_ASYNC);
                    playerX = stageStartPoint.x;
                    playerY = stageStartPoint.y;
                    mapDelete();
                    mapLoad();
                }
                else if (p->BlockType == BlockType_Once) { // 일회용 블럭과 충돌
                    playerDropping = false;
                    PlaySound(L"OnceBlockSound.wav", NULL, SND_ASYNC);
                    Block* temp = p;
                    p = p->prev;
                    blockDelete(temp);
                    StartJump(playerY + playerSize - 3, 2);
                }
                else if (p->BlockType == BlockType_Jump) { // 점프 블럭과 충돌
                    playerDropping = false;
                    PlaySound(L"JumpBlockSound.wav", NULL, SND_ASYNC);
                    StartJump(playerY + playerSize - 3, 3);
                }

                break;
            }
            else if (not playerDropping and playerY - playerSize >= playerJumpHeight) { // 점프
                if (playerY - playerSize <= p->rt.bottom and playerY - playerSize >= p->rt.top
                    and p->BlockType != BlockType_UpDown and p->BlockType != BlockType_DoubleJump and p->BlockType != BlockType_Star) { // 업다운 블럭이 아닌 블럭과 위로 충돌해 낙하 시작
                    playerDropping = true;
                }
                else if (playerY - playerSize >= p->rt.top and playerY - playerSize <= p->rt.bottom and p->BlockType == BlockType_UpDown) { // 점프 최대 지점에서 업다운 블럭 만남
                    playerDropping = false;
                    StartJump(p->rt.top - 3, 2);
                }
            }

            else if (not playerDropping and playerY - playerSize < playerJumpHeight) { // 점프 완료 후 다시 낙하
                playerDropping = true;
            }
        }
        else if (not playerDropping and playerY - playerSize < playerJumpHeight) { // 점프 완료 후 다시 낙하
            playerDropping = true;
        }

        if (p->next != firstBlockPointer)
            p = p->next;
    }
}

// 3. 화면 아래로 떨어지면 시작 위치 및 맵 복구
void HandleFallOut()
{
    if (playerY >= 840 and not SetBlockTypeSelect and not blockDeleteMode and not playerPointSet and not stageClear) { // 바닥으로 떨어짐
        playerMoveStart = false;
        doubleJumpMode = false;
        PlaySound(L"DeadSound.wav", NULL, SND_ASYNC);
        playerX = stageStartPoint.x;
        playerY = stageStartPoint.y;
        mapDelete();
        mapLoad();
        playerJumpStartY = playerY + playerSize - 3;
        playerJumpHeight = playerJumpStartY - gridSpacing * 2;
    }
}

// 4. 최종 좌표로 히트박스 갱신
void UpdatePlayerHitbox()
{
    // 플레이어 히트박스 설정
    playerRT.left = playerX - playerSize;
    playerRT.top = playerY - playerSize;
    playerRT.right = playerX + playerSize;
    playerRT.bottom = playerY + playerSize;
}

// 외부(main.cpp)에서 호출하는 함수는 이것 하나입니다.
void UpdatePlayerMovement()
{
    const bool lightningOn = UpdateLightningState();
    UpdatePlayerHitbox();

    if (HandleLightningContact(lightningOn)) {
        UpdatePlayerHitbox();
        return;
    }

    UpdateHorizontalMovement();
    UpdatePlayerHitbox();
    if (HandleLightningContact(lightningOn)) {
        UpdatePlayerHitbox();
        return;
    }

    UpdateVerticalMovement();
    UpdatePlayerHitbox();
    if (HandleLightningContact(lightningOn)) {
        UpdatePlayerHitbox();
        return;
    }

    HandleFallOut();
    UpdateLightningState();
    UpdatePlayerHitbox();
}
