#include "function.h"
#include "resource.h"

void StageClear()
{
	PlaySound(MAKEINTRESOURCE(IDW_STAGE_CLEAR), g_hInst, SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
	stageClear = true;
	stageCleared[stage - 1] = true;
	stageManager = false;
	doubleJumpMode = false;
	LMBHold = false;
	RMBHold = false;
	SetBlockTypeSelect = false;
	playerPointSet = false;
	blockDeleteMode = false;
}

// 게임 멀티 플레이로 변환시 해야할 내용
// StageClear() 이거는 필요가 없어짐
// - 살아 있는 플레이어가 별에 처음 도착하면 EndRound(playerId)로 라운드를 끝낸다.
// - 아무도 도착하지 못하고 전원이 사망하면 EndRound(0)으로 끝낸다. 이때는 득점자가 없다.
// - EndRound()는 결과 단계로 먼저 전환해 같은 라운드의 득점을 한 번만 처리한다.